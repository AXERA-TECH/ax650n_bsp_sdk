#pragma once

#include "ax_global_type.h"
#include "AudioCap.hpp"
#include "AudioPlay.hpp"

#ifdef __cplusplus
extern "C" {
#endif

// audio
// audio channel num
typedef enum axAPP_AUDIO_CHAN_E {
    AX_APP_AUDIO_CHAN_0,
    AX_APP_AUDIO_CHAN_1,
    AX_APP_AUDIO_CHAN_2,
    AX_APP_AUDIO_CHAN_3,
    AX_APP_AUDIO_CHAN_4,
    AX_APP_AUDIO_CHAN_5,
    AX_APP_AUDIO_CHAN_6,
    AX_APP_AUDIO_CHAN_7,
    AX_APP_AUDIO_CHAN_BUTT
} AX_APP_AUDIO_CHAN_E;

// audio sound mode
typedef enum axAPP_AUDIO_SOUND_MODE_E {
    AX_APP_AUDIO_SOUND_MODE_MONO = 0,         /*mono*/
    AX_APP_AUDIO_SOUND_MODE_STEREO = 1,       /*stereo*/
    AX_APP_AUDIO_SOUND_MODE_BUTT
} AX_APP_AUDIO_SOUND_MODE_E;

// audio layout mode
typedef enum axAPP_AUDIO_LAYOUT_MODE_E {
    AX_APP_AUDIO_LAYOUT_MIC_MIC = 0,
    AX_APP_AUDIO_LAYOUT_MIC_REF = 1,
    AX_APP_AUDIO_LAYOUT_REF_MIC = 2,
    AX_APP_AUDIO_LAYOUT_MODE_BUTT
} AX_APP_AUDIO_LAYOUT_MODE_E;

// audio bit width
typedef enum axAPP_AUDIO_BIT_WIDTH_E {
    AX_APP_AUDIO_BIT_WIDTH_8  = 0,   /* 8bit width */
    AX_APP_AUDIO_BIT_WIDTH_16 = 1,   /* 16bit width*/
    AX_APP_AUDIO_BIT_WIDTH_24 = 2,   /* 24bit width*/
    AX_APP_AUDIO_BIT_WIDTH_32 = 3,   /* 32bit width*/
    AX_APP_AUDIO_BIT_WIDTH_BUTT,
} AX_APP_AUDIO_BIT_WIDTH_E;

// audio sample rate
typedef enum axAPP_AUDIO_SAMPLE_RATE_E {
    AX_APP_AUDIO_SAMPLE_RATE_8000  = 8000,    /* 8K samplerate*/
    AX_APP_AUDIO_SAMPLE_RATE_12000 = 12000,   /* 12K samplerate*/
    AX_APP_AUDIO_SAMPLE_RATE_11025 = 11025,   /* 11.025K samplerate*/
    AX_APP_AUDIO_SAMPLE_RATE_16000 = 16000,   /* 16K samplerate*/
    AX_APP_AUDIO_SAMPLE_RATE_22050 = 22050,   /* 22.050K samplerate*/
    AX_APP_AUDIO_SAMPLE_RATE_24000 = 24000,   /* 24K samplerate*/
    AX_APP_AUDIO_SAMPLE_RATE_32000 = 32000,   /* 32K samplerate*/
    AX_APP_AUDIO_SAMPLE_RATE_44100 = 44100,   /* 44.1K samplerate*/
    AX_APP_AUDIO_SAMPLE_RATE_48000 = 48000,   /* 48K samplerate*/
    AX_APP_AUDIO_SAMPLE_RATE_64000 = 64000,   /* 64K samplerate*/
    AX_APP_AUDIO_SAMPLE_RATE_96000 = 96000   /* 96K samplerate*/
} AX_APP_AUDIO_SAMPLE_RATE_E;

typedef enum axAPP_AUDIO_AGC_MODE_E {
    // Adaptive mode intended for use if an analog volume control is available
    // on the capture device. It will require the user to provide coupling
    // between the OS mixer controls and AGC through the |stream_analog_level()|
    // functions.
    //
    // It consists of an analog gain prescription for the audio device and a
    // digital compression stage.
    AX_APP_AUDIO_AGC_MODE_ADAPTIVE_ANALOG = 0,

    // Adaptive mode intended for situations in which an analog volume control
    // is unavailable. It operates in a similar fashion to the adaptive analog
    // mode, but with scaling instead applied in the digital domain. As with
    // the analog mode, it additionally uses a digital compression stage.
    AX_APP_AUDIO_AGC_MODE_ADAPTIVE_DIGITAL,

    // Fixed mode which enables only the digital compression stage also used by
    // the two adaptive modes.
    //
    // It is distinguished from the adaptive modes by considering only a
    // short time-window of the input signal. It applies a fixed gain through
    // most of the input level range, and compresses (gradually reduces gain
    // with increasing level) the input signal at higher levels. This mode is
    // preferred on embedded devices where the capture signal level is
    // predictable, so that a known gain can be applied.
    AX_APP_AUDIO_AGC_MODE_FIXED_DIGITAL,

    AX_APP_AUDIO_AGC_MODE_BUTT
} AX_APP_AUDIO_AGC_MODE_E;

// audio noise suppression aggressiveness type
typedef enum axAPP_AUDIO_ANS_AGGRESSIVENESS_LEVEL_E {
    AX_APP_AUDIO_ANS_AGGRESSIVENESS_LEVEL_LOW = 0,
    AX_APP_AUDIO_ANS_AGGRESSIVENESS_LEVEL_MODERATE,
    AX_APP_AUDIO_ANS_AGGRESSIVENESS_LEVEL_HIGH,
    AX_APP_AUDIO_ANS_AGGRESSIVENESS_LEVEL_VERYHIGH,
    AX_APP_AUDIO_ANS_AGGRESSIVENESS_LEVEL_BUTT
} AX_APP_AUDIO_ANS_AGGRESSIVENESS_LEVEL_E;

// audio Acoustic Echo Canceller type
typedef enum axAPP_AUDIO_AEC_TYPE_E {
    AX_APP_AUDIO_AEC_DISABLE,
    AX_APP_AUDIO_AEC_FLOAT,
    AX_APP_AUDIO_AEC_FIXED,
    AX_APP_AUDIO_AEC_TYPE_BUTT
} AX_APP_AUDIO_AEC_TYPE_E;

// audio Acoustic Echo Canceller routing mode
typedef enum axAPP_AUDIO_AEC_ROUTING_MODE_E {
    AX_APP_AUDIO_AEC_QUITE_EARPIECE_OR_HEADSET,
    AX_APP_AUDIO_AEC_EARPIECE,
    AX_APP_AUDIO_AEC_LOUD_EARPIECE,
    AX_APP_AUDIO_AEC_SPEAKERPHONE,
    AX_APP_AUDIO_AEC_LOUD_SPEAKERPHONE,
    AX_APP_AUDIO_AEC_ROUTING_MODE_BUTT
} AX_APP_AUDIO_AEC_ROUTING_MODE_E;

// audio aec suppression level
typedef enum axAPP_AUDIO_AEC_SUPPRESSION_LEVEL_E {
    AX_APP_AUDIO_AEC_SUPPRESSION_LEVEL_LOW,
    AX_APP_AUDIO_AEC_SUPPRESSION_LEVEL_MODERATE,
    AX_APP_AUDIO_AEC_SUPPRESSION_LEVEL_HIGH,
    AX_APP_AUDIO_AEC_SUPPRESSION_LEVEL_BUTT
} AX_APP_AUDIO_AEC_SUPPRESSION_LEVEL_E;

// audio aac type
typedef enum axAPP_AUDIO_AAC_TYPE_E {
    AX_APP_AUDIO_AAC_TYPE_NONE = -1,
    AX_APP_AUDIO_AAC_TYPE_NULL_OBJECT = 0,
    AX_APP_AUDIO_AAC_TYPE_AAC_LC = 2,            /* Low Complexity object                     */
    AX_APP_AUDIO_AAC_TYPE_ER_AAC_LD = 23,        /* Error Resilient(ER) AAC LowDelay object   */
    AX_APP_AUDIO_AAC_TYPE_ER_AAC_ELD = 39,       /* AAC Enhanced Low Delay                    */
    AX_APP_AUDIO_AAC_TYPE_BUTT,
} AX_APP_AUDIO_AAC_TYPE_E;

// audio aac channel mode
typedef enum axAPP_AUDIO_AAC_CHAN_MODE_E {
    AX_APP_AUDIO_AAC_CHAN_MODE_INVALID = -1,
    AX_APP_AUDIO_AAC_CHAN_MODE_UNKNOWN = 0,
    AX_APP_AUDIO_AAC_CHAN_MODE_1 = 1,         /**< C */
    AX_APP_AUDIO_AAC_CHAN_MODE_2 = 2,         /**< L+R */
    AX_APP_AUDIO_AAC_CHAN_MODE_1_2 = 3,       /**< C, L+R */
    AX_APP_AUDIO_AAC_CHAN_MODE_1_2_1 = 4,     /**< C, L+R, Rear */
    AX_APP_AUDIO_AAC_CHAN_MODE_1_2_2 = 5,     /**< C, L+R, LS+RS */
    AX_APP_AUDIO_AAC_CHAN_MODE_1_2_2_1 = 6,   /**< C, L+R, LS+RS, LFE */
    AX_APP_AUDIO_AAC_CHAN_MODE_BUTT,
} AX_APP_AUDIO_AAC_CHAN_MODE_E;

// audio aac transport type
typedef enum axAPP_AUDIO_AAC_TRANS_TYPE_E {
    AX_APP_AUDIO_AAC_TRANS_TYPE_UNKNOWN = -1,    /* Unknown format.            */
    AX_APP_AUDIO_AAC_TRANS_TYPE_RAW = 0,         /* "as is" access units (packet based since there is obviously no sync layer) */
    AX_APP_AUDIO_AAC_TRANS_TYPE_ADTS = 2,        /* ADTS bitstream format.     */
    AX_APP_AUDIO_AAC_TRANS_TYPE_BUTT,
} AX_APP_AUDIO_AAC_TRANS_TYPE_E;

// audio file play status
typedef enum axAPP_AUDIO_PLAY_FILE_STATUS_E {
    AX_APP_AUDIO_PLAY_FILE_STATUS_COMPLETE,
    AX_APP_AUDIO_PLAY_FILE_STATUS_STOP,
    AX_APP_AUDIO_PLAY_FILE_STATUS_ERROR,
    AX_APP_AUDIO_PLAY_FILE_STATUS_BUTT
} AX_APP_AUDIO_PLAY_FILE_STATUS_E;

// audio aac encoder attribute
typedef struct axAPP_AUDIO_AAC_ENCODER_ATTR_T {
    AX_APP_AUDIO_AAC_TYPE_E eAacType;
    AX_APP_AUDIO_AAC_TRANS_TYPE_E eTransType;
} AX_APP_AUDIO_AAC_ENCODER_ATTR_T, *AX_APP_AUDIO_AAC_ENCODER_ATTR_PTR;

// audio aac decoder attribute
typedef struct axAPP_AUDIO_AAC_DECODER_ATTR_T {
    AX_APP_AUDIO_AAC_TRANS_TYPE_E eTransType;
} AX_APP_AUDIO_AAC_DECODER_ATTR_T, *AX_APP_AUDIO_AAC_DECODER_ATTR_PTR;

// audio default encoder attribute
typedef struct axAPP_AUDIO_DEF_ENCODER_ATTR_T {
    AX_U8 nReserved;
} AX_APP_AUDIO_DEF_ENCODER_ATTR_T, *AX_APP_AUDIO_DEF_ENCODER_ATTR_PTR;

// audio default decoder attribute
typedef struct axAPP_AUDIO_DEF_DECODER_ATTR_T {
    AX_U8 nReserved;
} AX_APP_AUDIO_DEF_DECODER_ATTR_T, *AX_APP_AUDIO_DEF_DECODER_ATTR_PTR;

// audio automatic gain control attribute
typedef struct axAPP_AUDIO_AGC_ATTR_T {
    AX_BOOL bEnable;
    AX_APP_AUDIO_AGC_MODE_E eAgcMode; /* only support AX_APP_AUDIO_AGC_MODE_FIXED_DIGITAL */
    AX_S16 nTargetLv; /* [-31 - 0], default is -3 */
    AX_S16 nGain; /*0 ~ 90 default 9*/
} AX_APP_AUDIO_AGC_ATTR_T, *AX_APP_AUDIO_AGC_ATTR_PTR;

// audio noise suppression attribute
typedef struct axAPP_AUDIO_ANS_ATTR_T {
    AX_BOOL bEnable;
    AX_APP_AUDIO_ANS_AGGRESSIVENESS_LEVEL_E eLevel;
} AX_APP_AUDIO_ANS_ATTR_T, *AX_APP_AUDIO_ANS_ATTR_PTR;

// audio aec float attribute
typedef struct axAPP_AUDIO_AEC_FLOAT_ATTR_T {
    AX_APP_AUDIO_AEC_SUPPRESSION_LEVEL_E eLevel;
} AX_APP_AUDIO_AEC_FLOAT_ATTR_T, *AX_APP_AUDIO_AEC_FLOAT_ATTR_PTR;

// audio aec fixed attribute
typedef struct axAPP_AUDIO_AEC_FIXED_ATTR_T {
    AX_APP_AUDIO_AEC_ROUTING_MODE_E eMode;
} AX_APP_AUDIO_AEC_FIXED_ATTR_T, *AX_APP_AUDIO_AEC_FIXED_ATTR_PTR;

// audio acoustic echo canceller attribute
typedef struct axAPP_AUDIO_AEC_ATTR_T {
    AX_APP_AUDIO_AEC_TYPE_E eType;
    union {
        AX_APP_AUDIO_AEC_FLOAT_ATTR_T stFloatAttr;
        AX_APP_AUDIO_AEC_FIXED_ATTR_T stFixedAttr;
    };
} AX_APP_AUDIO_AEC_ATTR_T, *AX_APP_AUDIO_AEC_ATTR_PTR;

// audio capture vqe attribute
typedef struct axAPP_AUDIO_CAP_VQE_ATTR_T {
    AX_APP_AUDIO_AEC_ATTR_T stAecAttr;
    AX_APP_AUDIO_ANS_ATTR_T stAnsAttr;
    AX_APP_AUDIO_AGC_ATTR_T stAgcAttr;
} AX_APP_AUDIO_CAP_VQE_ATTR_T, *AX_APP_AUDIO_CAP_VQE_ATTR_PTR;

// audio play vqe attribute
typedef struct axAPP_AUDIO_PLAY_VQE_ATTR_T {
    AX_APP_AUDIO_ANS_ATTR_T stAnsAttr;
    AX_APP_AUDIO_AGC_ATTR_T stAgcAttr;
} AX_APP_AUDIO_PLAY_VQE_ATTR_T, *AX_APP_AUDIO_PLAY_VQE_ATTR_PTR;

// audio encoder attribute
typedef struct axAPP_AUDIO_ENCODER_ATTR_T {
    AX_S32 nAOT;
    AX_U32 nBitRate;
    AX_PAYLOAD_TYPE_E eType;
    AX_APP_AUDIO_BIT_WIDTH_E eBitWidth;
    AX_APP_AUDIO_SOUND_MODE_E eSoundMode;
    AX_APP_AUDIO_SAMPLE_RATE_E eSampleRate;
} AX_APP_AUDIO_ENCODER_ATTR_T, *AX_APP_AUDIO_ENCODER_ATTR_PTR;

// audio capture dev attribute
typedef struct axAPP_AUDIO_CAP_DEV_ATTR_T {
    AX_U32 nCardId;
    AX_U32 nDeviceId;
    AX_F32 fVolume;
    AX_APP_AUDIO_CAP_VQE_ATTR_T stVqeAttr;
} AX_APP_AUDIO_CAP_DEV_ATTR_T, *AX_APP_AUDIO_CAP_DEV_ATTR_PTR;

// audio capture channel attribute
typedef struct axAPP_AUDIO_CAP_CHAN_ATTR_T {
    AX_BOOL bEnable;
    AX_U32 nBitRate;
    AX_APP_AUDIO_SOUND_MODE_E eSoundMode;
    AX_PAYLOAD_TYPE_E eType;
    union {
        AX_APP_AUDIO_DEF_ENCODER_ATTR_T stDefEncoder;
        AX_APP_AUDIO_AAC_ENCODER_ATTR_T stAacEncoder;
    };
} AX_APP_AUDIO_CAP_CHAN_ATTR_T, *AX_APP_AUDIO_CAP_CHAN_ATTR_PTR;

// audio capture pipe attribute
typedef struct axAPP_AUDIO_CAP_PIPE_ATTR_T {
    AX_APP_AUDIO_CAP_CHAN_ATTR_T stAudioChanAttr[AX_APP_AUDIO_CHAN_BUTT];
} AX_APP_AUDIO_CAP_PIPE_ATTR_T, *AX_APP_AUDIO_CAP_PIPE_ATTR_PTR;

typedef struct axAPP_AUDIO_CAP_ATTR_T {
    AX_BOOL bEnable;
    AX_APP_AUDIO_CAP_DEV_ATTR_T stDevAttr;
    AX_APP_AUDIO_CAP_PIPE_ATTR_T stPipeAttr;
} AX_APP_AUDIO_CAP_ATTR_T, *AX_APP_AUDIO_CAP_ATTR_PTR;

// audio play dev attribute
typedef struct axAPP_AUDIO_PLAY_DEV_ATTR_T {
    AX_U32 nCardId;
    AX_U32 nDeviceId;
    AX_F32 fVolume;
    AX_APP_AUDIO_PLAY_VQE_ATTR_T stVqeAttr;
} AX_APP_AUDIO_PLAY_DEV_ATTR_T, *AX_APP_AUDIO_PLAY_DEV_ATTR_PTR;

// audio play channel attribute
typedef struct axAPP_AUDIO_PLAY_CHAN_ATTR_T {
    AX_BOOL bEnable;
    AX_U32 nBitRate;
    AX_APP_AUDIO_SAMPLE_RATE_E eSampleRate;
    AX_APP_AUDIO_SOUND_MODE_E eSoundMode;
    AX_PAYLOAD_TYPE_E eType;
    union {
        AX_APP_AUDIO_DEF_DECODER_ATTR_T stDefDecoder;
        AX_APP_AUDIO_AAC_DECODER_ATTR_T stAacDecoder;
    };
} AX_APP_AUDIO_PLAY_CHAN_ATTR_T, *AX_APP_AUDIO_PLAY_CHAN_ATTR_PTR;

// audio play pipe attribute
typedef struct axAPP_AUDIO_PLAY_PIPE_ATTR_T {
    AX_APP_AUDIO_PLAY_CHAN_ATTR_T stAudioChanAttr[AX_APP_AUDIO_CHAN_BUTT];
} AX_APP_AUDIO_PLAY_PIPE_ATTR_T, *AX_APP_AUDIO_PLAY_PIPE_ATTR_PTR;

// audio play attribute
typedef struct axAPP_AUDIO_PLAY_ATTR_T {
    AX_BOOL bEnable;
    AX_APP_AUDIO_PLAY_DEV_ATTR_T stDevAttr;
    AX_APP_AUDIO_PLAY_PIPE_ATTR_T stPipeAttr;
} AX_APP_AUDIO_PLAY_ATTR_T, *AX_APP_AUDIO_PLAY_ATTR_PTR;

// audio dev common attribute
typedef struct axAPP_AUDIO_DEV_COMM_ATTR_T {
    AX_APP_AUDIO_BIT_WIDTH_E eBitWidth;
    AX_APP_AUDIO_SAMPLE_RATE_E eSampleRate;
    AX_U32 nPeriodSize;
} AX_APP_AUDIO_DEV_COMM_ATTR_T, *AX_APP_AUDIO_DEV_COMM_ATTR_PTR;

// audio attribute
typedef struct axAPP_AUDIO_ATTR_T {
    AX_APP_AUDIO_DEV_COMM_ATTR_T stDevCommAttr;
    AX_APP_AUDIO_CAP_ATTR_T stCapAttr;
    AX_APP_AUDIO_PLAY_ATTR_T stPlayAttr;
} AX_APP_AUDIO_ATTR_T, *AX_APP_AUDIO_ATTR_PTR;

// audio
// audio packet
typedef struct axAPP_AUDIO_PKT_T {
    AX_U32 nBitRate;
    AX_PAYLOAD_TYPE_E eType;
    AX_APP_AUDIO_BIT_WIDTH_E eBitWidth;
    AX_APP_AUDIO_SAMPLE_RATE_E eSampleRate;
    AX_APP_AUDIO_SOUND_MODE_E eSoundMode;
    AX_U8 *pData;
    AX_U32 nDataSize;
    AX_U64 u64Pts;
    AX_VOID *pUserData;
    AX_VOID *pPrivateData;
} AX_APP_AUDIO_PKT_T, *AX_APP_AUDIO_PKT_PTR;

// audio frame
typedef struct axAPP_AUDIO_FRAME_T {
    AX_PAYLOAD_TYPE_E eType;
    AX_APP_AUDIO_BIT_WIDTH_E eBitWidth;
    AX_APP_AUDIO_SAMPLE_RATE_E eSampleRate;
    AX_U32 nChnCnt;
    AX_U8 *pData;
    AX_U32 nDataSize;
    AX_U64 u64Pts;
    AX_VOID *pUserData;
    AX_VOID *pPrivateData;
} AX_APP_AUDIO_FRAME_T, *AX_APP_AUDIO_FRAME_PTR;

// audio file paly result
typedef struct axAPP_AUDIO_PLAY_FILE_RESULT_T {
    AX_PAYLOAD_TYPE_E eType;
    AX_APP_AUDIO_PLAY_FILE_STATUS_E eStatus;
    AX_CHAR *pstrFileName;
    AX_VOID *pUserData;
    AX_VOID *pPrivateData;
} AX_APP_AUDIO_PLAY_FILE_RESULT_T, *AX_APP_AUDIO_PLAY_FILE_RESULT_PTR;

// audio detect result
typedef struct axAPP_AUDIO_DETECT_RESULT_T {
    AX_F32 fValue;
    AX_VOID *pUserData;
    AX_VOID *pPrivateData;
} AX_APP_AUDIO_DETECT_RESULT_T, *AX_APP_AUDIO_DETECT_RESULT_PTR;

// audio
typedef AX_VOID (*AX_APP_AUDIO_FRAME_CALLBACK)(const AX_APP_AUDIO_FRAME_PTR pstFrame);
typedef AX_VOID (*AX_APP_AUDIO_PKT_CALLBACK)(AX_APP_AUDIO_CHAN_E eChan, const AX_APP_AUDIO_PKT_PTR pstPkt);
typedef AX_VOID (*AX_APP_AUDIO_PLAYFILERESULT_CALLBACK)(AX_APP_AUDIO_CHAN_E eChan, const AX_APP_AUDIO_PLAY_FILE_RESULT_PTR pstResult);
typedef AX_VOID (*AX_APP_AUDIO_DETECTRESULT_CALLBACK)(const AX_APP_AUDIO_DETECT_RESULT_PTR pstResult);

//////////////////////////////////////////////////////////////////////////////////////
/// @brief audio init
///
/// @param pstAttr      [O]: audio attribute
///
/// @return 0 if success, otherwise failure
//////////////////////////////////////////////////////////////////////////////////////
AX_S32 AX_APP_Audio_Init(const AX_APP_AUDIO_ATTR_PTR pstAttr);

//////////////////////////////////////////////////////////////////////////////////////
/// @brief audio deinit
///
/// @param NA
///
/// @return 0 if success, otherwise failure
//////////////////////////////////////////////////////////////////////////////////////
AX_S32 AX_APP_Audio_Deinit(AX_VOID);

//////////////////////////////////////////////////////////////////////////////////////
/// @brief audio start
///
/// @param NA
///
/// @return 0 if success, otherwise failure
//////////////////////////////////////////////////////////////////////////////////////
AX_S32 AX_APP_Audio_Start(AX_VOID);

//////////////////////////////////////////////////////////////////////////////////////
/// @brief audio stop
///
/// @param NA
///
/// @return 0 if success, otherwise failure
//////////////////////////////////////////////////////////////////////////////////////
AX_S32 AX_APP_Audio_Stop(AX_VOID);

//////////////////////////////////////////////////////////////////////////////////////
/// @brief audio get attribute
///
/// @param pstAttr      [O]: audio attribute
///
/// @return 0 if success, otherwise failure
//////////////////////////////////////////////////////////////////////////////////////
AX_S32 AX_APP_Audio_GetAttr(AX_APP_AUDIO_ATTR_PTR pstAttr);

//////////////////////////////////////////////////////////////////////////////////////
/// @brief audio set attribute
///
/// @param pstAttr      [I]: audio attribute
///
/// @return 0 if success, otherwise failure
//////////////////////////////////////////////////////////////////////////////////////
AX_S32 AX_APP_Audio_SetAttr(const AX_APP_AUDIO_ATTR_PTR pstAttr);

//////////////////////////////////////////////////////////////////////////////////////
/// @brief audio buffer play
///
/// @param eChan        [I]: audio channel
///        audio type   [I]: audio type
///        pData        [I]: audio data
///        nDataSize    [I]: audio data size
///        eChan        [I]: audio channel
///
/// @return 0 if success, otherwise failure
AX_S32 AX_APP_Audio_Play(AX_APP_AUDIO_CHAN_E eChan, AX_PAYLOAD_TYPE_E eType, const AX_U8 *pData, AX_U32 nDataSize);

//////////////////////////////////////////////////////////////////////////////////////
/// @brief audio file play
///
/// @param eChan        [I]: audio channel
///        eType        [I]: audio type
///        pstrFileName [I]: audio filename
///        nLoop        [I]: audio play times
///        callback     [I]: audio play file callback
///        pUserData    [I]: user data
///
/// @return 0 if success, otherwise failure
AX_S32 AX_APP_Audio_PlayFile(AX_APP_AUDIO_CHAN_E eChan,
                                        AX_PAYLOAD_TYPE_E eType,
                                        const AX_CHAR* pstrFileName,
                                        AX_S32 nLoop,
                                        AX_APP_AUDIO_PLAYFILERESULT_CALLBACK callback,
                                        AX_VOID *pUserData);

//////////////////////////////////////////////////////////////////////////////////////
/// @brief audio play stop play
///
/// @param eChan        [I]: audio channel
///
/// @return 0 if success, otherwise failure
//////////////////////////////////////////////////////////////////////////////////////
AX_S32 AX_APP_Audio_StopPlay(AX_APP_AUDIO_CHAN_E eChan);

//////////////////////////////////////////////////////////////////////////////////////
/// @brief audio get capture volume
///
/// @param pfVol        [O]: audio capture volume
///
/// @return 0 if success, otherwise failure
//////////////////////////////////////////////////////////////////////////////////////
AX_S32 AX_APP_Audio_GetCapVolume(AX_F32 *pfVol);

//////////////////////////////////////////////////////////////////////////////////////
/// @brief audio set capture volume
///
/// @param fVol         [I]: audio capture volume
///
/// @return 0 if success, otherwise failure
//////////////////////////////////////////////////////////////////////////////////////
AX_S32 AX_APP_Audio_SetCapVolume(AX_F32 fVol);

//////////////////////////////////////////////////////////////////////////////////////
/// @brief audio get play volume
///
/// @param pfVol        [O]: audio play volume
///
/// @return 0 if success, otherwise failure
//////////////////////////////////////////////////////////////////////////////////////
AX_S32 AX_APP_Audio_GetPlayVolume(AX_F32 *pfVol);

//////////////////////////////////////////////////////////////////////////////////////
/// @brief audio set Play volume
///
/// @param fVol         [I]: audio play volume
///
/// @return 0 if success, otherwise failure
//////////////////////////////////////////////////////////////////////////////////////
AX_S32 AX_APP_Audio_SetPlayVolume(AX_F32 fVol);

//////////////////////////////////////////////////////////////////////////////////////
/// @brief audio get capture pipe encoder attribute
///
/// @param eChan        [I]: audio capture pipe channel
///        pstAttr      [O]: audio capture pipe encoder attribute
///
/// @return 0 if success, otherwise failure
//////////////////////////////////////////////////////////////////////////////////////
AX_S32 AX_APP_Audio_GetEncoderAttr(AX_APP_AUDIO_CHAN_E eChan, AX_APP_AUDIO_ENCODER_ATTR_PTR pstAttr);

//////////////////////////////////////////////////////////////////////////////////////
/// @brief audio get play pipe attribute
///
/// @param eChan        [I]: audio play pipe channel
///        pstAttr      [O]: audio play pipe attribute
///
/// @return 0 if success, otherwise failure
//////////////////////////////////////////////////////////////////////////////////////
AX_S32 AX_APP_Audio_GetPlayPipeAttr(AX_APP_AUDIO_CHAN_E eChan, AX_APP_AUDIO_PLAY_CHAN_ATTR_PTR pstAttr);

//////////////////////////////////////////////////////////////////////////////////////
/// @brief audio set play pipe attribute
///
/// @param eChan        [I]: audio play pipe channel
///        pstAttr      [I]: audio play pipe attribute
///
/// @return 0 if success, otherwise failure
//////////////////////////////////////////////////////////////////////////////////////
AX_S32 AX_APP_Audio_SetPlayPipeAttr(AX_APP_AUDIO_CHAN_E eChan, const AX_APP_AUDIO_PLAY_CHAN_ATTR_PTR pstAttr);

//////////////////////////////////////////////////////////////////////////////////////
/// @brief audio get aac encoder config buffer
///
/// @param ppConfBuf    [O]: audio aac encoder config buffer
///        pDataSize    [I]: audio encoder config buffer size
///
/// @return 0 if success, otherwise failure
//////////////////////////////////////////////////////////////////////////////////////
AX_S32 AX_APP_Audio_GetAacEncoderConfigBuf(AX_APP_AUDIO_CHAN_E eChan, const AX_U8 **ppConfBuf, AX_U32 *pDataSize);

//////////////////////////////////////////////////////////////////////////////////////
/// @brief audio get aac encoder config buffer
///
/// @param pConfBuf     [I]: audio encoder config buffer
///        nDataSize    [I]: audio encoder config buffer size
///
/// @return 0 if success, otherwise failure
//////////////////////////////////////////////////////////////////////////////////////
AX_S32 AX_APP_Audio_SetAacDecoderConfigBuf(AX_APP_AUDIO_CHAN_E eChan, const AX_U8 *pConfBuf, AX_U32 nDataSize);

//////////////////////////////////////////////////////////////////////////////////////
/// @brief register audio catpure in frame callback
///
/// @param callback     [I]: audio capture in frame callback
///        pUserData    [I]: user data
///
/// @return 0 if success, otherwise failure
//////////////////////////////////////////////////////////////////////////////////////
AX_S32 AX_APP_Audio_RegisterCapInFrameCallback(const AX_APP_AUDIO_FRAME_CALLBACK callback, AX_VOID *pUserData);

//////////////////////////////////////////////////////////////////////////////////////
/// @brief unregister audio catpure in frame callback
///
/// @param NA
///
/// @return 0 if success, otherwise failure
//////////////////////////////////////////////////////////////////////////////////////
AX_S32 AX_APP_Audio_UnRegisterCapInFrameCallback(AX_VOID);

//////////////////////////////////////////////////////////////////////////////////////
/// @brief register audio encode pakcet out callback
///
/// @param eChan        [I]: audio channel
///        callback     [I]: audio encode pakcet out callback
///        pUserData    [I]: user data
///
/// @return 0 if success, otherwise failure
//////////////////////////////////////////////////////////////////////////////////////
AX_S32 AX_APP_Audio_RegisterPacketCallback(AX_APP_AUDIO_CHAN_E eChan, const AX_APP_AUDIO_PKT_CALLBACK callback, AX_VOID *pUserData);

//////////////////////////////////////////////////////////////////////////////////////
/// @brief unregister audio encode pakcet out callback
///
/// @param eChan        [I]: audio channel
///
/// @return 0 if success, otherwise failure
//////////////////////////////////////////////////////////////////////////////////////
AX_S32 AX_APP_Audio_UnRegisterPacketCallback(AX_APP_AUDIO_CHAN_E eChan);

//////////////////////////////////////////////////////////////////////////////////////
/// @brief register audio catpure in frame observer
///
/// @param pObserver    [I]: observer
///
/// @return 0 if success, otherwise failure
//////////////////////////////////////////////////////////////////////////////////////
AX_S32 AX_APP_Audio_RegCapInFrameObserver(AX_VOID* pObserver);

//////////////////////////////////////////////////////////////////////////////////////
/// @brief unregister audio catpure in frame observer
///
/// @param pObserver    [I]: observer
///
/// @return 0 if success, otherwise failure
//////////////////////////////////////////////////////////////////////////////////////
AX_S32 AX_APP_Audio_UnregCapInFrameObserver(AX_VOID* pObserver);

//////////////////////////////////////////////////////////////////////////////////////
/// @brief register audio encode pakcet out observer
///
/// @param eChan        [I]: audio channel
///        pObserver    [I]: observer
///
/// @return 0 if success, otherwise failure
//////////////////////////////////////////////////////////////////////////////////////
AX_S32 AX_APP_Audio_RegPacketObserver(AX_APP_AUDIO_CHAN_E eChan, AX_VOID* pObserver);

//////////////////////////////////////////////////////////////////////////////////////
/// @brief unregister audio encode pakcet out observer
///
/// @param eChan        [I]: audio channel
///        pObserver    [I]: observer
///
/// @return 0 if success, otherwise failure
//////////////////////////////////////////////////////////////////////////////////////
AX_S32 AX_APP_Audio_UnregPacketObserver(AX_APP_AUDIO_CHAN_E eChan, AX_VOID* pObserver);

//////////////////////////////////////////////////////////////////////////////////////
/// @brief register audio detect result callback
///
/// @param callback     [I]: audio detect result callback
///        pUserData    [I]: user data
///
/// @return 0 if success, otherwise failure
//////////////////////////////////////////////////////////////////////////////////////
AX_S32 AX_APP_Audio_RegisterDetectResultCallback(const AX_APP_AUDIO_DETECTRESULT_CALLBACK callback, AX_VOID *pUserData);

//////////////////////////////////////////////////////////////////////////////////////
/// @brief unregister detect result callback
///
/// @param NA
///
/// @return 0 if success, otherwise failure
//////////////////////////////////////////////////////////////////////////////////////
AX_S32 AX_APP_Audio_UnRegisterDetectResultCallback(AX_VOID);

#ifdef __cplusplus
}
#endif
