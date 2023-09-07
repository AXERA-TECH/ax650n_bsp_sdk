/* receive frame by standard linux select API */
AX_VOID CVideoDecoder::RecvThread(AX_VOID* pArg) {
    LOG_M_D(VDEC, "%s: +++", __func__);

    typedef struct {
        AX_S32 fd;
        AX_U32 nFps;
        AX_VDEC_GRP vdGrp;
        AX_VDEC_CHN vdChn;
        AX_U64 nSeqNum;
        AX_U64 nPTS;
    } VDEC_CHN_INFO_T;

    /* get all fds of each channel */
    const AX_U32 VDEC_GRP_NUM = m_arrGrpInfo.size();
    vector<VDEC_CHN_INFO_T> arrChns;
    arrChns.reserve(VDEC_GRP_NUM * MAX_VDEC_CHN_NUM);

    for (AX_VDEC_GRP vdGrp = 0; vdGrp < (AX_VDEC_GRP)VDEC_GRP_NUM; ++vdGrp) {
        for (AX_VDEC_CHN vdChn = 0; vdChn < MAX_VDEC_CHN_NUM; ++vdChn) {
            if (!m_arrGrpInfo[vdGrp].stAttr.bChnEnable[vdChn]) {
                continue;
            }

            AX_S32 fd = AX_VDEC_GetChnFd(vdGrp, vdChn);
            if (fd <= 0) {
                LOG_M_E(VDEC, "%s: invalid vdGrp %d, vdChn %d fd = %d", __func__, vdGrp, vdChn, fd);
                return;
            }

            arrChns.push_back({fd, m_arrGrpInfo[vdGrp].stAttr.nFps, vdGrp, vdChn, 0, 0});
        }
    }

    const AX_U32 TOTAL_CHN_COUNT = arrChns.size();
    if (0 == TOTAL_CHN_COUNT) {
        LOG_M_E(VDEC, "%s: no chn fds", __func__);
        return;
    }

    AX_S32 ret;
    AX_U32 nCount;
    fd_set rfds;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100 * 1000;

    while (m_DecodeThread.IsRunning()) {
        FD_ZERO(&rfds);

        nCount = 0;
        for (auto& m : arrChns) {
            if (m.fd > 0) {
                FD_SET(m.fd, &rfds);
                ++nCount;
            }
        }

        if (0 == nCount) {
            LOG_M_I(VDEC, "all vdec chns are finished");
            break;
        }

        AX_S32 maxfds = max_element(arrChns.begin(), arrChns.end(),
                                    [](const VDEC_CHN_INFO_T& a, const VDEC_CHN_INFO_T& b) -> bool { return a.fd < b.fd; })
                            ->fd +
                        1;

        /*
            when fd less than 1000, select, poll, epoll effenciy are almost same
            https://blog.csdn.net/tjcwt2011/article/details/125169005
        */
        ret = select(maxfds, &rfds, NULL, NULL, &tv);
        if (ret < 0) {
            LOG_M_E(VDEC, "%s: select fail, %s", __func__, strerror(errno));
            break;
        } else if (0 == ret) {
            /* timeout */
            continue;
        }

        for (AX_U32 i = 0; i < TOTAL_CHN_COUNT; ++i) {
            if (arrChns[i].fd <= 0) {
                /* skip removed chn fd */
                continue;
            }

            if (FD_ISSET(arrChns[i].fd, &rfds)) {
                AX_VDEC_GRP vdGrp = arrChns[i].vdGrp;
                AX_VDEC_CHN vdChn = arrChns[i].vdChn;

                AX_VIDEO_FRAME_INFO_T stVFrame;
                ret = AX_VDEC_GetChnFrame(vdGrp, vdChn, &stVFrame, 100);
                if (0 != ret) {
                    if (AX_ERR_VDEC_FLOW_END == ret) {
                        /* EOF, remove chn fd from select list */
                        FD_CLR(arrChns[i].fd, &rfds);
                        arrChns[i].fd = 0;
                        continue;
                    } else {
                        m_DecodeThread.Stop();
                        LOG_M_E(VDEC, "AX_VDEC_GetChnFrame(vdGrp %d, vdChn %d) fail, ret = 0x%x", vdGrp, vdChn, ret);
                        break;
                    }
                }

                if (1 == ++arrChns[i].nSeqNum || 0 == arrChns[i].nFps) {
                    /* 1st frame or unknown fps */
                    AX_SYS_GetCurPTS(&arrChns[i].nPTS);
                } else {
                    arrChns[i].nPTS += (PTS_TIME_BASE / arrChns[i].nFps);
                }

                /* notify received frame to each registed observer */
                if (m_mapObs.end() != m_mapObs.find(vdGrp)) {
                    CAXFrame axFrame;
                    axFrame.nGrp = (AX_S32)vdGrp;
                    axFrame.nChn = (AX_S32)vdChn;
                    axFrame.stFrame.stVFrame = stVFrame;
                    axFrame.stFrame.stVFrame.stVFrame.u64SeqNum = arrChns[i].nSeqNum;
                    if (0 == axFrame.stFrame.stVFrame.stVFrame.u64PTS) {
                        axFrame.stFrame.stVFrame.stVFrame.u64PTS = arrChns[i].nPTS;
                    }

                    for (auto& pObs : m_mapObs[vdGrp]) {
                        if (pObs) {
                            if (!pObs->OnRecvData(E_OBS_TARGET_TYPE_VDEC, vdGrp, (AX_VOID*)&axFrame)) {
                                LOG_M_E(VDEC, "%s: OnRecvData(vdGrp %d, vdChn %d) fail", __func__, vdGrp, vdChn);
                            }
                        }
                    }
                }

                ret = AX_VDEC_ReleaseChnFrame(vdGrp, vdChn, &stVFrame);
                if (0 != ret) {
                    LOG_M_E(VDEC, "AX_VDEC_ReleaseChnFrame(vdGrp %d, vdChn %d) fail, ret = 0x%x", vdGrp, vdChn, ret);
                    continue;
                }
            }
        }
    }

    LOG_M_D(VDEC, "%s: ---", __func__);
}

#else /*  __VDEC_FD__ */