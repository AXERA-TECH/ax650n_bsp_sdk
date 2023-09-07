#pragma once

#include "ax_global_type.h"
#include "AudioWrapper.hpp"
#include "AudioCap.hpp"
#include "AudioPlay.hpp"

#ifdef __cplusplus
extern "C" {
#endif

static inline AX_AUDIO_SOUND_MODE_E _AX_SOUND_MODE(AX_APP_AUDIO_SOUND_MODE_E eMode) {
    switch (eMode) {
    case AX_APP_AUDIO_SOUND_MODE_MONO:
        return AX_AUDIO_SOUND_MODE_MONO;
    case AX_APP_AUDIO_SOUND_MODE_STEREO:
        return AX_AUDIO_SOUND_MODE_STEREO;
    default:
        break;
    }

    return AX_AUDIO_SOUND_MODE_BUTT;
}

static inline AX_AUDIO_BIT_WIDTH_E _AX_BIT_WIDTH(AX_APP_AUDIO_BIT_WIDTH_E eBitWidth) {
    switch (eBitWidth) {
    case AX_APP_AUDIO_BIT_WIDTH_8:
        return AX_AUDIO_BIT_WIDTH_8;
    case AX_APP_AUDIO_BIT_WIDTH_16:
        return AX_AUDIO_BIT_WIDTH_16;
    case AX_APP_AUDIO_BIT_WIDTH_24:
        return AX_AUDIO_BIT_WIDTH_24;
    case AX_APP_AUDIO_BIT_WIDTH_32:
        return AX_AUDIO_BIT_WIDTH_32;
    default:
        break;
    }

    return AX_AUDIO_BIT_WIDTH_BUTT;
}

static inline AX_AUDIO_SAMPLE_RATE_E _AX_SAMPLE_RATE(AX_APP_AUDIO_SAMPLE_RATE_E eSampleRate) {
    switch (eSampleRate) {
    case AX_APP_AUDIO_SAMPLE_RATE_8000:
        return AX_AUDIO_SAMPLE_RATE_8000;
    case AX_APP_AUDIO_SAMPLE_RATE_12000:
        return AX_AUDIO_SAMPLE_RATE_12000;
    case AX_APP_AUDIO_SAMPLE_RATE_11025:
        return AX_AUDIO_SAMPLE_RATE_11025;
    case AX_APP_AUDIO_SAMPLE_RATE_16000:
        return AX_AUDIO_SAMPLE_RATE_16000;
    case AX_APP_AUDIO_SAMPLE_RATE_22050:
        return AX_AUDIO_SAMPLE_RATE_22050;
    case AX_APP_AUDIO_SAMPLE_RATE_24000:
        return AX_AUDIO_SAMPLE_RATE_24000;
    case AX_APP_AUDIO_SAMPLE_RATE_32000:
        return AX_AUDIO_SAMPLE_RATE_32000;
    case AX_APP_AUDIO_SAMPLE_RATE_44100:
        return AX_AUDIO_SAMPLE_RATE_44100;
    case AX_APP_AUDIO_SAMPLE_RATE_48000:
        return AX_AUDIO_SAMPLE_RATE_48000;
    case AX_APP_AUDIO_SAMPLE_RATE_64000:
        return AX_AUDIO_SAMPLE_RATE_64000;
    case AX_APP_AUDIO_SAMPLE_RATE_96000:
        return AX_AUDIO_SAMPLE_RATE_96000;
    default:
        break;
    }

    return AX_AUDIO_SAMPLE_RATE_16000;
}

static inline AX_AGC_MODE_E _AX_AGC_MODE(AX_APP_AUDIO_AGC_MODE_E eMode) {
    switch (eMode) {
    case AX_APP_AUDIO_AGC_MODE_ADAPTIVE_ANALOG:
        return AX_AGC_MODE_ADAPTIVE_ANALOG;
    case AX_APP_AUDIO_AGC_MODE_ADAPTIVE_DIGITAL:
        return AX_AGC_MODE_ADAPTIVE_DIGITAL;
    case AX_APP_AUDIO_AGC_MODE_FIXED_DIGITAL:
        return AX_AGC_MODE_FIXED_DIGITAL;
    default:
        break;
    }

    return AX_AGC_MODE_FIXED_DIGITAL;
}

static inline AX_AGGRESSIVENESS_LEVEL_E _AX_AGGRESSIVENESS_LEVEL(AX_APP_AUDIO_ANS_AGGRESSIVENESS_LEVEL_E eLevel) {
    switch (eLevel) {
    case AX_APP_AUDIO_ANS_AGGRESSIVENESS_LEVEL_LOW:
        return AX_AGGRESSIVENESS_LEVEL_LOW;
    case AX_APP_AUDIO_ANS_AGGRESSIVENESS_LEVEL_MODERATE:
        return AX_AGGRESSIVENESS_LEVEL_MODERATE;
    case AX_APP_AUDIO_ANS_AGGRESSIVENESS_LEVEL_HIGH:
        return AX_AGGRESSIVENESS_LEVEL_HIGH;
    case AX_APP_AUDIO_ANS_AGGRESSIVENESS_LEVEL_VERYHIGH:
        return AX_AGGRESSIVENESS_LEVEL_VERYHIGH;
    default:
        break;
    }

    return AX_AGGRESSIVENESS_LEVEL_MODERATE;
}

static inline AX_AEC_MODE_E _AX_AEC_TYPE(AX_APP_AUDIO_AEC_TYPE_E eType) {
    switch (eType) {
    case AX_APP_AUDIO_AEC_DISABLE:
        return AX_AEC_MODE_DISABLE;
    case AX_APP_AUDIO_AEC_FLOAT:
        return AX_AEC_MODE_FLOAT;
    case AX_APP_AUDIO_AEC_FIXED:
        return AX_AEC_MODE_FIXED;
    default:
        break;
    }

    return AX_AEC_MODE_FIXED;
}

static inline AX_ROUTING_MODE_E _AX_ROUTING_MODE(AX_APP_AUDIO_AEC_ROUTING_MODE_E eMode) {
    switch (eMode) {
    case AX_APP_AUDIO_AEC_QUITE_EARPIECE_OR_HEADSET:
        return AX_ROUTING_MODE_QUITE_EARPIECE_OR_HEADSET;
    case AX_APP_AUDIO_AEC_EARPIECE:
        return AX_ROUTING_MODE_EARPIECE;
    case AX_APP_AUDIO_AEC_LOUD_EARPIECE:
        return AX_ROUTING_MODE_LOUD_EARPIECE;
    case AX_APP_AUDIO_AEC_SPEAKERPHONE:
        return AX_ROUTING_MODE_SPEAKERPHONE;
    case AX_APP_AUDIO_AEC_LOUD_SPEAKERPHONE:
        return AX_ROUTING_MODE_LOUD_SPEAKERPHONE;
    default:
        break;
    }

    return AX_ROUTING_MODE_LOUD_SPEAKERPHONE;
}

static inline AX_SUPPRESSION_LEVEL_E _AX_SUPPRESSION_LEVEL(AX_APP_AUDIO_AEC_SUPPRESSION_LEVEL_E eLevel) {
    switch (eLevel) {
    case AX_APP_AUDIO_AEC_SUPPRESSION_LEVEL_LOW:
        return AX_SUPPRESSION_LEVEL_LOW;
    case AX_APP_AUDIO_AEC_SUPPRESSION_LEVEL_MODERATE:
        return AX_SUPPRESSION_LEVEL_MODERATE;
    case AX_APP_AUDIO_AEC_SUPPRESSION_LEVEL_HIGH:
        return AX_SUPPRESSION_LEVEL_HIGH;
    default:
        break;
    }

    return AX_SUPPRESSION_LEVEL_MODERATE;
}

static inline AX_AAC_TYPE_E _AX_AAC_TYPE(AX_APP_AUDIO_AAC_TYPE_E eType) {
    switch (eType) {
    case AX_APP_AUDIO_AAC_TYPE_NONE:
        return AX_AAC_TYPE_NONE;
    case AX_APP_AUDIO_AAC_TYPE_NULL_OBJECT:
        return AX_AAC_TYPE_NULL_OBJECT;
    case AX_APP_AUDIO_AAC_TYPE_AAC_LC:
        return AX_AAC_TYPE_AAC_LC;
    case AX_APP_AUDIO_AAC_TYPE_ER_AAC_LD:
        return AX_AAC_TYPE_ER_AAC_LD;
    case AX_APP_AUDIO_AAC_TYPE_ER_AAC_ELD:
        return AX_AAC_TYPE_ER_AAC_ELD;
    default:
        break;
    }

    return AX_AAC_TYPE_BUTT;
}

static inline AX_AAC_TRANS_TYPE_E _AX_AAC_TRANS_TYPE(AX_APP_AUDIO_AAC_TRANS_TYPE_E eType) {
    switch (eType) {
    case AX_APP_AUDIO_AAC_TRANS_TYPE_UNKNOWN:
        return AX_AAC_TRANS_TYPE_UNKNOWN;
    case AX_APP_AUDIO_AAC_TRANS_TYPE_RAW:
        return AX_AAC_TRANS_TYPE_RAW;
    case AX_APP_AUDIO_AAC_TRANS_TYPE_ADTS:
        return AX_AAC_TRANS_TYPE_ADTS;
    default:
        break;
    }

    return AX_AAC_TRANS_TYPE_BUTT;
}

static inline AX_APP_AUDIO_SOUND_MODE_E _APP_SOUND_MODE(AX_AUDIO_SOUND_MODE_E eMode) {
    switch (eMode) {
    case AX_AUDIO_SOUND_MODE_MONO:
        return AX_APP_AUDIO_SOUND_MODE_MONO;
    case AX_AUDIO_SOUND_MODE_STEREO:
        return AX_APP_AUDIO_SOUND_MODE_STEREO;
    default:
        break;
    }

    return AX_APP_AUDIO_SOUND_MODE_BUTT;
}

static inline AX_APP_AUDIO_BIT_WIDTH_E _APP_BIT_WIDTH(AX_AUDIO_BIT_WIDTH_E eBitWidth) {
    switch (eBitWidth) {
    case AX_AUDIO_BIT_WIDTH_8:
        return AX_APP_AUDIO_BIT_WIDTH_8;
    case AX_AUDIO_BIT_WIDTH_16:
        return AX_APP_AUDIO_BIT_WIDTH_16;
    case AX_AUDIO_BIT_WIDTH_24:
        return AX_APP_AUDIO_BIT_WIDTH_24;
    case AX_AUDIO_BIT_WIDTH_32:
        return AX_APP_AUDIO_BIT_WIDTH_32;
    default:
        break;
    }

    return AX_APP_AUDIO_BIT_WIDTH_BUTT;
}

static inline AX_APP_AUDIO_SAMPLE_RATE_E _APP_SAMPLE_RATE(AX_AUDIO_SAMPLE_RATE_E eSampleRate) {
    switch (eSampleRate) {
    case AX_AUDIO_SAMPLE_RATE_8000:
        return AX_APP_AUDIO_SAMPLE_RATE_8000;
    case AX_AUDIO_SAMPLE_RATE_12000:
        return AX_APP_AUDIO_SAMPLE_RATE_12000;
    case AX_AUDIO_SAMPLE_RATE_11025:
        return AX_APP_AUDIO_SAMPLE_RATE_11025;
    case AX_AUDIO_SAMPLE_RATE_16000:
        return AX_APP_AUDIO_SAMPLE_RATE_16000;
    case AX_AUDIO_SAMPLE_RATE_22050:
        return AX_APP_AUDIO_SAMPLE_RATE_22050;
    case AX_AUDIO_SAMPLE_RATE_24000:
        return AX_APP_AUDIO_SAMPLE_RATE_24000;
    case AX_AUDIO_SAMPLE_RATE_32000:
        return AX_APP_AUDIO_SAMPLE_RATE_32000;
    case AX_AUDIO_SAMPLE_RATE_44100:
        return AX_APP_AUDIO_SAMPLE_RATE_44100;
    case AX_AUDIO_SAMPLE_RATE_48000:
        return AX_APP_AUDIO_SAMPLE_RATE_48000;
    case AX_AUDIO_SAMPLE_RATE_64000:
        return AX_APP_AUDIO_SAMPLE_RATE_64000;
    case AX_AUDIO_SAMPLE_RATE_96000:
        return AX_APP_AUDIO_SAMPLE_RATE_96000;
    default:
        break;
    }

    return AX_APP_AUDIO_SAMPLE_RATE_16000;
}

static inline AX_APP_AUDIO_AGC_MODE_E _APP_AGC_MODE(AX_AGC_MODE_E eMode) {
    switch (eMode) {
    case AX_AGC_MODE_ADAPTIVE_ANALOG:
        return AX_APP_AUDIO_AGC_MODE_ADAPTIVE_ANALOG;
    case AX_AGC_MODE_ADAPTIVE_DIGITAL:
        return AX_APP_AUDIO_AGC_MODE_ADAPTIVE_DIGITAL;
    case AX_AGC_MODE_FIXED_DIGITAL:
        return AX_APP_AUDIO_AGC_MODE_FIXED_DIGITAL;
    default:
        break;
    }

    return AX_APP_AUDIO_AGC_MODE_FIXED_DIGITAL;
}

static inline AX_APP_AUDIO_ANS_AGGRESSIVENESS_LEVEL_E _APP_AGGRESSIVENESS_LEVEL(AX_AGGRESSIVENESS_LEVEL_E eLevel) {
    switch (eLevel) {
    case AX_AGGRESSIVENESS_LEVEL_LOW:
        return AX_APP_AUDIO_ANS_AGGRESSIVENESS_LEVEL_LOW;
    case AX_AGGRESSIVENESS_LEVEL_MODERATE:
        return AX_APP_AUDIO_ANS_AGGRESSIVENESS_LEVEL_MODERATE;
    case AX_AGGRESSIVENESS_LEVEL_HIGH:
        return AX_APP_AUDIO_ANS_AGGRESSIVENESS_LEVEL_HIGH;
    case AX_AGGRESSIVENESS_LEVEL_VERYHIGH:
        return AX_APP_AUDIO_ANS_AGGRESSIVENESS_LEVEL_VERYHIGH;
    default:
        break;
    }

    return AX_APP_AUDIO_ANS_AGGRESSIVENESS_LEVEL_MODERATE;
}

static inline AX_APP_AUDIO_AEC_TYPE_E _APP_AEC_TYPE(AX_AEC_MODE_E eMode) {
    switch (eMode) {
    case AX_AEC_MODE_DISABLE:
        return AX_APP_AUDIO_AEC_DISABLE;
    case AX_AEC_MODE_FLOAT:
        return AX_APP_AUDIO_AEC_FLOAT;
    case AX_AEC_MODE_FIXED:
        return AX_APP_AUDIO_AEC_FIXED;
    default:
        break;
    }

    return AX_APP_AUDIO_AEC_FIXED;
}

static inline AX_APP_AUDIO_AEC_ROUTING_MODE_E _APP_ROUTING_MODE(AX_ROUTING_MODE_E eMode) {
    switch (eMode) {
    case AX_ROUTING_MODE_QUITE_EARPIECE_OR_HEADSET:
        return AX_APP_AUDIO_AEC_QUITE_EARPIECE_OR_HEADSET;
    case AX_ROUTING_MODE_EARPIECE:
        return AX_APP_AUDIO_AEC_EARPIECE;
    case AX_ROUTING_MODE_LOUD_EARPIECE:
        return AX_APP_AUDIO_AEC_LOUD_EARPIECE;
    case AX_ROUTING_MODE_SPEAKERPHONE:
        return AX_APP_AUDIO_AEC_SPEAKERPHONE;
    case AX_ROUTING_MODE_LOUD_SPEAKERPHONE:
        return AX_APP_AUDIO_AEC_LOUD_SPEAKERPHONE;
    default:
        break;
    }

    return AX_APP_AUDIO_AEC_LOUD_SPEAKERPHONE;
}

static inline AX_APP_AUDIO_AEC_SUPPRESSION_LEVEL_E _APP_SUPPRESSION_LEVEL(AX_SUPPRESSION_LEVEL_E eLevel) {
    switch (eLevel) {
    case AX_SUPPRESSION_LEVEL_LOW:
        return AX_APP_AUDIO_AEC_SUPPRESSION_LEVEL_LOW;
    case AX_SUPPRESSION_LEVEL_MODERATE:
        return AX_APP_AUDIO_AEC_SUPPRESSION_LEVEL_MODERATE;
    case AX_SUPPRESSION_LEVEL_HIGH:
        return AX_APP_AUDIO_AEC_SUPPRESSION_LEVEL_HIGH;
    default:
        break;
    }

    return AX_APP_AUDIO_AEC_SUPPRESSION_LEVEL_MODERATE;
}

static inline AX_APP_AUDIO_AAC_TYPE_E _APP_AAC_TYPE(AX_AAC_TYPE_E eType) {
    switch (eType) {
    case AX_AAC_TYPE_NONE:
        return AX_APP_AUDIO_AAC_TYPE_NONE;
    case AX_AAC_TYPE_NULL_OBJECT:
        return AX_APP_AUDIO_AAC_TYPE_NULL_OBJECT;
    case AX_AAC_TYPE_AAC_LC:
        return AX_APP_AUDIO_AAC_TYPE_AAC_LC;
    case AX_AAC_TYPE_ER_AAC_LD:
        return AX_APP_AUDIO_AAC_TYPE_ER_AAC_LD;
    case AX_AAC_TYPE_ER_AAC_ELD:
        return AX_APP_AUDIO_AAC_TYPE_ER_AAC_ELD;
    default:
        break;
    }

    return AX_APP_AUDIO_AAC_TYPE_BUTT;
}

static inline AX_APP_AUDIO_AAC_TRANS_TYPE_E _APP_AAC_TRANS_TYPE(AX_AAC_TRANS_TYPE_E eType) {
    switch (eType) {
    case AX_AAC_TRANS_TYPE_UNKNOWN:
        return AX_APP_AUDIO_AAC_TRANS_TYPE_UNKNOWN;
    case AX_AAC_TRANS_TYPE_RAW:
        return AX_APP_AUDIO_AAC_TRANS_TYPE_RAW;
    case AX_AAC_TRANS_TYPE_ADTS:
        return AX_APP_AUDIO_AAC_TRANS_TYPE_ADTS;
    default:
        break;
    }

    return AX_APP_AUDIO_AAC_TRANS_TYPE_BUTT;
}

static inline AX_APP_AUDIO_PLAY_FILE_STATUS_E _APP_FILE_PLAY_STATUS(AUDIO_FILE_PLAY_STATUS_E eStatus) {
    switch (eStatus) {
    case AUDIO_FILE_PLAY_STATUS_COMPLETE:
        return AX_APP_AUDIO_PLAY_FILE_STATUS_COMPLETE;
    case AUDIO_FILE_PLAY_STATUS_STOP:
        return AX_APP_AUDIO_PLAY_FILE_STATUS_STOP;
    case AUDIO_FILE_PLAY_STATUS_ERROR:
        return AX_APP_AUDIO_PLAY_FILE_STATUS_ERROR;
    default:
        break;
    }

    return AX_APP_AUDIO_PLAY_FILE_STATUS_BUTT;
}

#ifdef __cplusplus
}
#endif

