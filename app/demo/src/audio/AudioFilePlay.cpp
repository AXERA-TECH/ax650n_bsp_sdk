/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "AudioFilePlay.hpp"
#include "AppLogApi.h"
#include "ElapsedTimer.hpp"
#include "GlobalDef.h"

#define AFPLAY "AFPLAY"

using namespace std;

namespace {
#define NOTIFY_AFPLAY_FILE(pstPlayFileAttr) \
            do { \
                std::unique_lock<std::mutex> lck(m_mtxPlayFile); \
                m_qPlayFile.push(pstPlayFileAttr); \
                m_cvPlayFile.notify_one(); \
            } while(0)

#define NOTIFY_AFPLAY_RESULT(pstResult) \
                        do { \
                            std::unique_lock<std::mutex> lck(m_mtxResult); \
                            m_qResult.push(pstResult); \
                            m_cvResult.notify_one(); \
                        } while(0)

#define AUDIO_ID_RIFF 0x46464952
#define AUDIO_ID_WAVE 0x45564157
#define AUDIO_ID_FMT  0x20746d66
#define AUDIO_ID_DATA 0x61746164

typedef struct audioWAV_RIFF_CHUNK_T {
    AX_U32 nChunkID; //'R','I','F','F'
    AX_U32 nChunkSize;
    AX_U32 nFormat; //'W','A','V','E'
} AUDIO_WAV_RIFF_CHUNK_T, *AUDIO_WAV_RIFF_CHUNK_PTR;

typedef struct audioWAV_FMT_CHUNK_T {
    AX_U32 nFmtID;
    AX_U32 nFmtSize;
    AX_U16 nFmtTag;
    AX_U16 nFmtChannels;
    AX_U32 nSampleRate;
    AX_U32 nByteRate;
    AX_U16 nBlockAilgn;
    AX_U16 nBitsPerSample;
} AUDIO_WAV_FMT_CHUNK_T, *AUDIO_WAV_FMT_CHUNK_PTR;

typedef struct audioWAV_DATA_CHUNK_T {
    AX_U32 nDataID; //'d','a','t','a'
    AX_U32 nDataSize;
} AUDIO_WAV_DATA_CHUNK_T, *AUDIO_WAV_DATA_CHUNK_PTR;

typedef struct audioWAV_STRUCT_T {
    AUDIO_WAV_RIFF_CHUNK_T stRiffRegion;
    AUDIO_WAV_FMT_CHUNK_T stFmtRegion;
    AUDIO_WAV_DATA_CHUNK_T stDataRegion;
} AUDIO_WAV_STRUCT_T, *AUDIO_WAV_STRUCT_PTR;

static AX_BOOL IsAudioSupport(AX_PAYLOAD_TYPE_E eType) {
    AX_BOOL bRet = (AX_BOOL)((PT_G711A == eType)
                            || (PT_G711U == eType)
                            || (PT_LPCM == eType)
                            || (PT_G726 == eType)
                            //|| (PT_OPUS == eType)
                            || (PT_AAC == eType));

    return bRet;
}

#define AUDIO_AAC_HEADER_SIZE (7)
#define AUDIO_AAC_HEADER_WITH_CRC_SIZE (AUDIO_AAC_HEADER_SIZE + 2)
}

CAudioFilePlay::CAudioFilePlay(CAudioPlayDev *pAudioPlayDev, CAudioDecoder *pAudioDecoder, AX_U32 nChannel)
: m_nChannel(nChannel)
, m_pAudioPlayDev(pAudioPlayDev)
, m_pAudioDecoder(pAudioDecoder) {

}

CAudioFilePlay::~CAudioFilePlay() {

}

AX_BOOL CAudioFilePlay::Start() {
    if (!m_bStart) {
        if (!m_bPlayFileThreadRunning) {
            m_bPlayFileThreadRunning = AX_TRUE;
            m_hPlayFileThread = std::thread(&CAudioFilePlay::ProcessFileThread, this);
        }

        if (!m_bResultThreadRunning) {
            m_bResultThreadRunning = AX_TRUE;
            m_hResultThread = std::thread(&CAudioFilePlay::ResultCallbackThread, this);
        }

        m_bStart = AX_TRUE;
    }

    return AX_TRUE;
}

AX_BOOL CAudioFilePlay::Stop() {
    if (m_bStart) {
        // STEP 1: stop play
        StopPlay();

        // STEP 2: stop play thread
        if (m_bPlayFileThreadRunning) {
            m_bPlayFileThreadRunning = AX_FALSE;
            m_cvPlayFile.notify_one();

            if (m_hPlayFileThread.joinable()) {
                m_hPlayFileThread.join();
            }
        }

        // STEP 3: stop audio play
        if (m_bResultThreadRunning) {
            m_bResultThreadRunning = AX_FALSE;
            m_cvResult.notify_one();

            if (m_hResultThread.joinable()) {
                m_hResultThread.join();
            }
        }

        m_bStart = AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL CAudioFilePlay::Init() {
    return AX_TRUE;
}

AX_BOOL CAudioFilePlay::DeInit() {
    if (m_bStart) {
        Stop();
    }

    return AX_TRUE;
}

AX_BOOL CAudioFilePlay::Play(AX_PAYLOAD_TYPE_E eType,
                                const std::string &strFileName,
                                AX_S32 nLoop,
                                AUDIO_FILE_PLAY_CALLBACK callback,
                                AX_VOID *pUserData) {
    AX_BOOL bRet = AX_FALSE;

    if ((access(strFileName.c_str(), 0) != 0)) {
        LOG_MM_E(AFPLAY, "invalid file name(%s)", strFileName.c_str());
        return bRet;
    }

    if (!IsAudioSupport(eType)) {
        LOG_MM_E(AFPLAY, "unsupport audio type(%d)", eType);
        return bRet;
    }

    if (!m_pAudioPlayDev) {
        LOG_MM_E(AFPLAY, "invalid audio play dev");
        return bRet;
    }

    if (!m_pAudioDecoder) {
        LOG_MM_E(AFPLAY, "invalid audio decoder");
        return bRet;
    }

    {
        ADEC_CONFIG_T stAttr;
        m_pAudioDecoder->GetAttr(stAttr);

        // check type
        if (eType != stAttr.eType) {
            LOG_MM_E(AFPLAY, "dismatch payload type(init(%d), play(%d)", stAttr.eType, eType);
            return AX_FALSE;
        }
    }

    // stop play
    StopPlay();

    {
        AUDIO_FILE_PLAY_ATTR_PTR pstPlayFileAttr = new AUDIO_FILE_PLAY_ATTR_T;

        if (!pstPlayFileAttr) {
            LOG_MM_E(AFPLAY, "memory alloc (size: %d) failed.", sizeof(AUDIO_FILE_PLAY_ATTR_T));
            return bRet;
        }

        pstPlayFileAttr->nLoop = (0 == nLoop) ? 1 : nLoop;
        pstPlayFileAttr->u64SeqNum = 1;
        pstPlayFileAttr->eType = eType;
        pstPlayFileAttr->strFileName = strFileName;
        pstPlayFileAttr->pUserData = pUserData;
        pstPlayFileAttr->callback = callback;
        pstPlayFileAttr->eStatus = AUDIO_FILE_PLAY_STATUS_BUTT;

        NOTIFY_AFPLAY_FILE(pstPlayFileAttr);

        // sync
        if (!callback) {
            std::unique_lock<std::mutex> lck(m_mtxWaitPlayFile);
            m_cvWaitPlayFile.wait(lck);

            if (AUDIO_FILE_PLAY_STATUS_ERROR != m_ePlayFileStatus) {
                bRet = AX_TRUE;
            }
        }
    }

    return bRet;
}

AX_BOOL CAudioFilePlay::StopPlay() {
    if (m_bPlayingFile) {
        m_bStopPlayFile = AX_TRUE;

        std::unique_lock<std::mutex> lck(m_mtxWaitStopPlayFile);
        m_cvWaitStopPlayFile.wait(lck, [this]() -> bool { return !m_bPlayingFile; });
    }

    return AX_TRUE;
}

AX_VOID CAudioFilePlay::ResultCallbackThread(AX_VOID)
{
    AX_CHAR szName[50] = {0};
    sprintf(szName, "AFPLAY_res_%d", m_nChannel);
    prctl(PR_SET_NAME, szName);

    AUDIO_FILE_PLAY_ATTR_T *pstResult = nullptr;

    m_bResultThreadRunning = AX_TRUE;

    while (m_bResultThreadRunning) {
        pstResult = nullptr;

        {
            std::unique_lock<std::mutex> lck(m_mtxResult);
            m_cvResult.wait(lck, [this]() { return (!m_qResult.empty() || !m_bResultThreadRunning); });

            if (m_qResult.size() > 0) {
                pstResult = m_qResult.front();
                m_qResult.pop();
            }
        }

        if (pstResult) {
            if (pstResult->callback) {
                AUDIO_FILE_PLAY_RESULT_T stResult;
                stResult.eType = pstResult->eType;
                stResult.strFileName = pstResult->strFileName;
                stResult.eStatus = pstResult->eStatus;
                stResult.pUserData = pstResult->pUserData;

                pstResult->callback(m_nChannel, &stResult);
            }

            delete pstResult;
        }
    }
}

AX_VOID CAudioFilePlay::ProcessFileThread()
{
    AX_CHAR szName[50] = {0};
    sprintf(szName, "AFPLAY_%d", m_nChannel);
    prctl(PR_SET_NAME, szName);

    AUDIO_FILE_PLAY_ATTR_T *pstPlayFileAttr = nullptr;

    m_bPlayFileThreadRunning = AX_TRUE;

    while (m_bPlayFileThreadRunning) {
        pstPlayFileAttr = nullptr;

        {
            std::unique_lock<std::mutex> lck(m_mtxPlayFile);
            m_cvPlayFile.wait(lck, [this]() { return (!m_qPlayFile.empty() || !m_bPlayFileThreadRunning); });

            if (m_qPlayFile.size() > 0) {
                pstPlayFileAttr = m_qPlayFile.front();
                m_qPlayFile.pop();
            }
        }

        if (pstPlayFileAttr) {
            m_bPlayingFile = AX_TRUE;
            m_bStopPlayFile = AX_FALSE;

            ProcessPlayFile(pstPlayFileAttr);

            m_ePlayFileStatus = pstPlayFileAttr->eStatus;

            // sync
            if (!pstPlayFileAttr->callback) {
                std::unique_lock<std::mutex> lck(m_mtxWaitPlayFile);
                m_cvWaitPlayFile.notify_one();

                delete pstPlayFileAttr;
            }
            else {
                NOTIFY_AFPLAY_RESULT(pstPlayFileAttr);
            }

            m_bPlayingFile = AX_FALSE;

            if (m_bStopPlayFile) {
                std::unique_lock<std::mutex> lck(m_mtxWaitStopPlayFile);
                m_cvWaitStopPlayFile.notify_one();
            }
        }
    }
}

AX_VOID CAudioFilePlay::ProcessPlayFile(AUDIO_FILE_PLAY_ATTR_PTR pstPlayFileAttr) {
    LOG_MM_I(AFPLAY, "+++");

    FILE *pFile = nullptr;
    AX_S32 nLoop = 0;
    AX_U8 *pBuffer = nullptr;
    AX_S32 nReadSize = 960;
    // aac trans type
    AX_AAC_TRANS_TYPE_E eTransType = AX_AAC_TRANS_TYPE_ADTS;

    pFile = fopen(pstPlayFileAttr->strFileName.c_str(), "rb");

    if (!pFile) {
        pstPlayFileAttr->eStatus = AUDIO_FILE_PLAY_STATUS_ERROR;
        LOG_MM_E(AFPLAY, "open file (%s) failed", pstPlayFileAttr->strFileName.c_str());
        goto EXIT;
    }

    LOG_MM_N(AFPLAY, "start play file: %s", pstPlayFileAttr->strFileName.c_str());

    switch (pstPlayFileAttr->eType) {
    case PT_G711A:
    case PT_G711U:
        nReadSize = 960;
        break;

    case PT_AAC:
        nReadSize = 10240;
        break;

    default:
        nReadSize = 960;
        break;
    }

    pBuffer = new AX_U8[nReadSize];

    if (!pBuffer) {
        pstPlayFileAttr->eStatus = AUDIO_FILE_PLAY_STATUS_ERROR;
        LOG_MM_E(AFPLAY, "alloc memory[%d] failed", nReadSize);
        goto EXIT;
    }

    if (PT_AAC == pstPlayFileAttr->eType){
        ADEC_CONFIG_T stAttr;
        m_pAudioDecoder->GetAttr(stAttr);
        eTransType = stAttr.stDecoderAttr.stAacDecoder.eTransType;
    }

    do {
        AX_S32 nTotalSize = 0;

        do {
            AX_S32 nNumRead = 0;
            AX_S32 nBufferSize = 0;

            if (PT_AAC == pstPlayFileAttr->eType) {
                AX_U8 *pPacket = (AX_U8 *)pBuffer;
                AX_S32 nPacketSize = 0;

                if (AX_AAC_TRANS_TYPE_ADTS == eTransType) {
                    nNumRead = fread(pPacket, 1, AUDIO_AAC_HEADER_SIZE, pFile);
                    if (nNumRead != AUDIO_AAC_HEADER_SIZE) {
                        break;
                    }

                    if (pPacket[0] != 0xff
                        || (pPacket[1] & 0xf0) != 0xf0) {
                        pstPlayFileAttr->eStatus = AUDIO_FILE_PLAY_STATUS_ERROR;
                        LOG_MM_E(AFPLAY, "Not an ADTS packet");
                        break;
                    }

                    /* Set to 1 if there is no CRC and 0 if there is CRC */
                    AX_BOOL bNoCRC = (1 == (pPacket[1] & 0x1)) ? AX_TRUE : AX_FALSE;

                    nPacketSize = ((pPacket[3] & 0x03) << 11) | (pPacket[4] << 3) | (pPacket[5] >> 5);

                    if (bNoCRC) {
                        if (nPacketSize < AUDIO_AAC_HEADER_SIZE
                            || nPacketSize > nReadSize) {
                            pstPlayFileAttr->eStatus = AUDIO_FILE_PLAY_STATUS_ERROR;
                            LOG_MM_E(AFPLAY, "Invalid packet size(%d)", nPacketSize);
                            break;
                        }
                        else {
                            nNumRead = fread(pPacket + AUDIO_AAC_HEADER_SIZE, 1, nPacketSize - AUDIO_AAC_HEADER_SIZE, pFile);

                            if (nNumRead != nPacketSize - AUDIO_AAC_HEADER_SIZE) {
                                pstPlayFileAttr->eStatus = AUDIO_FILE_PLAY_STATUS_ERROR;
                                LOG_MM_E(AFPLAY, "Partial packet");
                                break;
                            }
                        }
                    }
                    else {
                        if (nPacketSize < AUDIO_AAC_HEADER_WITH_CRC_SIZE
                            || nPacketSize > nReadSize) {
                            pstPlayFileAttr->eStatus = AUDIO_FILE_PLAY_STATUS_ERROR;
                            LOG_MM_E(AFPLAY, "Invalid packet size(%d)", nPacketSize);
                            break;
                        }
                        else {
                            nNumRead = fread(pPacket + AUDIO_AAC_HEADER_WITH_CRC_SIZE, 1, nPacketSize - AUDIO_AAC_HEADER_WITH_CRC_SIZE, pFile);

                            if (nNumRead != nPacketSize - AUDIO_AAC_HEADER_WITH_CRC_SIZE) {
                                pstPlayFileAttr->eStatus = AUDIO_FILE_PLAY_STATUS_ERROR;
                                LOG_MM_E(AFPLAY, "Partial packet");
                                break;
                            }
                        }
                    }
                }
                else /*if (AX_AAC_TRANS_TYPE_RAW == eTransType)*/ {
                    nNumRead = fread(&nPacketSize, 1, 2, pFile);
                    if (nNumRead < 1
                        || nPacketSize > nReadSize) {
                        pstPlayFileAttr->eStatus = AUDIO_FILE_PLAY_STATUS_ERROR;
                        LOG_MM_E(AFPLAY, "Invalid packet size(%d)", nPacketSize);
                        break;
                    }

                    nNumRead = fread(pPacket, 1, nPacketSize, pFile);
                    if (nNumRead != nPacketSize) {
                        pstPlayFileAttr->eStatus = AUDIO_FILE_PLAY_STATUS_ERROR;
                        LOG_MM_E(AFPLAY, "Partial packet");
                        break;
                    }
                }

                if (nNumRead > 0) {
                    nBufferSize = nPacketSize;
                    nTotalSize += nBufferSize;
                }
            }
            else if (PT_LPCM == pstPlayFileAttr->eType) {
                if (0 == nTotalSize) {
                    nNumRead = fread(pBuffer, 1, sizeof(AUDIO_WAV_STRUCT_T), pFile);

                    if (nNumRead != sizeof(AUDIO_WAV_STRUCT_T)) {
                        pstPlayFileAttr->eStatus = AUDIO_FILE_PLAY_STATUS_ERROR;
                        LOG_MM_E(AFPLAY, "Error wav format file");
                        break;
                    }

                    AUDIO_WAV_STRUCT_PTR pstWavHeader = (AUDIO_WAV_STRUCT_PTR)pBuffer;

                    if (AUDIO_ID_RIFF != pstWavHeader->stRiffRegion.nChunkID
                        || AUDIO_ID_WAVE != pstWavHeader->stRiffRegion.nFormat
                        // only support mono
                        || pstWavHeader->stFmtRegion.nFmtChannels > 1) {
                        pstPlayFileAttr->eStatus = AUDIO_FILE_PLAY_STATUS_ERROR;
                        LOG_MM_E(AFPLAY, "Invalid wav file: %c%c%c%c %c%c%c%c SampleRate: %d, ByteRate: %d, FmtChannels: %d, BitsPerSample: %d, DataSize: %d",
                                    (AX_U8)(pstWavHeader->stRiffRegion.nChunkID & 0xFF),
                                    (AX_U8)((pstWavHeader->stRiffRegion.nChunkID >> 8) & 0xFF),
                                    (AX_U8)((pstWavHeader->stRiffRegion.nChunkID >> 16) & 0xFF),
                                    (AX_U8)((pstWavHeader->stRiffRegion.nChunkID >> 24) & 0xFF),
                                    (AX_U8)(pstWavHeader->stRiffRegion.nFormat & 0xFF),
                                    (AX_U8)((pstWavHeader->stRiffRegion.nFormat >> 8) & 0xFF),
                                    (AX_U8)((pstWavHeader->stRiffRegion.nFormat >> 16) & 0xFF),
                                    (AX_U8)((pstWavHeader->stRiffRegion.nFormat >> 24) & 0xFF),
                                    pstWavHeader->stFmtRegion.nSampleRate,
                                    pstWavHeader->stFmtRegion.nByteRate,
                                    pstWavHeader->stFmtRegion.nFmtChannels,
                                    pstWavHeader->stFmtRegion.nBitsPerSample,
                                    pstWavHeader->stDataRegion.nDataSize
                                    );
                        break;
                    }
                }

                nNumRead = fread(pBuffer, 1, nReadSize, pFile);

                if (nNumRead > 0) {
                    nBufferSize = nNumRead;
                    nTotalSize += nBufferSize;
                }
            }
            else {
                nNumRead = fread(pBuffer, 1, nReadSize, pFile);

                if (nNumRead > 0) {
                    nBufferSize = nNumRead;  // nPcmSize != nNumRead memleak
                    nTotalSize += nBufferSize;
                }
            }

            if (nBufferSize > 0) {
                {
                    ADEC_CONFIG_T stAttr;
                    m_pAudioDecoder->GetAttr(stAttr);

                    // resample
                    m_pAudioPlayDev->SetResample(stAttr.eSampleRate);
                }

                {
                    AX_AUDIO_STREAM_T stStream;
                    stStream.pStream = (AX_U8 *)pBuffer;
                    stStream.u64PhyAddr = 0;
                    stStream.u32Len = nBufferSize;
                    stStream.u32Seq = (AX_U32)pstPlayFileAttr->u64SeqNum ++;
                    stStream.bEof = AX_FALSE;

                    AX_BOOL bRet = m_pAudioDecoder->Play(pstPlayFileAttr->eType, &stStream);

                    if (!bRet) {
                        pstPlayFileAttr->eStatus = AUDIO_FILE_PLAY_STATUS_ERROR;
                        break;
                    }
                }

                // TODO:
                CElapsedTimer::GetInstance()->mSleep(8);
            }
            else {
                break;
            }
        } while (m_bPlayFileThreadRunning && !m_bStopPlayFile);

        // loop + 1
        ++nLoop;

        // rewind file
        rewind(pFile);

        // Check AO Status
        do {
            AUDIO_PLAY_DEV_STATUS_E eStatus = AUDIO_PLAY_DEV_STATUS_BUTT;
            AX_BOOL bRet = m_pAudioPlayDev->QueryDevStat(eStatus);
            if (!bRet) {
                pstPlayFileAttr->eStatus = AUDIO_FILE_PLAY_STATUS_ERROR;
                break;
            }
            else if (AUDIO_PLAY_DEV_STATUS_BUSY == eStatus) {
                // wait play finish
                CElapsedTimer::GetInstance()->mSleep(5);
            }
            else {
                break;
            }
        } while (1);
    } while (m_bPlayFileThreadRunning
                && !m_bStopPlayFile
                && (pstPlayFileAttr->nLoop < 0 || (nLoop < pstPlayFileAttr->nLoop))
                && pstPlayFileAttr->eStatus != AUDIO_FILE_PLAY_STATUS_ERROR);

    if (m_bStopPlayFile) {
        pstPlayFileAttr->eStatus = AUDIO_FILE_PLAY_STATUS_STOP;
    }
    else if (nLoop >= pstPlayFileAttr->nLoop
       && pstPlayFileAttr->eStatus != AUDIO_FILE_PLAY_STATUS_ERROR) {
        pstPlayFileAttr->eStatus = AUDIO_FILE_PLAY_STATUS_COMPLETE;
    }

EXIT:
    if (pFile) {
        fclose(pFile);
    }

    if (pBuffer) {
        delete[] pBuffer;
    }

    LOG_MM_I(AFPLAY, "---");
}
