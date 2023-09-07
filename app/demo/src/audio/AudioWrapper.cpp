#include "AudioWrapper.hpp"
#include "AudioWrapperUtils.hpp"
#include "AudioWrapperObserver.hpp"
#include "AudioCap.hpp"
#include "AudioPlay.hpp"
#include "AudioOptionHelper.h"
#include "AppLogApi.h"
#include "IObserver.h"

namespace {
#define AUDIO_WRAPPER "AUDIO_WRAPPER"

static std::mutex audioMutex;

static AX_APP_AUDIO_PLAYFILERESULT_CALLBACK g_audio_play_file_callback;

static CAudioCap* g_pACapInstance = nullptr;
static AX_BOOL g_bACapStart = AX_FALSE;
static CAudioPlay* g_pAPlayInstance = nullptr;
static AX_BOOL g_bAPlayStart = AX_FALSE;

static std::unique_ptr<IObserver> g_ACapRawObs;
static std::vector<std::unique_ptr<IObserver>> g_vecACapEncObs;

#define DECLARE_AUDIOWRAPPER_THREAD_SAFE_API() std::lock_guard<std::mutex> lck(audioMutex)

static AX_S32 audio_capture_attr_parser(const AX_APP_AUDIO_ATTR_PTR pstAttr,
                                                    AUDIO_CAP_DEV_ATTR_T &stACapDevAttr,
                                                    AX_U32 &nACapPipeNum,
                                                    AENC_CONFIG_T stACapPipeAttr[AX_APP_AUDIO_CHAN_BUTT]) {
    nACapPipeNum = 0;

    // capture
    if (pstAttr->stCapAttr.bEnable) {
        AX_U32 nChnCnt = 2;

        // capture pipe
        for (AX_U32 i = 0; i < AX_APP_AUDIO_CHAN_BUTT; i++) {
            if (pstAttr->stCapAttr.stPipeAttr.stAudioChanAttr[i].bEnable) {
                stACapPipeAttr[nACapPipeNum].bLink = AX_TRUE;
                stACapPipeAttr[nACapPipeNum].nChannel = i;
                stACapPipeAttr[nACapPipeNum].nInDepth = AENC_DEFAULT_IN_DEPTH;
                stACapPipeAttr[nACapPipeNum].nOutDepth = AENC_DEFAULT_OUT_DEPTH;

                if (AX_APP_AUDIO_SOUND_MODE_MONO == pstAttr->stCapAttr.stPipeAttr.stAudioChanAttr[i].eSoundMode) {
                    stACapPipeAttr[nACapPipeNum].nChnCnt = 1;
                    nChnCnt = 1;
                }
                else {
                    stACapPipeAttr[nACapPipeNum].nChnCnt = 2;
                }

                stACapPipeAttr[nACapPipeNum].nBitRate = pstAttr->stCapAttr.stPipeAttr.stAudioChanAttr[i].nBitRate;
                stACapPipeAttr[nACapPipeNum].eType = pstAttr->stCapAttr.stPipeAttr.stAudioChanAttr[i].eType;
                stACapPipeAttr[nACapPipeNum].eBitWidth = _AX_BIT_WIDTH(pstAttr->stDevCommAttr.eBitWidth);
                stACapPipeAttr[nACapPipeNum].eSampleRate = _AX_SAMPLE_RATE(pstAttr->stDevCommAttr.eSampleRate);
                stACapPipeAttr[nACapPipeNum].eSoundMode = _AX_SOUND_MODE(pstAttr->stCapAttr.stPipeAttr.stAudioChanAttr[i].eSoundMode);

                if (PT_AAC == stACapPipeAttr[nACapPipeNum].eType) {
                    stACapPipeAttr[nACapPipeNum].stEncoderAttr.stAacEncoder.eAacType = _AX_AAC_TYPE(pstAttr->stCapAttr.stPipeAttr.stAudioChanAttr[i].stAacEncoder.eAacType);
                    stACapPipeAttr[nACapPipeNum].stEncoderAttr.stAacEncoder.eTransType = _AX_AAC_TRANS_TYPE(pstAttr->stCapAttr.stPipeAttr.stAudioChanAttr[i].stAacEncoder.eTransType);
                }
                else {
                    stACapPipeAttr[nACapPipeNum].stEncoderAttr.stDefEncoder.nBitRate = pstAttr->stCapAttr.stPipeAttr.stAudioChanAttr[i].nBitRate;
                    stACapPipeAttr[nACapPipeNum].stEncoderAttr.stDefEncoder.eBitWidth = _AX_BIT_WIDTH(pstAttr->stDevCommAttr.eBitWidth);
                    stACapPipeAttr[nACapPipeNum].stEncoderAttr.stDefEncoder.eSampleRate = _AX_SAMPLE_RATE(pstAttr->stDevCommAttr.eSampleRate);
                }

                nACapPipeNum ++;
            }
        }

        // capture dev
        {
            stACapDevAttr.bLink = AX_TRUE;
            stACapDevAttr.nCardId = pstAttr->stCapAttr.stDevAttr.nCardId;
            stACapDevAttr.nDeviceId = pstAttr->stCapAttr.stDevAttr.nDeviceId;
            stACapDevAttr.nBlkSize = AUDIO_CAP_DEV_DEFAULT_BLK_SIZE;
            stACapDevAttr.nBlkCnt = AUDIO_CAP_DEV_DEFAULT_BLK_CNT;
            stACapDevAttr.nDepth = AUDIO_CAP_DEV_DEFAULT_DEPTH;
            stACapDevAttr.nChnCnt = nChnCnt;
            stACapDevAttr.nPeriodSize = pstAttr->stDevCommAttr.nPeriodSize;
            stACapDevAttr.eBitWidth = _AX_BIT_WIDTH(pstAttr->stDevCommAttr.eBitWidth);
            stACapDevAttr.eSampleRate = _AX_SAMPLE_RATE(pstAttr->stDevCommAttr.eSampleRate);
            if (1 == stACapDevAttr.nChnCnt) {
                stACapDevAttr.eLayoutMode = AX_AI_MIC_REF;
            }
            else {
                stACapDevAttr.eLayoutMode = AX_AI_MIC_MIC;
            }

            if (pstAttr->stDevCommAttr.eSampleRate == AX_APP_AUDIO_SAMPLE_RATE_8000
                || pstAttr->stDevCommAttr.eSampleRate == AX_APP_AUDIO_SAMPLE_RATE_16000) {
                stACapDevAttr.stVqeAttr.s32SampleRate = (AX_S32)_AX_SAMPLE_RATE(pstAttr->stDevCommAttr.eSampleRate);
                stACapDevAttr.stVqeAttr.u32FrameSamples = pstAttr->stDevCommAttr.nPeriodSize;
            }
            else {
                stACapDevAttr.stVqeAttr.s32SampleRate = (AX_S32)AX_AUDIO_SAMPLE_RATE_16000;
                stACapDevAttr.stVqeAttr.u32FrameSamples = stACapDevAttr.stVqeAttr.s32SampleRate / 100;
            }

            // aec
            stACapDevAttr.stVqeAttr.stAecCfg.enAecMode = _AX_AEC_TYPE(pstAttr->stCapAttr.stDevAttr.stVqeAttr.stAecAttr.eType);
            if (AX_APP_AUDIO_AEC_FIXED == pstAttr->stCapAttr.stDevAttr.stVqeAttr.stAecAttr.eType) {
                stACapDevAttr.stVqeAttr.stAecCfg.stAecFixedCfg.eRoutingMode = _AX_ROUTING_MODE(pstAttr->stCapAttr.stDevAttr.stVqeAttr.stAecAttr.stFixedAttr.eMode);
            }
            else if (AX_APP_AUDIO_AEC_FLOAT == pstAttr->stCapAttr.stDevAttr.stVqeAttr.stAecAttr.eType) {
                stACapDevAttr.stVqeAttr.stAecCfg.stAecFloatCfg.enSuppressionLevel = _AX_SUPPRESSION_LEVEL(pstAttr->stCapAttr.stDevAttr.stVqeAttr.stAecAttr.stFloatAttr.eLevel);
            }

            // ns
            stACapDevAttr.stVqeAttr.stNsCfg.bNsEnable = (AX_BOOL)pstAttr->stCapAttr.stDevAttr.stVqeAttr.stAnsAttr.bEnable;
            stACapDevAttr.stVqeAttr.stNsCfg.enAggressivenessLevel = _AX_AGGRESSIVENESS_LEVEL(pstAttr->stCapAttr.stDevAttr.stVqeAttr.stAnsAttr.eLevel);

            // agc
            stACapDevAttr.stVqeAttr.stAgcCfg.bAgcEnable = (AX_BOOL)pstAttr->stCapAttr.stDevAttr.stVqeAttr.stAgcAttr.bEnable;
            stACapDevAttr.stVqeAttr.stAgcCfg.enAgcMode = _AX_AGC_MODE(pstAttr->stCapAttr.stDevAttr.stVqeAttr.stAgcAttr.eAgcMode);
            stACapDevAttr.stVqeAttr.stAgcCfg.s16TargetLevel = (AX_S16)pstAttr->stCapAttr.stDevAttr.stVqeAttr.stAgcAttr.nTargetLv;
            stACapDevAttr.stVqeAttr.stAgcCfg.s16Gain = (AX_S16)pstAttr->stCapAttr.stDevAttr.stVqeAttr.stAgcAttr.nGain;
        }
    }

    return 0;
}

static AX_S32 audio_play_attr_parser(const AX_APP_AUDIO_ATTR_PTR pstAttr,
                                                AUDIO_PLAY_DEV_ATTR_T &stAPlayDevAttr,
                                                AX_U32 &nAPlayPipeNum,
                                                ADEC_CONFIG_T stAPlayPipeAttr[AX_APP_AUDIO_CHAN_BUTT]) {
    nAPlayPipeNum = 0;

    // play
    if (pstAttr->stPlayAttr.bEnable) {
        AX_U32 nChnCnt = 2;

        // play pipe
        for (AX_U32 i = 0; i < AX_APP_AUDIO_CHAN_BUTT; i++) {
            if (pstAttr->stPlayAttr.stPipeAttr.stAudioChanAttr[i].bEnable) {
                stAPlayPipeAttr[nAPlayPipeNum].bLink = AX_TRUE;
                stAPlayPipeAttr[nAPlayPipeNum].nChannel = i;
                stAPlayPipeAttr[nAPlayPipeNum].eType = pstAttr->stPlayAttr.stPipeAttr.stAudioChanAttr[i].eType;
                if (PT_AAC == stAPlayPipeAttr[nAPlayPipeNum].eType) {
                    stAPlayPipeAttr[nAPlayPipeNum].nBlkSize = ADEC_AAC_BLK_SIZE;
                }
                else if (PT_OPUS == stAPlayPipeAttr[nAPlayPipeNum].eType) {
                    stAPlayPipeAttr[nAPlayPipeNum].nBlkSize = ADEC_OPUS_BLK_SIZE;
                }
                else {
                    stAPlayPipeAttr[nAPlayPipeNum].nBlkSize = ADEC_DEFAULT_BLK_SIZE;
                }
                stAPlayPipeAttr[nAPlayPipeNum].nBlkCnt = ADEC_DEFAULT_BLK_CNT;
                stAPlayPipeAttr[nAPlayPipeNum].nInDepth = ADEC_DEFAULT_IN_DEPTH;
                stAPlayPipeAttr[nAPlayPipeNum].nOutDepth = ADEC_DEFAULT_OUT_DEPTH;
                stAPlayPipeAttr[nAPlayPipeNum].eBitWidth = _AX_BIT_WIDTH(pstAttr->stDevCommAttr.eBitWidth);
                stAPlayPipeAttr[nAPlayPipeNum].eSampleRate = _AX_SAMPLE_RATE(pstAttr->stPlayAttr.stPipeAttr.stAudioChanAttr[i].eSampleRate);
                stAPlayPipeAttr[nAPlayPipeNum].eSoundMode = _AX_SOUND_MODE(pstAttr->stPlayAttr.stPipeAttr.stAudioChanAttr[i].eSoundMode);

                if (AX_APP_AUDIO_SOUND_MODE_MONO == pstAttr->stPlayAttr.stPipeAttr.stAudioChanAttr[i].eSoundMode) {
                    nChnCnt = 1;
                }

                if (PT_AAC == stAPlayPipeAttr[nAPlayPipeNum].eType) {
                    stAPlayPipeAttr[nAPlayPipeNum].stDecoderAttr.stAacDecoder.eTransType = _AX_AAC_TRANS_TYPE(pstAttr->stPlayAttr.stPipeAttr.stAudioChanAttr[i].stAacDecoder.eTransType);
                }
                else {
                    stAPlayPipeAttr[nAPlayPipeNum].stDecoderAttr.stDefDecoder.nBitRate = pstAttr->stPlayAttr.stPipeAttr.stAudioChanAttr[i].nBitRate;
                    stAPlayPipeAttr[nAPlayPipeNum].stDecoderAttr.stDefDecoder.eBitWidth = _AX_BIT_WIDTH(pstAttr->stDevCommAttr.eBitWidth);
                    stAPlayPipeAttr[nAPlayPipeNum].stDecoderAttr.stDefDecoder.eSampleRate = _AX_SAMPLE_RATE(pstAttr->stPlayAttr.stPipeAttr.stAudioChanAttr[i].eSampleRate);
                }

                nAPlayPipeNum ++;
            }
        }

        // play dev
        {
            stAPlayDevAttr.bLink = AX_TRUE;
            stAPlayDevAttr.nCardId = pstAttr->stPlayAttr.stDevAttr.nCardId;
            stAPlayDevAttr.nDeviceId = pstAttr->stPlayAttr.stDevAttr.nDeviceId;
            stAPlayDevAttr.nDepth = AUDIO_CAP_DEV_DEFAULT_DEPTH;
            stAPlayDevAttr.nChnCnt = nChnCnt;
            stAPlayDevAttr.nPeriodSize = pstAttr->stDevCommAttr.nPeriodSize;
            stAPlayDevAttr.eBitWidth = _AX_BIT_WIDTH(pstAttr->stDevCommAttr.eBitWidth);
            stAPlayDevAttr.eSampleRate = _AX_SAMPLE_RATE(pstAttr->stDevCommAttr.eSampleRate);

            if (1 == nChnCnt) {
                stAPlayDevAttr.eSoundMode = (AX_AUDIO_SOUND_MODE_E)AX_AUDIO_SOUND_MODE_MONO;
            }
            else {
                stAPlayDevAttr.eSoundMode = (AX_AUDIO_SOUND_MODE_E)AX_AUDIO_SOUND_MODE_STEREO;
            }

            if (pstAttr->stDevCommAttr.eSampleRate == AX_APP_AUDIO_SAMPLE_RATE_8000
                || pstAttr->stDevCommAttr.eSampleRate == AX_APP_AUDIO_SAMPLE_RATE_16000) {
                stAPlayDevAttr.stVqeAttr.s32SampleRate = (AX_S32)_AX_SAMPLE_RATE(pstAttr->stDevCommAttr.eSampleRate);
                stAPlayDevAttr.stVqeAttr.u32FrameSamples = pstAttr->stDevCommAttr.nPeriodSize;
            }
            else {
                stAPlayDevAttr.stVqeAttr.s32SampleRate = (AX_S32)AX_APP_AUDIO_SAMPLE_RATE_16000;
                stAPlayDevAttr.stVqeAttr.u32FrameSamples = stAPlayDevAttr.stVqeAttr.s32SampleRate / 100;
            }

            // ns
            stAPlayDevAttr.stVqeAttr.stNsCfg.bNsEnable = (AX_BOOL)pstAttr->stPlayAttr.stDevAttr.stVqeAttr.stAnsAttr.bEnable;
            stAPlayDevAttr.stVqeAttr.stNsCfg.enAggressivenessLevel = _AX_AGGRESSIVENESS_LEVEL(pstAttr->stPlayAttr.stDevAttr.stVqeAttr.stAnsAttr.eLevel);

            // agc
            stAPlayDevAttr.stVqeAttr.stAgcCfg.bAgcEnable = (AX_BOOL)pstAttr->stPlayAttr.stDevAttr.stVqeAttr.stAgcAttr.bEnable;
            stAPlayDevAttr.stVqeAttr.stAgcCfg.enAgcMode = _AX_AGC_MODE(pstAttr->stPlayAttr.stDevAttr.stVqeAttr.stAgcAttr.eAgcMode);
            stAPlayDevAttr.stVqeAttr.stAgcCfg.s16TargetLevel = (AX_S16)pstAttr->stPlayAttr.stDevAttr.stVqeAttr.stAgcAttr.nTargetLv;
            stAPlayDevAttr.stVqeAttr.stAgcCfg.s16Gain = (AX_S16)pstAttr->stPlayAttr.stDevAttr.stVqeAttr.stAgcAttr.nGain;
        }
    }

    return 0;
}

static AX_S32 audio_capture_start(AX_VOID) {
    if (!g_bACapStart) {
        AX_BOOL bRet = AX_TRUE;

        if (g_pACapInstance) {
            bRet = g_pACapInstance->Start();

            if (bRet) {
                AX_APP_AUDIO_ATTR_T stAudioAttr = APP_AUDIO_ATTR();
                bRet = g_pACapInstance->SetVolume(stAudioAttr.stCapAttr.stDevAttr.fVolume);
            }
        }

        if (!bRet) {
            return -1;
        }

        g_bACapStart = AX_TRUE;
    }

    return 0;
}

AX_S32 audio_capture_stop(AX_VOID) {
    if (g_bACapStart) {
        AX_BOOL bRet = AX_TRUE;

        if (g_pACapInstance) {
            bRet = g_pACapInstance->Stop();
        }

        if (!bRet) {
            return -1;
        }

        g_bACapStart = AX_FALSE;
    }

    return 0;
}

AX_S32 audio_capture_deinit(AX_VOID) {
    if (g_pACapInstance) {
        audio_capture_stop();

        g_pACapInstance->DeInit();
        delete g_pACapInstance;
        g_pACapInstance = nullptr;
    }

    return 0;
}

static AX_S32 audio_capture_init(const AX_APP_AUDIO_ATTR_PTR pstAttr) {
    AX_S32 nRet = 0;

    // audio capture
    if (pstAttr->stCapAttr.bEnable) {
        AUDIO_CAP_DEV_ATTR_T stACapDevAttr;
        AX_U32 nACapPipeNum = 0;
        AENC_CONFIG_T stACapPipeAttr[AX_APP_AUDIO_CHAN_BUTT];

        audio_capture_attr_parser(pstAttr, stACapDevAttr, nACapPipeNum, stACapPipeAttr);

        g_pACapInstance = new CAudioCap();

        if (!g_pACapInstance) {
            nRet = -1;
            goto EXIT;
        }

        if (!g_pACapInstance->Init(&stACapDevAttr, nACapPipeNum, &stACapPipeAttr[0])) {
            LOG_M_E(AUDIO_WRAPPER, "Init audio capture failed.");
            nRet = -1;
            goto EXIT;
        }
    }

EXIT:
    if (0 != nRet) {
        audio_capture_deinit();
    }

    return nRet;
}

static AX_S32 audio_play_start(AX_VOID) {
    if (!g_bAPlayStart) {
        AX_BOOL bRet = AX_TRUE;

        if (g_pAPlayInstance) {
            bRet = g_pAPlayInstance->Start();

            if (bRet) {
                AX_APP_AUDIO_ATTR_T stAudioAttr = APP_AUDIO_ATTR();
                bRet = g_pAPlayInstance->SetVolume(stAudioAttr.stPlayAttr.stDevAttr.fVolume);
            }
        }

        if (!bRet) {
            return -1;
        }

        g_bAPlayStart = AX_TRUE;
    }

    return 0;
}

static AX_S32 audio_play_stop(AX_VOID) {
    if (g_bAPlayStart) {
        AX_BOOL bRet = AX_TRUE;

        if (g_pAPlayInstance) {
            bRet = g_pAPlayInstance->Stop();
        }

        if (!bRet) {
            return -1;
        }

        g_bAPlayStart = AX_FALSE;
    }

    return 0;
}

static AX_S32 audio_play_deinit(AX_VOID) {
    if (g_pAPlayInstance) {
        audio_play_stop();

        g_pAPlayInstance->DeInit();
        delete g_pAPlayInstance;
        g_pAPlayInstance = nullptr;
    }

    return 0;
}

static AX_S32 audio_play_init(const AX_APP_AUDIO_ATTR_PTR pstAttr) {
    AX_S32 nRet = 0;

    // audio play
    if (pstAttr->stPlayAttr.bEnable) {
        AUDIO_PLAY_DEV_ATTR_T stAPlayDevAttr;
        AX_U32 nAPlayPipeNum = 0;
        ADEC_CONFIG_T stAPlayPipeAttr[AX_APP_AUDIO_CHAN_BUTT];

        audio_play_attr_parser(pstAttr, stAPlayDevAttr, nAPlayPipeNum, stAPlayPipeAttr);

        g_pAPlayInstance = new CAudioPlay();

        if (!g_pAPlayInstance) {
            nRet = -1;
            goto EXIT;
        }

        if (!g_pAPlayInstance->Init(&stAPlayDevAttr, nAPlayPipeNum, &stAPlayPipeAttr[0])) {
            LOG_M_E(AUDIO_WRAPPER, "Start audio play failed.");
            nRet = -1;
            goto EXIT;
        }
    }

EXIT:
    if (0 != nRet) {
        audio_play_deinit();
    }

    return nRet;
}

static AX_VOID audio_play_file_callback(AX_U32 nChannel, const AUDIO_FILE_PLAY_RESULT_PTR pstResult) {
    if (g_audio_play_file_callback) {
        AX_APP_AUDIO_PLAY_FILE_RESULT_T stPlayFileResult;

        memset(&stPlayFileResult, 0x00, sizeof(stPlayFileResult));

        if (pstResult) {
            stPlayFileResult.eType = pstResult->eType;
            stPlayFileResult.eStatus = _APP_FILE_PLAY_STATUS(pstResult->eStatus);
            stPlayFileResult.pstrFileName = (AX_CHAR *)pstResult->strFileName.c_str();
            stPlayFileResult.pUserData = (AX_VOID *)pstResult->pUserData;
            stPlayFileResult.pPrivateData = nullptr;
        }

        g_audio_play_file_callback((AX_APP_AUDIO_CHAN_E)nChannel, &stPlayFileResult);
    }
}
}

AX_S32 AX_APP_Audio_Init(const AX_APP_AUDIO_ATTR_PTR pstAttr) {
    DECLARE_AUDIOWRAPPER_THREAD_SAFE_API();

    if (!pstAttr) {
        return -1;
    }

    // audio capture
    AX_S32 nRet = audio_capture_init(pstAttr);

    if (0 != nRet) {
        goto EXIT;
    }

    // audio play
    nRet = audio_play_init(pstAttr);

    if (0 != nRet) {
        audio_capture_deinit();
        goto EXIT;
    }

    SET_APP_AUDIO_ATTR(*pstAttr);

EXIT:
    return nRet;
}

AX_S32 AX_APP_Audio_Deinit(AX_VOID) {
    DECLARE_AUDIOWRAPPER_THREAD_SAFE_API();

    AX_S32 nRet = audio_capture_deinit();

    if (0 != nRet) {
        goto EXIT;
    }

    nRet = audio_play_deinit();

EXIT:
    AX_APP_AUDIO_ATTR_T stAudioAttr;
    memset(&stAudioAttr, 0x00, sizeof(AX_APP_AUDIO_ATTR_T));
    SET_APP_AUDIO_ATTR(stAudioAttr);

    return nRet;
}

AX_S32 AX_APP_Audio_Start(AX_VOID) {
    DECLARE_AUDIOWRAPPER_THREAD_SAFE_API();

    AX_S32 nRet = audio_capture_start();

    if (0 != nRet) {
        goto EXIT;
    }

    nRet = audio_play_start();

EXIT:
    return nRet;
}

AX_S32 AX_APP_Audio_Stop(AX_VOID) {
    DECLARE_AUDIOWRAPPER_THREAD_SAFE_API();

    AX_S32 nRet = audio_capture_stop();

    if (0 != nRet) {
        goto EXIT;
    }

    nRet = audio_play_stop();

EXIT:
    return nRet;
}

AX_S32 AX_APP_Audio_GetAttr(AX_APP_AUDIO_ATTR_PTR pstAttr) {
    DECLARE_AUDIOWRAPPER_THREAD_SAFE_API();

    if (pstAttr) {
        *pstAttr = APP_AUDIO_ATTR();
    }

    return 0;
}

AX_S32 AX_APP_Audio_SetAttr(const AX_APP_AUDIO_ATTR_PTR pstAttr) {
    AX_S32 nRet = 0;

    if (pstAttr) {
        AX_APP_AUDIO_ATTR_T stAudioAttr = APP_AUDIO_ATTR();

        // dev common attr change
        if (pstAttr->stDevCommAttr.eBitWidth != stAudioAttr.stDevCommAttr.eBitWidth
            || pstAttr->stDevCommAttr.eSampleRate != stAudioAttr.stDevCommAttr.eSampleRate
            || pstAttr->stDevCommAttr.nPeriodSize != stAudioAttr.stDevCommAttr.nPeriodSize) {
            AX_APP_Audio_Deinit();

            return AX_APP_Audio_Init(pstAttr);
        }

        DECLARE_AUDIOWRAPPER_THREAD_SAFE_API();

        // audio capture
        AX_BOOL bReContruct = AX_FALSE;

        if (pstAttr->stCapAttr.bEnable != stAudioAttr.stCapAttr.bEnable) {
            bReContruct = AX_TRUE;
        }
        else {
            for (AX_U32 i = 0; i < AX_APP_AUDIO_CHAN_BUTT; i++) {
                if (pstAttr->stCapAttr.stPipeAttr.stAudioChanAttr[i].bEnable != stAudioAttr.stCapAttr.stPipeAttr.stAudioChanAttr[i].bEnable) {
                    bReContruct = AX_TRUE;
                    break;
                }
            }
        }

        if (bReContruct) {
            audio_capture_deinit();

            nRet = audio_capture_init(pstAttr);

            if (0 != nRet) {
                goto EXIT;
            }

            nRet = audio_capture_start();

            if (0 != nRet) {
                goto EXIT;
            }
        }
        else if (pstAttr->stCapAttr.bEnable && g_pACapInstance) {
            AUDIO_CAP_DEV_ATTR_T stACapDevAttr;
            AX_U32 nACapPipeNum = 0;
            AENC_CONFIG_T stACapPipeAttr[AX_APP_AUDIO_CHAN_BUTT];

            audio_capture_attr_parser(pstAttr, stACapDevAttr, nACapPipeNum, stACapPipeAttr);

            g_pACapInstance->SetDevAttr(stACapDevAttr);

            g_pACapInstance->SetVolume((AX_F64)pstAttr->stCapAttr.stDevAttr.fVolume);

            for (AX_U32 i = 0; i < nACapPipeNum; i++) {
                g_pACapInstance->SetAencAttr(stACapPipeAttr[i].nChannel, stACapPipeAttr[i]);
            }
        }

        // audio play
        bReContruct = AX_FALSE;
        if (pstAttr->stPlayAttr.bEnable != stAudioAttr.stPlayAttr.bEnable) {
            bReContruct = AX_TRUE;
        }
        else {
            for (AX_U32 i = 0; i < AX_APP_AUDIO_CHAN_BUTT; i++) {
                if (pstAttr->stPlayAttr.stPipeAttr.stAudioChanAttr[i].bEnable != stAudioAttr.stPlayAttr.stPipeAttr.stAudioChanAttr[i].bEnable) {
                    bReContruct = AX_TRUE;
                    break;
                }
            }
        }

        if (bReContruct) {
            audio_play_deinit();

            nRet = audio_play_init(pstAttr);

            if (0 != nRet) {
                goto EXIT;
            }

            nRet = audio_play_start();

            if (0 != nRet) {
                goto EXIT;
            }
        }
        else if (pstAttr->stPlayAttr.bEnable && g_pAPlayInstance) {
            AUDIO_PLAY_DEV_ATTR_T stAPlayDevAttr;
            AX_U32 nAPlayPipeNum = 0;
            ADEC_CONFIG_T stAPlayPipeAttr[AX_APP_AUDIO_CHAN_BUTT];

            audio_play_attr_parser(pstAttr, stAPlayDevAttr, nAPlayPipeNum, stAPlayPipeAttr);

            g_pAPlayInstance->SetDevAttr(stAPlayDevAttr);

            g_pAPlayInstance->SetVolume((AX_F64)pstAttr->stPlayAttr.stDevAttr.fVolume);

            for (AX_U32 i = 0; i < nAPlayPipeNum; i++) {
                AX_BOOL bRet = g_pAPlayInstance->SetAdecAttr(stAPlayPipeAttr[i].nChannel, stAPlayPipeAttr[i]);

                if (!bRet) {
                    nRet = -1;
                    goto EXIT;
                }
            }
        }

        SET_APP_AUDIO_ATTR(*pstAttr);
    }

EXIT:
    return nRet;
}

AX_S32 AX_APP_Audio_Play(AX_APP_AUDIO_CHAN_E eChan, AX_PAYLOAD_TYPE_E eType, const AX_U8 *pData, AX_U32 nDataSize) {
    DECLARE_AUDIOWRAPPER_THREAD_SAFE_API();

    if (g_pAPlayInstance) {
        ADEC_CONFIG_T stAttr;

        if (g_pAPlayInstance->GetAdecAttr((AX_U32)eChan, stAttr)) {
            if (stAttr.eType != eType) {
                if (eType == PT_AAC) {
                    stAttr.nBlkSize = ADEC_AAC_BLK_SIZE;
                    stAttr.stDecoderAttr.stAacDecoder.eTransType = AX_AAC_TRANS_TYPE_ADTS;
                    stAttr.stDecoderAttr.stAacDecoder.nConfLen = 0;
                }
                else if (eType == PT_OPUS) {
                    stAttr.nBlkSize = ADEC_OPUS_BLK_SIZE;
                }
                else {
                    stAttr.nBlkSize = ADEC_DEFAULT_BLK_SIZE;
                }

                if (stAttr.nBlkCnt < ADEC_DEFAULT_BLK_CNT) {
                    stAttr.nBlkCnt = ADEC_DEFAULT_BLK_CNT;
                }

                stAttr.eType = eType;

                g_pAPlayInstance->SetAdecAttr((AX_U32)eChan, stAttr);
            }
        }

        static AX_U64 u64SeqNum = 0;

        AX_BOOL bRet = g_pAPlayInstance->Play(eType, pData, nDataSize, ++ u64SeqNum, (AX_U32)eChan);

        if (!bRet) {
            return -1;
        }

        return 0;
    }

    return -1;
}

AX_S32 AX_APP_Audio_PlayFile(AX_APP_AUDIO_CHAN_E eChan,
                                        AX_PAYLOAD_TYPE_E eType,
                                        const AX_CHAR* pstrFileName,
                                        AX_S32 nLoop,
                                        AX_APP_AUDIO_PLAYFILERESULT_CALLBACK callback,
                                        AX_VOID *pUserData) {
    DECLARE_AUDIOWRAPPER_THREAD_SAFE_API();

    if (g_pAPlayInstance) {
        ADEC_CONFIG_T stAttr;

        if (g_pAPlayInstance->GetAdecAttr((AX_U32)eChan, stAttr)) {
            if (stAttr.eType != eType) {
                if (eType == PT_AAC) {
                    stAttr.nBlkSize = ADEC_AAC_BLK_SIZE;
                    stAttr.stDecoderAttr.stAacDecoder.eTransType = AX_AAC_TRANS_TYPE_ADTS;
                    stAttr.stDecoderAttr.stAacDecoder.nConfLen = 0;
                }
                else if (eType == PT_OPUS) {
                    stAttr.nBlkSize = ADEC_OPUS_BLK_SIZE;
                }
                else {
                    stAttr.nBlkSize = ADEC_DEFAULT_BLK_SIZE;
                }

                if (stAttr.nBlkCnt < ADEC_DEFAULT_BLK_CNT) {
                    stAttr.nBlkCnt = ADEC_DEFAULT_BLK_CNT;
                }

                stAttr.eType = eType;

                g_pAPlayInstance->SetAdecAttr((AX_U32)eChan, stAttr);
            }
        }

        std::string strFileName = pstrFileName;

        g_audio_play_file_callback = callback;

        AX_BOOL bRet = g_pAPlayInstance->PlayFile(eType, strFileName, nLoop, audio_play_file_callback, pUserData, (AX_U32)eChan);

        if (!bRet) {
            return -1;
        }

        return 0;
    }

    return -1;
}

AX_S32 AX_APP_Audio_StopPlay(AX_APP_AUDIO_CHAN_E eChan) {
    DECLARE_AUDIOWRAPPER_THREAD_SAFE_API();

    if (g_pAPlayInstance) {
        AX_BOOL bRet = g_pAPlayInstance->StopPlayFile((AX_U32)eChan);

        if (!bRet) {
            return -1;
        }

        return 0;
    }

    return -1;
}

AX_S32 AX_APP_Audio_GetCapVolume(AX_F32 *pfVol) {
    DECLARE_AUDIOWRAPPER_THREAD_SAFE_API();

    if (g_pACapInstance && pfVol) {
        AX_F64 fVolume = 0;

        AX_BOOL bRet = g_pACapInstance->GetVolume(fVolume);

        if (!bRet) {
            return -1;
        }

        *pfVol = (AX_F32)fVolume;

        return 0;
    }

    return -1;
}

AX_S32 AX_APP_Audio_SetCapVolume(AX_F32 fVol) {
    DECLARE_AUDIOWRAPPER_THREAD_SAFE_API();

    if (g_pACapInstance) {
        AX_F64 fVolume = (AX_F64)fVol;

        AX_BOOL bRet = g_pACapInstance->SetVolume(fVolume);

        if (!bRet) {
            return -1;
        }

        return 0;
    }

    return -1;
}

AX_S32 AX_APP_Audio_GetPlayVolume(AX_F32 *pfVol) {
    DECLARE_AUDIOWRAPPER_THREAD_SAFE_API();

    if (g_pAPlayInstance && pfVol) {
        AX_F64 fVolume = 0;

        AX_BOOL bRet = g_pAPlayInstance->GetVolume(fVolume);

        if (!bRet) {
            return -1;
        }

        *pfVol = (AX_F32)fVolume;

        return 0;
    }

    return -1;
}

AX_S32 AX_APP_Audio_SetPlayVolume(AX_F32 fVol) {
    DECLARE_AUDIOWRAPPER_THREAD_SAFE_API();

    if (g_pAPlayInstance) {
        AX_F64 fVolume = (AX_F64)fVol;

        AX_BOOL bRet = g_pAPlayInstance->SetVolume(fVolume);

        if (!bRet) {
            return -1;
        }

        return 0;
    }

    return -1;
}

AX_S32 AX_APP_Audio_GetEncoderAttr(AX_APP_AUDIO_CHAN_E eChan, AX_APP_AUDIO_ENCODER_ATTR_PTR pstAttr) {
    // DECLARE_AUDIOWRAPPER_THREAD_SAFE_API();

    if (eChan < AX_APP_AUDIO_CHAN_BUTT
        && pstAttr
        && g_pACapInstance) {
        AENC_CONFIG_T stAttr;
        AX_BOOL bRet = g_pACapInstance->GetAencAttr((AX_U32)eChan, stAttr);

        if (bRet) {
            pstAttr->eType = stAttr.eType;
            pstAttr->nBitRate = stAttr.nBitRate;
            pstAttr->eBitWidth = _APP_BIT_WIDTH(stAttr.eBitWidth);
            pstAttr->eSoundMode = _APP_SOUND_MODE(stAttr.eSoundMode);
            pstAttr->eSampleRate = _APP_SAMPLE_RATE(stAttr.eSampleRate);
            if (PT_AAC == stAttr.eType) {
                pstAttr->nAOT = (AX_S32)stAttr.stEncoderAttr.stAacEncoder.eAacType;
            }
            else {
                pstAttr->nAOT = 0;
            }
        }

        if (!bRet) {
            return -1;
        }

        return 0;
    }

    return -1;
}

AX_S32 AX_APP_Audio_GetPlayPipeAttr(AX_APP_AUDIO_CHAN_E eChan, AX_APP_AUDIO_PLAY_CHAN_ATTR_PTR pstAttr) {
    DECLARE_AUDIOWRAPPER_THREAD_SAFE_API();

    if (eChan < AX_APP_AUDIO_CHAN_BUTT
        && pstAttr
        && g_pAPlayInstance) {
        AX_APP_AUDIO_ATTR_T stAudioAttr = APP_AUDIO_ATTR();

        *pstAttr = stAudioAttr.stPlayAttr.stPipeAttr.stAudioChanAttr[eChan];

        return 0;
    }

    return -1;
}

AX_S32 AX_APP_Audio_SetPlayPipeAttr(AX_APP_AUDIO_CHAN_E eChan, const AX_APP_AUDIO_PLAY_CHAN_ATTR_PTR pstAttr) {
    DECLARE_AUDIOWRAPPER_THREAD_SAFE_API();

    AX_S32 nRet = -1;

    if (eChan < AX_APP_AUDIO_CHAN_BUTT
        && pstAttr
        && g_pAPlayInstance) {
        // audio play
        AX_BOOL bReContruct = AX_FALSE;
        AX_APP_AUDIO_ATTR_T stAudioAttr = APP_AUDIO_ATTR();

        if (pstAttr->bEnable != stAudioAttr.stPlayAttr.stPipeAttr.stAudioChanAttr[eChan].bEnable) {
            bReContruct = AX_TRUE;
        }

        stAudioAttr.stPlayAttr.stPipeAttr.stAudioChanAttr[eChan] = *pstAttr;

        if (bReContruct) {
            audio_play_deinit();

            nRet = audio_play_init(&stAudioAttr);

            if (0 != nRet) {
                goto EXIT;
            }

            nRet = audio_play_start();

            if (0 != nRet) {
                goto EXIT;
            }
        }
        else if (pstAttr->bEnable) {
            AUDIO_PLAY_DEV_ATTR_T stAPlayDevAttr;
            AX_U32 nAPlayPipeNum = 0;
            ADEC_CONFIG_T stAPlayPipeAttr[AX_APP_AUDIO_CHAN_BUTT];

            audio_play_attr_parser(&stAudioAttr, stAPlayDevAttr, nAPlayPipeNum, stAPlayPipeAttr);

            AX_BOOL bRet = g_pAPlayInstance->SetAdecAttr((AX_U32)eChan, stAPlayPipeAttr[eChan]);

            if (!bRet) {
                nRet = -1;
                goto EXIT;
            }
        }

        SET_APP_AUDIO_ATTR(stAudioAttr);

        nRet = 0;
    }

EXIT:
    return nRet;
}

AX_S32 AX_APP_Audio_GetAacEncoderConfigBuf(AX_APP_AUDIO_CHAN_E eChan, const AX_U8 **ppConfBuf, AX_U32 *pDataSize) {
    DECLARE_AUDIOWRAPPER_THREAD_SAFE_API();

    if (eChan < AX_APP_AUDIO_CHAN_BUTT
        && ppConfBuf
        && g_pACapInstance) {
        AX_BOOL bRet = g_pACapInstance->GetAacEncoderConfigBuf((AX_U32)eChan, ppConfBuf, pDataSize);

        if (!bRet) {
            return -1;
        }

        return 0;
    }

    return -1;
}

AX_S32 AX_APP_Audio_SetAacDecoderConfigBuf(AX_APP_AUDIO_CHAN_E eChan, const AX_U8 *pConfBuf, AX_U32 nDataSize) {
    DECLARE_AUDIOWRAPPER_THREAD_SAFE_API();

    if (eChan < AX_APP_AUDIO_CHAN_BUTT
        && pConfBuf
        && g_pAPlayInstance) {
        //TODO
        return -1;
    }

    return -1;
}

AX_S32 AX_APP_Audio_RegisterCapInFrameCallback(const AX_APP_AUDIO_FRAME_CALLBACK callback, AX_VOID *pUserData) {
    DECLARE_AUDIOWRAPPER_THREAD_SAFE_API();

    if (g_pACapInstance) {
        g_ACapRawObs = CObserverMaker::CreateObserver<CAudioWrapperObserver>((AX_VOID *)callback, (AX_VOID *)pUserData);
        g_pACapInstance->RegObserver(AX_AUDIO_CAP_NODE_RAW, 0, g_ACapRawObs.get());

        return 0;
    }

    return -1;
}

AX_S32 AX_APP_Audio_UnRegisterCapInFrameCallback(AX_VOID) {
    DECLARE_AUDIOWRAPPER_THREAD_SAFE_API();

    if (g_pACapInstance) {
        g_pACapInstance->UnregObserver(AX_AUDIO_CAP_NODE_RAW, 0, g_ACapRawObs.get());

        return 0;
    }

    return -1;
}

AX_S32 AX_APP_Audio_RegisterPacketCallback(AX_APP_AUDIO_CHAN_E eChan, const AX_APP_AUDIO_PKT_CALLBACK callback, AX_VOID *pUserData) {
    DECLARE_AUDIOWRAPPER_THREAD_SAFE_API();

    if (g_pACapInstance && eChan < AX_APP_AUDIO_CHAN_BUTT) {
        g_vecACapEncObs.resize(AX_APP_AUDIO_CHAN_BUTT);
        g_vecACapEncObs[eChan] = CObserverMaker::CreateObserver<CAudioWrapperObserver>((AX_VOID *)callback, (AX_VOID *)pUserData);
        g_pACapInstance->RegObserver(AX_AUDIO_CAP_NODE_AENC, eChan, g_vecACapEncObs[eChan].get());

        return 0;
    }

    return -1;
}

AX_S32 AX_APP_Audio_UnRegisterPacketCallback(AX_APP_AUDIO_CHAN_E eChan) {
    DECLARE_AUDIOWRAPPER_THREAD_SAFE_API();

    if (g_pACapInstance && eChan < g_vecACapEncObs.size()) {
        g_pACapInstance->UnregObserver(AX_AUDIO_CAP_NODE_AENC, eChan, g_vecACapEncObs[eChan].get());

        return 0;
    }

    return -1;
}

AX_S32 AX_APP_Audio_RegCapInFrameObserver(AX_VOID* pObserver) {
    DECLARE_AUDIOWRAPPER_THREAD_SAFE_API();

    if (g_pACapInstance) {
        g_pACapInstance->RegObserver(AX_AUDIO_CAP_NODE_RAW, 0, (IObserver *)pObserver);

        return 0;
    }

    return -1;
}

AX_S32 AX_APP_Audio_UnregCapInFrameObserver(AX_VOID* pObserver) {
    DECLARE_AUDIOWRAPPER_THREAD_SAFE_API();

    if (g_pACapInstance) {
        g_pACapInstance->UnregObserver(AX_AUDIO_CAP_NODE_RAW, 0, (IObserver *)pObserver);

        return 0;
    }

    return -1;
}

AX_S32 AX_APP_Audio_RegPacketObserver(AX_APP_AUDIO_CHAN_E eChan, AX_VOID* pObserver) {
    DECLARE_AUDIOWRAPPER_THREAD_SAFE_API();

    if (g_pACapInstance) {
        g_pACapInstance->RegObserver(AX_AUDIO_CAP_NODE_AENC, eChan, (IObserver *)pObserver);

        return 0;
    }

    return -1;
}

AX_S32 AX_APP_Audio_UnregPacketObserver(AX_APP_AUDIO_CHAN_E eChan, AX_VOID* pObserver) {
    DECLARE_AUDIOWRAPPER_THREAD_SAFE_API();

    if (g_pACapInstance) {
        g_pACapInstance->UnregObserver(AX_AUDIO_CAP_NODE_AENC, eChan, (IObserver *)pObserver);

        return 0;
    }

    return -1;
}

AX_S32 AX_APP_Audio_RegisterDetectResultCallback(const AX_APP_AUDIO_DETECTRESULT_CALLBACK callback, AX_VOID *pUserData) {
    // TODO:

    return -1;
}

AX_S32 AX_APP_Audio_UnRegisterDetectResultCallback(AX_VOID) {
    // TODO:

    return -1;
}
