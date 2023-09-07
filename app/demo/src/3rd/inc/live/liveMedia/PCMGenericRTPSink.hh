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
// "liveMedia"
// Copyright (c) 1996-2020 Live Networks, Inc.  All rights reserved.
// RTP sink for PCM Generic
// C++ header
// support: G711A/G711U/G726/OPUS

#ifndef _PCM_GENERIC_RTP_SINK_HH
#define _PCM_GENERIC_RTP_SINK_HH

#ifndef _MULTI_FRAMED_RTP_SINK_HH
#include "MultiFramedRTPSink.hh"
#endif

class PCMGenericRTPSink: public MultiFramedRTPSink {
public:
  static PCMGenericRTPSink* createNew(UsageEnvironment& env, Groupsock* RTPgs,
		      u_int8_t rtpPayloadFormat,
		      u_int32_t rtpTimestampFrequency = 16000,
		      unsigned numChannels = 1,
		      signed type = 0);

protected:
  PCMGenericRTPSink(UsageEnvironment& env, Groupsock* RTPgs,
			       u_int8_t rtpPayloadFormat,
			       u_int32_t rtpTimestampFrequency,
			       unsigned numChannels,
			       signed type);
	// called only by createNew()

  virtual ~PCMGenericRTPSink();

private: // redefined virtual functions:
  virtual
  Boolean frameCanAppearAfterPacketStart(unsigned char const* frameStart,
                     unsigned numBytesInFrame) const;
  virtual void doSpecialFrameHandling(unsigned fragmentationOffset,
                                      unsigned char* frameStart,
                                      unsigned numBytesInFrame,
                                      struct timeval framePresentationTime,
                                      unsigned numRemainingBytes);

  virtual char const* sdpMediaType() const;

  virtual char const* auxSDPLine(); // for the "a=fmtp:" SDP line

private:
  char const* fSDPMediaTypeString;
  char const* fFmtpSDPLine;
  Boolean fAllowMultipleFramesPerPacket;
  Boolean fSetMBitOnLastFrames, fSetMBitOnNextPacket;
};

#endif
