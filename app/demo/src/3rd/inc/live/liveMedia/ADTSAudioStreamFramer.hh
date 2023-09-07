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
// A filter that reads AAC audio frames, and outputs each frame with
// C++ header

#ifndef _ADTS_AUDIO_STREAM_FRAMER_HH
#define _ADTS_AUDIO_STREAM_FRAMER_HH

#ifndef _FRAMED_FILTER_HH
#include "FramedFilter.hh"
#endif

class ADTSAudioStreamFramer: public FramedFilter {
public:
  static ADTSAudioStreamFramer*
  createNew(UsageEnvironment& env, FramedSource* inputSource, u_int32_t sampleRate=16000);

protected:
  ADTSAudioStreamFramer(UsageEnvironment& env, FramedSource* inputSource, u_int32_t sampleRate);
      // called only by createNew()
  virtual ~ADTSAudioStreamFramer();

protected:
  // redefined virtual functions:
  virtual void doGetNextFrame();

  void setPresentationTime(const struct timeval &presentationTime, const unsigned &durationInMicroseconds);

  void computeNextPresentationTime();

protected:
  static void afterGettingFrame(void* clientData, unsigned frameSize,
                                unsigned numTruncatedBytes,
                                struct timeval presentationTime,
                                unsigned durationInMicroseconds);
  void afterGettingFrame1(unsigned frameSize,
                          unsigned numTruncatedBytes,
                          struct timeval presentationTime,
                          unsigned durationInMicroseconds);

private:
  struct timeval currentFramePlayTime() const;

private:
  struct timeval fPresentationTimeBase;
  struct timeval fNextPresentationTime;
  struct timeval fPresentationTimeBaseSpecified;
  u_int32_t fSampleRate;
};
#endif
