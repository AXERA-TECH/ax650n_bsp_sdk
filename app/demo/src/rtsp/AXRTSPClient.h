#pragma once
#include <functional>
#include <unordered_map>
#include "BasicUsageEnvironment.hh"
#include "ax_global_type.h"
#include "liveMedia.hh"
#include "nalu.hpp"
#include "AXEvent.hpp"

#define MAX_TRACK_NUM (5)
typedef struct {
    AX_PAYLOAD_TYPE_E enPayload;
    unsigned int rtpPayload;
    char *control;
    union {
        struct {
            unsigned char *sps[3];
            unsigned int len[3];
        } video;
        struct {
        } audio;
    };
} TRACK_INFO_T;

typedef struct {
    unsigned char *url;
    unsigned char *sdp;
    std::unordered_map<void *, TRACK_INFO_T> tracks;
} TRACKS_INFO_T;

typedef struct axRtspClientCallback {
    std::function<void(const void *, const unsigned char *, unsigned, AX_PAYLOAD_TYPE_E ePayload, STREAM_NALU_TYPE_E eNalu, struct timeval)>
        OnRecvFrame;
    std::function<void(const TRACKS_INFO_T &)> OnTracksInfo;
    std::function<void(void)> OnPreparePlay;
    std::function<void(int, const char *)> OnCheckAlive;

    axRtspClientCallback(void) {
        OnRecvFrame = nullptr;
        OnTracksInfo = nullptr;
        OnPreparePlay = nullptr;
        OnCheckAlive = nullptr;
    }

} RtspClientCallback;

// Define a class to hold per-stream state that we maintain throughout each stream's lifetime:
class StreamClientState {
public:
    StreamClientState();
    virtual ~StreamClientState();

public:
    MediaSubsessionIterator *iter;
    MediaSession *session;
    MediaSubsession *subsession;
    TaskToken streamTimerTask;
    double duration;
    RtspClientCallback cb;
    int bufSize;
    int streamTransportMode = {0}; /* 0: UDP 1: TCP */
    int keepAliveInterval = {10};  /* unit: seconds */
    CAXEvent played;
};

// If you're streaming just a single stream (i.e., just from a single URL, once), then you can define and use just a single
// "StreamClientState" structure, as a global variable in your application.  However, because - in this demo application - we're
// showing how to play multiple streams, concurrently, we can't do that.  Instead, we have to have a separate "StreamClientState"
// structure for each "RTSPClient".  To do this, we subclass "RTSPClient", and add a "StreamClientState" field to the subclass:
class CAXRTSPClient : public RTSPClient {
public:
    static CAXRTSPClient *createNew(UsageEnvironment &env, char const *rtspURL, RtspClientCallback cb, int bufSize, int verbosityLevel = 0,
                                    char const *applicationName = NULL, portNumBits tunnelOverHTTPPortNum = 0);

    void Start(void);

    /*
        client will be deleted automaticaly by continueAfterDESCRIBE() -> shutdownStream()
        Set pointer to nullptr to avoid wild pointer, like:
            CAXRTSPClient *client = CAXRTSPClient::createNew(...);
            client->Stop();
            client = nullptr;
        Also smart pointer can be used like:
            std::unique_ptr<CAXRTSPClient, std::function<void(CAXRTSPClient*)>> p(CAXRTSPClient::createNew(),
                [&](CAXRTSPClient *client { client->Stop(); });
            p->Start();
            p = nullptr;
    */
    void Stop(void);

    AX_BOOL CheckPlayed(int ms) {
        return scs.played.WaitEvent(ms);
    }

protected:
    CAXRTSPClient(UsageEnvironment &env, char const *rtspURL, RtspClientCallback cb, int bufSize, int verbosityLevel,
                  char const *applicationName, portNumBits tunnelOverHTTPPortNum);
    // called only by createNew();
    virtual ~CAXRTSPClient();

public:
    StreamClientState scs;
};

// Define a data sink (a subclass of "MediaSink") to receive the data for each subsession (i.e., each audio or video 'substream').
// In practice, this might be a class (or a chain of classes) that decodes and then renders the incoming audio or video.
// Or it might be a "FileSink", for outputting the received data into a file (as is done by the "openRTSP" application).
// In this example code, however, we define a simple 'dummy' sink that receives incoming data, but does nothing with it.
class DummySink : public MediaSink {
public:
    static DummySink *createNew(UsageEnvironment &env,
                                MediaSubsession &subsession,  // identifies the kind of data that's being received
                                char const *streamId,         // identifies the stream itself (optional)
                                RtspClientCallback cb, unsigned int bufSize);

private:
    DummySink(UsageEnvironment &env, MediaSubsession &subsession, char const *streamId, RtspClientCallback cb, unsigned int bufSize);
    // called only by "createNew()"
    virtual ~DummySink();

    static void afterGettingFrame(void *clientData, unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime,
                                  unsigned durationInMicroseconds);
    void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime,
                           unsigned durationInMicroseconds);

private:
    // redefined virtual functions:
    virtual Boolean continuePlaying();
    Boolean reallocBuf(unsigned char *buf, unsigned int sz);

private:
    /*
        |fReceiveBuffer                                                       |
        |<--                    m_nReceiveBufferSize                       -->|
        |                                                                     |
        |m_cacheBuf                      |m_frameBuf                          |
        |<--     m_cacheBufCap        -->|<--       m_frameBufCap          -->|
        |                                                                     |
        |   cache for SPS PPS VPS        |               I/P frame            |
    */
    unsigned char *fReceiveBuffer;
    unsigned int m_nReceiveBufferSize;
    unsigned int m_frameBufCap;
    unsigned char *m_frameBuf; /* video frame of IPB */
    unsigned char *m_cacheBuf; /* cache SPS, PPS, VPS */
    unsigned int m_cacheBufCap;
    unsigned int m_cacheBufLen;
    MediaSubsession &fSubsession;
    int firstFrame;
    RtspClientCallback m_cb;
    AX_PAYLOAD_TYPE_E m_ePayload;
};
