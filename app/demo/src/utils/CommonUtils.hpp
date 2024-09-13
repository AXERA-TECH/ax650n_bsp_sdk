/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#pragma once

#include <arpa/inet.h>
#include <dirent.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "CmdLineParser.h"
#include "GlobalDef.h"
#include "IPPLBuilder.h"
#include "IVPSGrpStage.h"
#include "OSDHandler.h"
#include "ax_base_type.h"

#define UTILS "Utils"

class CCommonUtils {
public:
    CCommonUtils(AX_VOID) = default;
    ~CCommonUtils(AX_VOID) = default;

    static std::string GetPPLConfigDir() {
        AX_S32 nIndex = 0;
        if (!CCmdLineParser::GetInstance()->GetPPLIndex(nIndex)) {
            return "";
        }
#ifdef SLT
        switch (nIndex) {
            case E_PPL_TYPE_ITS:
                return "/opt/bin/res/frtdemo/config/its";
            case E_PPL_TYPE_IPC:
                return "/opt/bin/res/frtdemo/config/ipc";
            case E_PPL_TYPE_PANO:
                return "/opt/bin/res/frtdemo/config/pano";
            default:
                return "";
        }
#else
        switch (nIndex) {
            case E_PPL_TYPE_ITS:
                return "./config/its";
            case E_PPL_TYPE_IPC:
                return "./config/ipc";
            case E_PPL_TYPE_PANO:
                return "./config/pano";
            default:
                return "";
        }
#endif
        return "";
    }

    static std::string GetPPLSpecification() {
        AX_S32 nIndex = 0;
        if (!CCmdLineParser::GetInstance()->GetPPLIndex(nIndex)) {
            return "";
        }

        switch (nIndex) {
            case E_PPL_TYPE_ITS:
                return "FRTDemo-ITS";
            case E_PPL_TYPE_IPC:
                return "FRTDemo-IPC";
            case E_PPL_TYPE_PANO:
                return "FRTDemo-PANO";
            default:
                return "";
        }

        return "";
    }

    static AX_BOOL ModuleEqual(const IPC_MOD_INFO_T& tMod_1, const IPC_MOD_INFO_T& tMod_2, AX_BOOL bIgnoreChn = AX_FALSE) {
        return (tMod_1.eModType == tMod_2.eModType && tMod_1.nGroup == tMod_2.nGroup && (bIgnoreChn || tMod_1.nChannel == tMod_2.nChannel))
                   ? AX_TRUE
                   : AX_FALSE;
    }

    static AX_S32 OverlayOffsetX(AX_S32 nWidth, AX_S32 nOsdWidth, AX_S32 nXMargin, OSD_ALIGN_TYPE_E eAlign) {
        AX_S32 Offset = 0;
        if (OSD_ALIGN_TYPE_LEFT_TOP == eAlign || OSD_ALIGN_TYPE_LEFT_BOTTOM == eAlign) {
            if (nWidth < nOsdWidth) {
                Offset = nXMargin;
            } else {
                if (nWidth - nOsdWidth > nXMargin) {
                    Offset = nXMargin;
                } else {
                    Offset = nWidth - nOsdWidth;
                }
            }
            Offset = ALIGN_UP(Offset, OSD_ALIGN_X_OFFSET);

        } else if (OSD_ALIGN_TYPE_RIGHT_TOP == eAlign || OSD_ALIGN_TYPE_RIGHT_BOTTOM == eAlign) {
            if (nWidth < nOsdWidth) {
                Offset = 0;
            } else {
                if (nWidth - nOsdWidth > nXMargin) {
                    Offset = nWidth - (nOsdWidth + nXMargin) - (OSD_ALIGN_X_OFFSET - 1);
                    if (Offset < 0) {
                        Offset = 0;
                    }
                    Offset = ALIGN_UP(Offset, OSD_ALIGN_X_OFFSET);
                } else {
                    Offset = 0;
                }
            }
        }
        if (Offset < 0) {
            Offset = 0;
        }
        return Offset;
    }
    static AX_S32 OverlayOffsetY(AX_S32 nHeight, AX_S32 nOsdHeight, AX_S32 nYMargin, OSD_ALIGN_TYPE_E eAlign) {
        AX_S32 Offset = 0;
        if (OSD_ALIGN_TYPE_LEFT_TOP == eAlign || OSD_ALIGN_TYPE_RIGHT_TOP == eAlign) {
            if (nHeight < nOsdHeight) {
                Offset = nYMargin;
            } else {
                if (nHeight - nOsdHeight > nYMargin) {
                    Offset = nYMargin;
                } else {
                    Offset = nHeight - nOsdHeight;
                }
            }
            Offset = ALIGN_UP(Offset, OSD_ALIGN_Y_OFFSET);
        } else if (OSD_ALIGN_TYPE_LEFT_BOTTOM == eAlign || OSD_ALIGN_TYPE_RIGHT_BOTTOM == eAlign) {
            if (nHeight < nOsdHeight) {
                Offset = 0;
            } else {
                if (nHeight - nOsdHeight > nYMargin) {
                    Offset = nHeight - (nOsdHeight + nYMargin) - (OSD_ALIGN_Y_OFFSET - 1);
                    if (Offset < 0) {
                        Offset = 0;
                    }
                    Offset = ALIGN_UP(Offset, OSD_ALIGN_Y_OFFSET);
                } else {
                    Offset = 0;
                }
            }
        }
        if (Offset < 0) {
            Offset = 0;
        }
        return Offset;
    }

    static AX_BOOL LoadImage(const AX_CHAR* pszImge, AX_U64* pPhyAddr, AX_VOID** ppVirAddr, AX_U32 nImgSize) {
        if (nullptr == pszImge || 0 == nImgSize) {
            LOG_M_E(UTILS, "invailed param\n");
            return AX_FALSE;
        }

        FILE* fp = fopen(pszImge, "rb");
        if (fp) {
            fseek(fp, 0, SEEK_END);
            AX_U32 nFileSize = ftell(fp);
            if (nImgSize != nFileSize) {
                LOG_M_E(UTILS, "file size not right, %d != %d", nImgSize, nFileSize);
                fclose(fp);
                return AX_FALSE;
            }
            fseek(fp, 0, SEEK_SET);
            AX_S32 ret = AX_SYS_MemAlloc(pPhyAddr, ppVirAddr, nFileSize, 128, (AX_S8*)"LOAD_IMAGE");
            if (0 != ret) {
                LOG_M_E(UTILS, "AX_SYS_MemAlloc fail, ret=0x%x", ret);
                fclose(fp);
                return AX_FALSE;
            }
            if (fread(*ppVirAddr, 1, nFileSize, fp) != nFileSize) {
                LOG_M_E(UTILS, "fread fail, %s", strerror(errno));
                fclose(fp);
                return AX_FALSE;
            }

            fclose(fp);
            return AX_TRUE;
        } else {
            LOG_M_E(UTILS, "fopen %s fail, %s", pszImge, strerror(errno));
            return AX_FALSE;
        }
    }

    static AX_BOOL GetIP(AX_CHAR* pOutIPAddress) {
        std::string strDevPrefix = "eth";
        for (char c = '0'; c <= '9'; ++c) {
            std::string strDevice = strDevPrefix + c;
            int fd;
            struct ifreq ifr;
            fd = socket(AF_INET, SOCK_DGRAM, 0);
            strcpy(ifr.ifr_name, strDevice.c_str());
            if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) {
                ::close(fd);
                continue;
            }

            char* pIP = inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr);
            if (pIP) {
                strcpy((char*)pOutIPAddress, pIP);
                ::close(fd);
                return AX_TRUE;
            }
        }

        return AX_FALSE;
    }

    static AX_BOOL GetBoardID(AX_CHAR* szBoardID, AX_U32 nSize) {
        if (nullptr == szBoardID || 0 == nSize) {
            return AX_FALSE;
        }

        constexpr AX_CHAR DEV_PATH[] = "/proc/ax_proc/board_id";
        FILE* pFile = NULL;
        pFile = fopen(DEV_PATH, "r");
        if (pFile) {
            fread(&szBoardID[0], nSize, 1, pFile);
            fclose(pFile);
        } else {
            LOG_MM_E(UTILS, "open %s fail", DEV_PATH);
            return AX_FALSE;
        }

        return AX_TRUE;
    }

    static std::string GetPathFromFileName(const std::string strFileName)
    {
        if (strFileName.empty()) {
            LOG_MM_E(UTILS, "strFileName is empty");
            return "";
        }

        size_t pos = strFileName.find_last_of("/");
        std::string strPath = strFileName.substr(0, pos + 1);

        LOG_MM_D(UTILS, "get file path: %s", strPath.c_str());

        return strPath;
    }
};
