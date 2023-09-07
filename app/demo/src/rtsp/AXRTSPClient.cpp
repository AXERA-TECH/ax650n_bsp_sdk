/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// Copyright (c) 1996-2020, Live Networks, Inc.  All rights reserved
// A demo application, showing how to create and run a RTSP client (that can potentially receive multiple streams concurrently).
//
// NOTE: This code - although it builds a running application - is intended only to illustrate how to develop your own RTSP
// client application.  For a full-featured RTSP client application - with much more functionality, and many options - see
// "openRTSP": http://www.live555.com/openRTSP/

#include "AXRTSPClient.h"
#include "AXException.hpp"
#include "h264.hpp"
#include "hevc.hpp"

// global define
#define KEEP_ALIVE_PACKAGE_INTERVAL (10)
#define RTSP_CLIENT_VERBOSITY_LEVEL (1)  // by default, print verbose output from each "RTSPClient"
#define REQUEST_STREAMING_OVER_TCP (False)
// Even though we're not going to be doing anything with the incoming data, we still need to receive it.
// Define the size of the buffer that we'll use:
#define DUMMY_SINK_RECEIVE_BUFFER_SIZE 800000
#define NALU_START_CODE_LENGTH (4)
static constexpr unsigned char NALU_START_CODE[NALU_START_CODE_LENGTH] = {0x00, 0x00, 0x00, 0x01};

// static unsigned rtspClientCount = 0;  // Counts how many streams (i.e., "RTSPClient"s) are currently in use.
// char eventLoopWatchVariable = 0;

// Forward function definitions:

// RTSP 'response handlers':
static void continueAfterDESCRIBE(RTSPClient *rtspClient, int resultCode, char *resultString);
static void continueAfterSETUP(RTSPClient *rtspClient, int resultCode, char *resultString);
static void continueAfterPLAY(RTSPClient *rtspClient, int resultCode, char *resultString);

// Other event handler functions:
// called when a stream's subsession (e.g., audio or video substream) ends
static void subsessionAfterPlaying(void *clientData);
// called when a RTCP "BYE" is received for a subsession
static void subsessionByeHandler(void *clientData, char const *reason);
// called at the end of a stream's expected duration (if the stream has not already signaled its end using a RTCP "BYE")
static void streamTimerHandler(void *clientData);

// Used to iterate through each stream's 'subsessions', setting up each one:
static void setupNextSubsession(RTSPClient *rtspClient);

// Used to shut down and close a stream (including its "RTSPClient" object):
static void shutdownStream(RTSPClient *rtspClient, int exitCode = 1);

// A function that outputs a string that identifies each stream (for debugging output).  Modify this if you wish:
UsageEnvironment &operator<<(UsageEnvironment &env, const RTSPClient &rtspClient) {
    return env << "[URL:\"" << rtspClient.url() << "\"]: ";
}

// A function that outputs a string that identifies each subsession (for debugging output).  Modify this if you wish:
UsageEnvironment &operator<<(UsageEnvironment &env, const MediaSubsession &subsession) {
    return env << subsession.mediumName() << "/" << subsession.codecName();
}

// Implementation of the RTSP 'response handlers':
static void continueAfterDESCRIBE(RTSPClient *rtspClient, int resultCode, char *resultString) {
    do {
        UsageEnvironment &env = rtspClient->envir();                  // alias
        StreamClientState &scs = ((CAXRTSPClient *)rtspClient)->scs;  // alias

        if (resultCode != 0) {
            env << *rtspClient << "Failed to get a SDP description: " << resultString << "\n";
            delete[] resultString;
            break;
        }

        char *const sdpDescription = resultString;
        // printf("%s: Got a SDP description: %s\n", rtspClient->url(), resultString);

        // Create a media session object from this SDP description:
        scs.session = MediaSession::createNew(env, sdpDescription);
        if (scs.session == NULL) {
            delete[] sdpDescription;  // because we don't need it anymore
            env << *rtspClient << "Failed to create a MediaSession object from the SDP description: " << env.getResultMsg() << "\n";
            break;
        } else if (!scs.session->hasSubsessions()) {
            delete[] sdpDescription;  // because we don't need it anymore
            env << *rtspClient << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
            break;
        }

        if (scs.cb.OnTracksInfo) {
            TRACKS_INFO_T tracks;
            tracks.url = (unsigned char *)rtspClient->url();
            tracks.sdp = (unsigned char *)sdpDescription;

            SPropRecord *sps = nullptr;
            SPropRecord *pps = nullptr;
            SPropRecord *vps = nullptr;
            MediaSubsessionIterator iter(*scs.session);
            while (1) {
                MediaSubsession *subsession = iter.next();
                if (!subsession) {
                    break;
                }

                TRACK_INFO_T track;
                track.rtpPayload = subsession->rtpPayloadFormat();
                track.control = (char *)subsession->controlPath();

                if (!sps && (0 == strcmp(subsession->mediumName(), "video"))) {
                    unsigned int num = 0;
                    if (0 == strcmp(subsession->codecName(), "H264")) {
                        track.enPayload = PT_H264;

                        sps = parseSPropParameterSets(subsession->fmtp_spropparametersets(), num);
                        if (sps && num > 0) {
                            for (unsigned int i = 0; i < num; ++i) {
                                track.video.sps[i] = &sps[i].sPropBytes[0];
                                track.video.len[i] = sps[i].sPropLength;
                            }
                        }

                        tracks.tracks[subsession] = track;
                    } else if (0 == strcmp(subsession->codecName(), "H265")) {
                        track.enPayload = PT_H265;

                        sps = parseSPropParameterSets(subsession->fmtp_spropsps(), num);
                        if (sps && num > 0) {
                            track.video.sps[0] = &sps[0].sPropBytes[0];
                            track.video.len[0] = sps[0].sPropLength;
                        }

                        pps = parseSPropParameterSets(subsession->fmtp_sproppps(), num);
                        if (pps && num > 0) {
                            track.video.sps[1] = &pps[0].sPropBytes[0];
                            track.video.len[1] = pps[0].sPropLength;
                        }

                        vps = parseSPropParameterSets(subsession->fmtp_spropvps(), num);
                        if (vps && num > 0) {
                            track.video.sps[2] = &vps[0].sPropBytes[0];
                            track.video.len[2] = vps[0].sPropLength;
                        }

                        tracks.tracks[subsession] = track;
                    } else {
                        sps = nullptr;
                    }

                } /* else if (0 == strcmp(subsession->mediumName(), "audio")) {
                    // TODO: Audio track

                    tracks.tracks[subsession] = track;
                } */
            }

            scs.cb.OnTracksInfo(tracks);

            if (sps) {
                delete[] sps;
            }

            if (pps) {
                delete[] pps;
            }

            if (vps) {
                delete[] vps;
            }
        }

        delete[] sdpDescription;

        // Then, create and set up our data source objects for the session.  We do this by iterating over the session's 'subsessions',
        // calling "MediaSubsession::initiate()", and then sending a RTSP "SETUP" command, on each one.
        // (Each 'subsession' will have its own data source.)
        scs.iter = new MediaSubsessionIterator(*scs.session);
        setupNextSubsession(rtspClient);
        return;
    } while (0);

    /* An unrecoverable error occurred with this stream.
       Jira 5905
       - if network is linked up, but server is not lanuched, continueAfterDESCRIBE is invoked, shutdownStream will delete rtspClient
       - if network is linked down, sendDescribeCommand fail by setup socket, so continueAfterDESCRIBE is NOT invoked, rtspClient remain not deleted
       That makes confused  to distinguish when to invoke CAXRTSPClient::Stop(), so delete rtspClient by CAXRTSPClient::Stop() explicitly
    */
    // shutdownStream(rtspClient);
}

// By default, we request that the server stream its data using RTP/UDP.
// If, instead, you want to request that the server stream via RTP-over-TCP, change the following to True:
static void setupNextSubsession(RTSPClient *rtspClient) {
    UsageEnvironment &env = rtspClient->envir();                  // alias
    StreamClientState &scs = ((CAXRTSPClient *)rtspClient)->scs;  // alias

    scs.subsession = scs.iter->next();
    if (scs.subsession != NULL) {
        if (!scs.subsession->initiate()) {
            env << *rtspClient << "Failed to initiate the \"" << *scs.subsession << "\" subsession: " << env.getResultMsg() << "\n";
            setupNextSubsession(rtspClient);  // give up on this subsession; go to the next one
        } else {
            if (scs.subsession->rtcpIsMuxed()) {
                //      LOG_M(CLIENT, "[%s]: Initiated the %s subsession, client port(%d)", rtspClient->url(), scs.subsession->mediumName(),
                //           scs.subsession->clientPortNum());
            } else {
                //     LOG_M(CLIENT, "[%s]: Initiated the %s subsession, client ports(%d-%d)", rtspClient->url(),
                //     scs.subsession->mediumName(),
                //          scs.subsession->clientPortNum(), scs.subsession->clientPortNum() + 1);
            }

#if 0
            if (scs.subsession->rtpSource() != NULL) {
                // Because we're saving the incoming data, rather than playing
                // it in real time, allow an especially large time threshold
                // (1 second) for reordering misordered incoming packets:
                unsigned const thresh = 1000000; // 1 second
                scs.subsession->rtpSource()->setPacketReorderingThresholdTime(thresh);
            }
#endif

            // Continue setting up this subsession, by sending a RTSP "SETUP" command:
            rtspClient->sendSetupCommand(*scs.subsession, continueAfterSETUP, False, (1 == scs.streamTransportMode) ? True : False);
        }
        return;
    }

    if (scs.cb.OnPreparePlay) {
        scs.cb.OnPreparePlay();
    }

    // We've finished setting up all of the subsessions.  Now, send a RTSP "PLAY" command to start the streaming:
    if (scs.session->absStartTime() != NULL) {
        // Special case: The stream is indexed by 'absolute' time, so send an appropriate "PLAY" command:
        rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY, scs.session->absStartTime(), scs.session->absEndTime());
    } else {
        // scs.duration = scs.session->playEndTime() - scs.session->playStartTime();
        scs.duration = KEEP_ALIVE_PACKAGE_INTERVAL;
        rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY);
    }
}

static void continueAfterSETUP(RTSPClient *rtspClient, int resultCode, char *resultString) {
    // LOG_M(CLIENT, "continueAfterSETUP +++");
    do {
        UsageEnvironment &env = rtspClient->envir();                  // alias
        StreamClientState &scs = ((CAXRTSPClient *)rtspClient)->scs;  // alias

        if (resultCode != 0) {
            env << *rtspClient << "Failed to set up the \"" << *scs.subsession << "\" subsession: " << resultString << "\n";
            break;
        }

        if (scs.subsession->rtcpIsMuxed()) {
            //   LOG_M(CLIENT, "[%s]: Set up the %s subsession, client port(%d)", rtspClient->url(), scs.subsession->mediumName(),
            //         scs.subsession->clientPortNum());
        } else {
            //     LOG_M(CLIENT, "[%s]: Initiated the %s subsession, client ports(%d-%d)", rtspClient->url(), scs.subsession->mediumName(),
            //           scs.subsession->clientPortNum(), scs.subsession->clientPortNum() + 1);
        }

        // Having successfully setup the subsession, create a data sink for it, and call "startPlaying()" on it.
        // (This will prepare the data sink to receive data; the actual flow of data from the client won't start happening until later,
        // after we've sent a RTSP "PLAY" command.)

        scs.subsession->sink = DummySink::createNew(env, *scs.subsession, rtspClient->url(), scs.cb, scs.bufSize);
        // perhaps use your own custom "MediaSink" subclass instead
        if (scs.subsession->sink == NULL) {
            env << *rtspClient << "Failed to create a data sink for the \"" << *scs.subsession << "\" subsession: " << env.getResultMsg()
                << "\n";
            break;
        }

        //   LOG_M(CLIENT, "[%s]: Created a data sink for %s subsession", rtspClient->url(), scs.subsession->mediumName());
        scs.subsession->miscPtr = rtspClient;  // a hack to let subsession handler functions get the "RTSPClient" from the subsession
        scs.subsession->sink->startPlaying(*(scs.subsession->readSource()), subsessionAfterPlaying, scs.subsession);
        // Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
        if (scs.subsession->rtcpInstance() != NULL) {
            scs.subsession->rtcpInstance()->setByeWithReasonHandler(subsessionByeHandler, scs.subsession);
        }
    } while (0);
    delete[] resultString;

    // Set up the next subsession, if any:
    setupNextSubsession(rtspClient);
    //  LOG_M(CLIENT, "continueAfterSETUP ---");
}

static void continueAfterPLAY(RTSPClient *rtspClient, int resultCode, char *resultString) {
    // LOG_M(CLIENT, "continueAfterPLAY +++");
    // Boolean success = False;

    do {
        UsageEnvironment &env = rtspClient->envir();                  // alias
        StreamClientState &scs = ((CAXRTSPClient *)rtspClient)->scs;  // alias

        if (resultCode != 0) {
            env << *rtspClient << "Failed to start playing session: " << resultString << "\n";
            break;
        }

// Set a timer to be handled at the end of the stream's expected duration (if the stream does not already signal its end
// using a RTCP "BYE").  This is optional.  If, instead, you want to keep the stream active - e.g., so you can later
// 'seek' back within it and do another RTSP "PLAY" - then you can omit this code.
// (Alternatively, if you don't want to receive the entire stream, you could set this timer for some shorter value.)
#if 0
        if (scs.duration > 0) {
            unsigned uSecsToDelay = (unsigned)(scs.duration * 1000000);
            scs.streamTimerTask = env.taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc *)streamTimerHandler, rtspClient);
        }
#else
        if (scs.keepAliveInterval > 0) {
            unsigned uSecsToDelay = 10000; /* delay 10ms to lanuch heat beat handler */
            scs.streamTimerTask = env.taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc *)streamTimerHandler, rtspClient);
        }
#endif

        //   LOG_M(CLIENT, "[%s] Started playing session ...", rtspClient->url());
        scs.played.SetEvent();

        // success = True;
    } while (0);

    delete[] resultString;

#if 0 /*
        rtspClient will be freed of shutdownStream, see the below comment of continueAfterDESCRIBE
        Mmm, fixme: Is there any better solution to avoid double free?
    */
    if (!success) {
        // An unrecoverable error occurred with this stream.
        shutdownStream(rtspClient);
    }
#endif
    //  LOG_M(CLIENT, "continueAfterPLAY ---");
}

// Implementation of the other event handlers:
static void subsessionAfterPlaying(void *clientData) {
    // LOG_M(CLIENT, "subsessionAfterPlaying +++");
    MediaSubsession *subsession = (MediaSubsession *)clientData;
    RTSPClient *rtspClient = (RTSPClient *)(subsession->miscPtr);

    // Begin by closing this subsession's stream:
    Medium::close(subsession->sink);
    subsession->sink = NULL;

    // Next, check whether *all* subsessions' streams have now been closed:
    MediaSession &session = subsession->parentSession();
    MediaSubsessionIterator iter(session);
    while ((subsession = iter.next()) != NULL) {
        if (subsession->sink != NULL) return;  // this subsession is still active
    }

    // All subsessions' streams have now been closed, so shutdown the client:
    shutdownStream(rtspClient);
    //  LOG_M(CLIENT, "subsessionAfterPlaying ---");
}

static void subsessionByeHandler(void *clientData, char const *reason) {
    //  LOG_M(CLIENT, "subsessionByeHandler +++");
    MediaSubsession *subsession = (MediaSubsession *)clientData;
    // RTSPClient *rtspClient = (RTSPClient *)subsession->miscPtr;

    // LOG_M(CLIENT, "Received RTCP <BYE> (Reason: \"%s\") on %s subsession", rtspClient->url(), reason, subsession->mediumName());

    // Now act as if the subsession had closed:
    subsessionAfterPlaying(subsession);

    //  LOG_M(CLIENT, "subsessionByeHandler ---");
}

static void checkAlive(RTSPClient *rtspClient, int resultCode, char *resultString) {
    StreamClientState &scs = ((CAXRTSPClient *)rtspClient)->scs;
    if (scs.cb.OnCheckAlive) {
        scs.cb.OnCheckAlive(resultCode, resultString);
    }

    delete []resultString;
}

static void streamTimerHandler(void *clientData) {
    CAXRTSPClient *rtspClient = (CAXRTSPClient *)clientData;
    StreamClientState &scs = rtspClient->scs;     // alias
    UsageEnvironment &env = rtspClient->envir();  // alias
                                                  // LOG_M_I(CLIENT, "Send KeepAlive packet.");
    rtspClient->sendGetParameterCommand(*rtspClient->scs.session, checkAlive, NULL);

    /* Restart another keep alive package */
    scs.streamTimerTask =
        env.taskScheduler().scheduleDelayedTask(scs.keepAliveInterval * 1000000, (TaskFunc *)streamTimerHandler, rtspClient);
}

static void shutdownStream(RTSPClient *rtspClient, int exitCode) {
    StreamClientState &scs = ((CAXRTSPClient *)rtspClient)->scs;
    // UsageEnvironment &env = rtspClient->envir();
    // std::string url = rtspClient->url();
    // env << "shutdownStream " << url.c_str() << "+++\n";

    // First, check whether any subsessions have still to be closed:
    if (scs.session != NULL) {
        Boolean someSubsessionsWereActive = False;
        MediaSubsessionIterator iter(*scs.session);
        MediaSubsession *subsession;

        while ((subsession = iter.next()) != NULL) {
            if (subsession->sink != NULL) {
                Medium::close(subsession->sink);
                subsession->sink = NULL;

                if (subsession->rtcpInstance() != NULL) {
                    subsession->rtcpInstance()->setByeHandler(NULL,
                                                              NULL);  // in case the server sends a RTCP "BYE" while handling "TEARDOWN"
                }

                someSubsessionsWereActive = True;
            }
        }

        if (someSubsessionsWereActive) {
            // Send a RTSP "TEARDOWN" command, to tell the server to shutdown the stream.
            // Don't bother handling the response to the "TEARDOWN".
            rtspClient->sendTeardownCommand(*scs.session, NULL);
        }
    }

    //  LOG_M(CLIENT, "Closing the stream.");

    Medium::close(rtspClient);
    // Note that this will also cause this stream's "StreamClientState" structure to get reclaimed.

#if 0
    if (--rtspClientCount == 0) {
        // The final stream has ended, so exit the application now.
        // (Of course, if you're embedding this code into your own application, you might want to comment this out,
        // and replace it with "eventLoopWatchVariable = 1;", so that we leave the LIVE555 event loop, and continue running "main()".)
        eventLoopWatchVariable = 1;
    }
#endif
    // env << "shutdownStream " << url.c_str() << "---\n";
}

// Implementation of "CAXRTSPClient":
CAXRTSPClient *CAXRTSPClient::createNew(UsageEnvironment &env, char const *rtspURL, RtspClientCallback cb, int bufSize, int verbosityLevel,
                                        char const *applicationName, portNumBits tunnelOverHTTPPortNum) {
    return new CAXRTSPClient(env, rtspURL, cb, bufSize, verbosityLevel, applicationName, tunnelOverHTTPPortNum);
}

CAXRTSPClient::CAXRTSPClient(UsageEnvironment &env, char const *rtspURL, RtspClientCallback cb, int bufSize, int verbosityLevel,
                             char const *applicationName, portNumBits tunnelOverHTTPPortNum)
    : RTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, -1) {
    scs.cb = cb;
    if (bufSize <= 0) {
        bufSize = DUMMY_SINK_RECEIVE_BUFFER_SIZE;
    }
    scs.bufSize = bufSize;
}

CAXRTSPClient::~CAXRTSPClient() {
}

void CAXRTSPClient::Start(void) {
    // Next, send a RTSP "DESCRIBE" command, to get a SDP description for the stream.
    // Note that this command - like all RTSP commands - is sent asynchronously; we do not block, waiting for a response.
    // Instead, the following function call returns immediately, and we handle the RTSP response later, from within the event loop:
    sendDescribeCommand(continueAfterDESCRIBE);
}

void CAXRTSPClient::Stop(void) {
    shutdownStream(this);
}

// Implementation of "StreamClientState":
StreamClientState::StreamClientState() : iter(NULL), session(NULL), subsession(NULL), streamTimerTask(NULL), duration(0.0), bufSize(0) {
}

StreamClientState::~StreamClientState() {
    delete iter;
    if (session != NULL) {
        // We also need to delete "session", and unschedule "streamTimerTask" (if set)
        UsageEnvironment &env = session->envir();  // alias

        env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
        Medium::close(session);
    }
}

// Implementation of "DummySink":
DummySink *DummySink::createNew(UsageEnvironment &env, MediaSubsession &subsession, char const *streamId, RtspClientCallback cb,
                                unsigned int bufSize) {
    return new DummySink(env, subsession, streamId, cb, bufSize);
}

DummySink::DummySink(UsageEnvironment &env, MediaSubsession &subsession, char const *streamId, RtspClientCallback cb, unsigned int bufSize)
    : MediaSink(env), fSubsession(subsession), firstFrame(1) {
    m_cacheBufCap = 0x2000;
    m_cacheBufLen = 0;
    m_frameBufCap = bufSize;
    m_nReceiveBufferSize = m_frameBufCap + m_cacheBufCap;
    fReceiveBuffer = (unsigned char *)malloc(m_nReceiveBufferSize);
    m_cacheBuf = &fReceiveBuffer[0];
    m_frameBuf = &fReceiveBuffer[m_cacheBufCap];
    memcpy(m_frameBuf, &NALU_START_CODE[0], NALU_START_CODE_LENGTH);

    m_cb = cb;

    if (0 == strcmp(subsession.codecName(), "H264")) {
        m_ePayload = PT_H264;
    } else if (0 == strcmp(subsession.codecName(), "H265")) {
        m_ePayload = PT_H265;
    } else {
        m_ePayload = PT_BUTT;
    }
}

DummySink::~DummySink() {
    free(fReceiveBuffer);
}

void DummySink::afterGettingFrame(void *clientData, unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime,
                                  unsigned durationInMicroseconds) {
    DummySink *sink = (DummySink *)clientData;
    sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

// If you don't want to see debugging output for each received frame, then comment out the following line:
// #define DEBUG_PRINT_EACH_RECEIVED_FRAME 1
void DummySink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime,
                                  unsigned /*durationInMicroseconds*/) {
// We've just received a frame of data.  (Optionally) print out information about it:
#ifdef DEBUG_PRINT_EACH_RECEIVED_FRAME
    // if (fStreamId != NULL) envir() << "Stream \"" << fStreamId << "\"; ";
    envir() << fSubsession.mediumName() << "/" << fSubsession.codecName() << ":\tReceived " << frameSize << " bytes";
    if (numTruncatedBytes > 0) envir() << " (with " << numTruncatedBytes << " bytes truncated)";
    char uSecsStr[6 + 1];  // used to output the 'microseconds' part of the presentation time
    sprintf(uSecsStr, "%06u", (unsigned)presentationTime.tv_usec);
    envir() << ".\tPresentation time: " << (int)presentationTime.tv_sec << "." << uSecsStr;
    if (fSubsession.rtpSource() != NULL && !fSubsession.rtpSource()->hasBeenSynchronizedUsingRTCP()) {
        envir() << "!";  // mark the debugging output to indicate that this presentation time is not RTCP-synchronized
    }
#ifdef DEBUG_PRINT_NPT
    envir() << "\tNPT: " << fSubsession.getNormalPlayTime(presentationTime);
#endif
    envir() << "\n";
#endif

    if (PT_H264 == m_ePayload) {
        frameSize += NALU_START_CODE_LENGTH;
        AX_U8 naluType = (m_frameBuf[NALU_START_CODE_LENGTH] & 0x1F);
        if (H264_NAL_SEI == naluType) {
            /* ignore SEI */
        } else if (H264_NAL_SPS == naluType || H264_NAL_PPS == naluType) {
            if (m_cacheBufLen + frameSize > m_cacheBufCap) {
                envir() << "rtsp cache buffer is not enough, realloc from " << m_nReceiveBufferSize;
                reallocBuf(m_frameBuf, frameSize);
                envir() << " to " << m_nReceiveBufferSize << "\n";
            } else {
                memmove(&m_cacheBuf[m_cacheBufLen], m_frameBuf, frameSize);
                m_cacheBufLen += frameSize;
            }
        } else if (naluType == H264_NAL_IDR_SLICE) {
            if (m_cacheBufLen > 0) {
                memmove(&m_cacheBuf[m_cacheBufLen], m_frameBuf, frameSize);
                m_cacheBufLen += frameSize;
                m_cb.OnRecvFrame(&fSubsession, &m_cacheBuf[0], m_cacheBufLen, m_ePayload, NALU_TYPE_IDR, presentationTime);
                m_cacheBufLen = 0;
                memcpy(m_frameBuf, &NALU_START_CODE[0], NALU_START_CODE_LENGTH);
            } else {
                m_cb.OnRecvFrame(&fSubsession, m_frameBuf, frameSize, m_ePayload, NALU_TYPE_IDR, presentationTime);
            }
        } else {
            m_cb.OnRecvFrame(&fSubsession, m_frameBuf, frameSize, m_ePayload, NALU_TYPE_OTH, presentationTime);
        }
    }
    else if (PT_H265 == m_ePayload) {
        frameSize += NALU_START_CODE_LENGTH;
        AX_U8 naluType = ((m_frameBuf[NALU_START_CODE_LENGTH] & 0x7E) >> 1);
        if (HEVC_NAL_SEI_PREFIX == naluType || HEVC_NAL_SEI_SUFFIX == naluType) {
            /* ignore SEI */
        } else if (naluType == HEVC_NAL_VPS || naluType == HEVC_NAL_SPS || naluType == HEVC_NAL_PPS) {
            if (m_cacheBufLen + frameSize > m_cacheBufCap) {
                envir() << "rtsp cache buffer is not enough, realloc from " << m_nReceiveBufferSize;
                reallocBuf(m_frameBuf, frameSize);
                envir() << " to " << m_nReceiveBufferSize << "\n";
            } else {
                memmove(&m_cacheBuf[m_cacheBufLen], m_frameBuf, frameSize);
                m_cacheBufLen += frameSize;
            }
        } else if (naluType >= HEVC_NAL_BLA_W_LP && naluType <= HEVC_NAL_CRA_NUT) {
            if (m_cacheBufLen > 0) {
                memmove(&m_cacheBuf[m_cacheBufLen], m_frameBuf, frameSize);
                m_cacheBufLen += frameSize;
                m_cb.OnRecvFrame(&fSubsession, &m_cacheBuf[0], m_cacheBufLen, m_ePayload, NALU_TYPE_IDR, presentationTime);
                m_cacheBufLen = 0;
                memcpy(m_frameBuf, &NALU_START_CODE[0], NALU_START_CODE_LENGTH);
            } else {
                m_cb.OnRecvFrame(&fSubsession, m_frameBuf, frameSize, m_ePayload, NALU_TYPE_IDR, presentationTime);
            }
        } else {
            m_cb.OnRecvFrame(&fSubsession, m_frameBuf, frameSize, m_ePayload, NALU_TYPE_OTH, presentationTime);
        }
    }

    // Then continue, to request the next frame of data:
    continuePlaying();
}

Boolean DummySink::continuePlaying() {
    if (fSource == NULL) return False;  // sanity check (should not happen)

    // Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
    fSource->getNextFrame(&m_frameBuf[NALU_START_CODE_LENGTH], m_frameBufCap - NALU_START_CODE_LENGTH, afterGettingFrame, this, onSourceClosure, this);
    return True;
}

Boolean DummySink::reallocBuf(unsigned char *buf, unsigned int sz) {
    /* cache capacity size is increased twice size */
    unsigned int cacheBufCap = m_cacheBufCap * 2;
    do {
        if (m_cacheBufLen + sz < cacheBufCap) {
            break;
        }

        cacheBufCap *= 2;

    } while (1);

    /* realloc new buffer */
    unsigned int bufSize = m_frameBufCap + cacheBufCap;
    unsigned char *newBuf = (unsigned char *)malloc(bufSize);
    if (!newBuf) {
        THROW_AX_EXCEPTION("malloc %d fail", bufSize);
        return False;
    }

    /* copy cache buffer */
    memcpy(&newBuf[0], &m_cacheBuf[0], m_cacheBufLen);
    memcpy(&newBuf[m_cacheBufLen], buf, sz);
    m_cacheBufLen += sz;
    /* copy frame buffer */
    memcpy(&newBuf[cacheBufCap], &m_frameBuf[0], m_frameBufCap);

    /* free old buf and swap to new buf */
    free(fReceiveBuffer);

    fReceiveBuffer = newBuf;
    m_nReceiveBufferSize = bufSize;

    m_cacheBufCap = cacheBufCap;
    m_cacheBuf = &fReceiveBuffer[0];
    m_frameBuf = &fReceiveBuffer[m_cacheBufCap];

    return True;
}