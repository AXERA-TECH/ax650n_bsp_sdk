#include "SplitWidgetMgr.h"
#include "AXNVRFramework.h"

#include <QDebug>

#ifndef ALIGN_UP
#define ALIGN_UP(x, align) (((x) + ((align)-1)) & ~((align)-1))
#endif

#ifndef ALIGN_DOWN
#define ALIGN_DOWN(x, align) ((x) & ~((align)-1))
#endif

SplitWidgetMgr::SplitWidgetMgr(AX_NVR_VIEW_TYPE enViewType, QGridLayout *gridLayout, QWidget *parent)
    : m_enViewType(enViewType)
    , m_pGridLayout(gridLayout)
    , m_pParentWidget(parent)
{
    switch (m_enViewType)
    {
    case AX_NVR_VIEW_TYPE::PREVIEW:
        m_eSupportSplitType = SPLIT_TYPE::FOUR;
        m_eCurrentSplitType = m_eSupportSplitType;
        break;
    case AX_NVR_VIEW_TYPE::PLAYBACK:
        m_eSupportSplitType = SPLIT_TYPE::ONE;
        m_eCurrentSplitType = m_eSupportSplitType;
        break;
    case AX_NVR_VIEW_TYPE::POLLING:
        m_eSupportSplitType = SPLIT_TYPE::ONE;
        m_eCurrentSplitType = m_eSupportSplitType;
        break;
    case AX_NVR_VIEW_TYPE::PIP:
        m_eSupportSplitType = SPLIT_TYPE::ONE;
        m_eCurrentSplitType = m_eSupportSplitType;
        break;
    default:
        break;
    }

    this->init();
}

SplitWidgetMgr::~SplitWidgetMgr() {
    this->deinit();
}

void SplitWidgetMgr::Reset(void) {
    for (auto &item: m_listPreviewWidgets) {
        item.pDispWidget->Reset();
    }
}

int SplitWidgetMgr::GetWidgetIndex(QWidget *pWidget) {
    int index = -1;
    for (auto &item: m_listPreviewWidgets) {
        if (item.pDispWidget == pWidget) {
            index = item.nIndex;
            break;
        }
    }
    return index;
}

int SplitWidgetMgr::GetWidgetDevid(QWidget *pWidget) {
    int index = -1;
    for (auto &item: m_listPreviewWidgets) {
        if (item.pDispWidget == pWidget) {
            index = item.pDispWidget->m_nDevId;
            break;
        }
    }
    return index;
}

const vector<AX_NVR_RECT_T> SplitWidgetMgr::CalcLayouts(SPLIT_TYPE enSplitType,
                                                        int nWidth, int nHeight,
                                                        int nLeftMargin, int nTopMargin,
                                                        int nRightMargin, int nBottomMargin) const {

    int cols_ = 0;
    int rows_ = 0;
    switch (enSplitType)
    {
    case SPLIT_TYPE::ONE:
    case SPLIT_TYPE::MAX:
        cols_ = 1;
        rows_ = 1;
        break;
    case SPLIT_TYPE::FOUR:
        cols_ = 2;
        rows_ = 2;
        break;
    case SPLIT_TYPE::SIXTEEN:
    case SPLIT_TYPE::EIGHT:
        cols_ = 4;
        rows_ = 4;
        break;
    case SPLIT_TYPE::THIRTYSIX:
        cols_ = 6;
        rows_ = 6;
        break;
    case SPLIT_TYPE::SIXTYFOUR:
        cols_ = 8;
        rows_ = 8;
        break;
    default:
        cols_ = 1;
        rows_ = 1;
        break;
    }

    int verticalSpacing = m_pGridLayout->verticalSpacing();
    int horizontalSpacing = m_pGridLayout->horizontalSpacing();
    int rect_w_tmp = (nWidth - (cols_ - 1) * horizontalSpacing) / cols_;
    int rect_h_tmp = (nHeight - (rows_ - 1) * verticalSpacing) / rows_;
    int rect_w = ALIGN_DOWN(rect_w_tmp, 8);
    int rect_h = ALIGN_DOWN(rect_h_tmp, 2);
    int w = rect_w * cols_ + (cols_ - 1) * horizontalSpacing;
    int h = rect_h * rows_ + (rows_ - 1) * verticalSpacing;
    int left_right = nWidth - w;
    int top_bottom = nHeight - h;
    int left = left_right / 2;
    left = left - left % 2;
    int right = left_right - left;
    int top = top_bottom / 2;
    top = top - top % 2;
    int bottom = top_bottom - top;

    m_pGridLayout->setContentsMargins(nLeftMargin + left, nTopMargin + top, nRightMargin + right, nBottomMargin + bottom);

    vector<AX_NVR_RECT_T> vecVoRect;
    QPoint pt = m_pParentWidget->mapToGlobal(QPoint(0, 0));
    if (enSplitType == SPLIT_TYPE::EIGHT) {
        int index = 0;
        vector<int> channels = {1,2,4,5,6,8,9,10};
        for (AX_U32 i = 0; i < (AX_U32)rows_; ++i) {
            for (AX_U32 j = 0; j < (AX_U32)cols_; ++j) {
                if (std::find(channels.begin(), channels.end(), index) != channels.end()) {
                    index++;
                    continue;
                }

                AX_NVR_RECT_T nvr_rect;
                nvr_rect.x = pt.x() + nLeftMargin + left + j * horizontalSpacing + j * rect_w;
                nvr_rect.y = pt.y() + nTopMargin + top + i * verticalSpacing + i * rect_h;
                if (index == 0) {
                    nvr_rect.w = rect_w * 3 + horizontalSpacing * 2;
                    nvr_rect.h = rect_h * 3 + verticalSpacing * 2;
                }
                else {
                    nvr_rect.w = rect_w;
                    nvr_rect.h = rect_h;
                }
                vecVoRect.emplace_back(nvr_rect);
                // qDebug() << nvr_rect.x << nvr_rect.y << nvr_rect.w << nvr_rect.h;
                index++;
            }
        }
    } else {
        for (AX_U32 i = 0; i < (AX_U32)rows_; ++i) {
            for (AX_U32 j = 0; j < (AX_U32)cols_; ++j) {
                AX_NVR_RECT_T nvr_rect;
                nvr_rect.x = pt.x() + nLeftMargin + left + j * horizontalSpacing + j * rect_w;
                nvr_rect.y = pt.y() + nTopMargin + top + i * verticalSpacing + i * rect_h;
                nvr_rect.w = rect_w;
                nvr_rect.h = rect_h;
                // qDebug() << nvr_rect.x << nvr_rect.y << nvr_rect.w << nvr_rect.h;
                vecVoRect.emplace_back(nvr_rect);
            }
        }
    }
    return vecVoRect;
}

const vector<AX_NVR_DEV_ID> SplitWidgetMgr::GetViewChns(SPLIT_TYPE enSplitType, PREV_NEXT_TYPE enPrevNextType) const {

    vector<AX_NVR_DEV_ID> vecChannels;
    if (m_nMaxWidgetIndex == -1) {
        const int nStep = this->get_channel_step(enSplitType);
        if (nStep < 1) {
            return vecChannels;
        }

        int StartIndex = m_nCurrentStartIndex;
        if (PREV_NEXT_TYPE::PREV == enPrevNextType) {
            StartIndex = m_nCurrentStartIndex - nStep;
            if (StartIndex < 0) {
                StartIndex = 0;
            }
        }
        else if (PREV_NEXT_TYPE::NEXT == enPrevNextType){
            StartIndex = StartIndex + nStep;
            if (StartIndex >= MAX_DEVICE_COUNT) {
                StartIndex = StartIndex - nStep;
            }
        }

        for (int i = StartIndex; i < StartIndex + nStep; ++i) {
            auto &item = m_listPreviewWidgets[i];
            vecChannels.emplace_back(item.nIndex);
        }
    } else {
        auto &item = m_listPreviewWidgets[m_nMaxWidgetIndex];
        vecChannels.emplace_back(item.nIndex);
    }
    return vecChannels;
}

const ax_nvr_channel_vector SplitWidgetMgr::GetViewChannels(SPLIT_TYPE enSplitType, PREV_NEXT_TYPE enPrevNextType) const {

    ax_nvr_channel_vector vecChannels;
    if (m_nMaxWidgetIndex == -1) {
        const int nStep = this->get_channel_step(enSplitType);
        if (nStep < 1) {
            return vecChannels;
        }

        int StartIndex = m_nCurrentStartIndex;
        if (PREV_NEXT_TYPE::PREV == enPrevNextType) {
            StartIndex = m_nCurrentStartIndex - nStep;
            if (StartIndex < 0) {
                StartIndex = 0;
            }
        }
        else if (PREV_NEXT_TYPE::NEXT == enPrevNextType){
            StartIndex = StartIndex + nStep;
            if (StartIndex >= MAX_DEVICE_COUNT) {
                StartIndex = StartIndex - nStep;
            }
        }

        for (int i = StartIndex; i < StartIndex + nStep; ++i) {
            auto &item = m_listPreviewWidgets[i];
            vecChannels.emplace_back(make_tuple(item.nIndex, item.pDispWidget->m_nDevId));
        }
    } else {
        auto &item = m_listPreviewWidgets[m_nMaxWidgetIndex];
        vecChannels.emplace_back(make_tuple(item.nIndex, item.pDispWidget->m_nDevId));

    }
    return vecChannels;
}

void SplitWidgetMgr::ChangeSplitWidgets(SPLIT_TYPE eSplitType) {
    if (eSplitType != SPLIT_TYPE::MAX) {
        m_eCurrentSplitType = eSplitType;
    }
    m_nCurrentStartIndex = 0;

    this->change_view_videos();
}

bool SplitWidgetMgr::ChangePrevNextWidgets(PREV_NEXT_TYPE ePrevNextType, bool bTest) {

    bool bRet = false;
    do {

        const int nStep = this->get_channel_step(m_eCurrentSplitType);
        if (nStep < 1) {
            break;
        }

        int nCurrentStartIndex = m_nCurrentStartIndex;
        if (PREV_NEXT_TYPE::PREV == ePrevNextType) {
            if (m_nCurrentStartIndex == 0) break;

            nCurrentStartIndex = nCurrentStartIndex - nStep;
            if (nCurrentStartIndex < 0) {
                break;
            }
        }
        else if (PREV_NEXT_TYPE::NEXT == ePrevNextType) {
            if (m_nCurrentStartIndex == MAX_DEVICE_COUNT - nStep) break;
            nCurrentStartIndex = nCurrentStartIndex + nStep;
            if (nCurrentStartIndex >= MAX_DEVICE_COUNT) {
                break;
            }
        } else {
            break;
        }

        if (!bTest) {
            m_nCurrentStartIndex = nCurrentStartIndex;
            this->change_view_videos();
        }
        bRet = true;
    } while (0);

    return bRet;
}

void SplitWidgetMgr::MinMaxSelected(int nWighetIndexSeled) {
    if (nWighetIndexSeled < 0 || nWighetIndexSeled > 63) {
        return;
    }

    if (m_nMaxWidgetIndex == -1)  {     // MAX
        auto &item = m_listPreviewWidgets[nWighetIndexSeled];
        if (item.pDispWidget != nullptr) {
            this->hide_view_videos();
            m_nMaxWidgetIndex = item.nIndex;
            this->visible_widget(item, true, 0, 0);
            item.pDispWidget->Restore();
        }
    } else {                            // MIN
        auto &item = m_listPreviewWidgets[m_nMaxWidgetIndex];
        this->change_view_videos();
        item.pDispWidget->Restore();
        m_nMaxWidgetIndex = -1;
    }
}

// int SplitWidgetMgr::MaxWidgetSelected(QLabel *pWidget) {
//     int nIndex = m_nMaxWidgetIndex;
//     if (m_nMaxWidgetIndex == -1) {
//         this->hide_view_videos();
//         for (auto &wdg: m_listPreviewWidgets) {
//             if (wdg.pDispWidget != pWidget) continue;
//             m_nMaxWidgetIndex = wdg.nIndex;
//             this->visible_widget(wdg, true, 0, 0);
//             break;
//         }
//         return m_nMaxWidgetIndex;
//     }
//     m_nMaxWidgetIndex = -1;
//     this->change_view_videos();
//     return m_nMaxWidgetIndex;
// }

void SplitWidgetMgr::init()
{
    QStringList qss;
    qss.append("QFrame{border:2px solid #000000;}");
    // qss.append("ScaleLabel{font:75 25px;color:#F0F0F0;border:none;background-color: rgba(255, 117, 125, 65);}");
    qss.append("ScaleLabel{font:75 25px;color:#F0F0F0;border:1px solid #959595;}");
    if (m_enViewType != AX_NVR_VIEW_TYPE::POLLING && m_enViewType != AX_NVR_VIEW_TYPE::PIP) {
        qss.append("ScaleLabel:focus{border:2px solid #00BB9E;}");
    }
    m_pParentWidget->setStyleSheet(qss.join(""));

    m_nMaxWidgetIndex = -1;

    for (int i = 0; i < MAX_DEVICE_COUNT; i++) {

        NVR_PREVIEW_WIDGET_T preview_widget;

        preview_widget.nIndex = i;
        if (AX_NVR_VIEW_TYPE::PLAYBACK == m_enViewType) {
            preview_widget.pDispWidget = new ScaleLabel(-1, m_enViewType, m_pParentWidget);
        }
        else if (AX_NVR_VIEW_TYPE::PIP == m_enViewType) {
            preview_widget.pDispWidget = new ScaleLabel(-1, m_enViewType, m_pParentWidget);
        }
        else {
            preview_widget.pDispWidget = new ScaleLabel(i, m_enViewType, m_pParentWidget);
        }

        preview_widget.pDispWidget->setObjectName(QString("video%1").arg(i + 1));
        preview_widget.pDispWidget->installEventFilter(m_pParentWidget);
        preview_widget.pDispWidget->setFocusPolicy(Qt::StrongFocus);
        preview_widget.pDispWidget->setAlignment(Qt::AlignTop|Qt::AlignLeft);
        if (m_enViewType != AX_NVR_VIEW_TYPE::PLAYBACK && m_enViewType != AX_NVR_VIEW_TYPE::PIP) {
        // if (m_enViewType != AX_NVR_VIEW_TYPE::PLAYBACK) {
            preview_widget.pDispWidget->setText(QString("CHN %1").arg(i + 1));
        }

        m_listPreviewWidgets.append(preview_widget);
    }

    m_listPreviewWidgets[0].pDispWidget->setFocus();
}

void SplitWidgetMgr::SetChannelsText(PLAYBACK_ACTION_TYPE enActionType, const ax_nvr_channel_vector &vecChannels, const QString& strAppendText /*= ""*/) {
    for (auto &chn: vecChannels) {
        AX_NVR_DEV_ID devid = get<1>(chn);
        int i = get<0>(chn);
        if (PLAYBACK_ACTION_TYPE::PLAY == enActionType) {
            m_listPreviewWidgets[i].pDispWidget->m_nDevId = devid;
            m_listPreviewWidgets[i].pDispWidget->setText(QString("CHN %1").arg(devid + 1));
        } else if (PLAYBACK_ACTION_TYPE::STOP == enActionType) {
            m_listPreviewWidgets[i].pDispWidget->m_nDevId = -1;
            m_listPreviewWidgets[i].pDispWidget->setText(QString(""));
        } else if (PLAYBACK_ACTION_TYPE::FAST_SPEED == enActionType || PLAYBACK_ACTION_TYPE::SLOW_SPEED == enActionType) {
            m_listPreviewWidgets[i].pDispWidget->setText(QString("CHN %1 %2").arg(devid + 1).arg(strAppendText));
        }
    }
}

void SplitWidgetMgr::deinit() {
    for (int i = 0; i < MAX_DEVICE_COUNT; i++) {
        ScaleLabel *pDispWidget = m_listPreviewWidgets.at(i).pDispWidget;
        delete pDispWidget;
        pDispWidget = nullptr;
    }
}

void SplitWidgetMgr::change_view_videos() {
    m_nMaxWidgetIndex = -1;
    this->hide_view_videos();
    switch (m_eCurrentSplitType)
    {
    case SPLIT_TYPE::ONE:
    case SPLIT_TYPE::FOUR:
    case SPLIT_TYPE::SIXTEEN:
    case SPLIT_TYPE::THIRTYSIX:
    case SPLIT_TYPE::SIXTYFOUR:
        this->change_video(m_nCurrentStartIndex, m_eCurrentSplitType);
        break;
    case SPLIT_TYPE::EIGHT:
        this->change_video_8(m_nCurrentStartIndex);
        break;
    default:
        break;
    }
}

void SplitWidgetMgr::change_video_8(int index)
{
    int nTempIndex = index;
    this->visible_widget(m_listPreviewWidgets[nTempIndex++], true, 0, 0, 3, 3);
    this->visible_widget(m_listPreviewWidgets[nTempIndex++], true, 0, 3, 1, 1);
    this->visible_widget(m_listPreviewWidgets[nTempIndex++], true, 1, 3, 1, 1);
    this->visible_widget(m_listPreviewWidgets[nTempIndex++], true, 2, 3, 1, 1);
    this->visible_widget(m_listPreviewWidgets[nTempIndex++], true, 3, 0, 1, 1 );
    this->visible_widget(m_listPreviewWidgets[nTempIndex++], true, 3, 1, 1, 1);
    this->visible_widget(m_listPreviewWidgets[nTempIndex++], true, 3, 2, 1, 1);
    this->visible_widget(m_listPreviewWidgets[nTempIndex++], true, 3, 3, 1, 1);

    m_listPreviewWidgets[index].pDispWidget->setFocus();
}

void SplitWidgetMgr::change_video(int index, SPLIT_TYPE eSplitType)
{
    int count = 0;
    int row = 0;
    int column = 0;
    for (int i = 0; i < MAX_DEVICE_COUNT; i++) {
        if (i >= index) {
            this->visible_widget(m_listPreviewWidgets[i], true, row, column);
            count++;
            column++;
            if (column == (int)eSplitType) {
                row++;
                column = 0;
            }
        }

        if (count == ((int)eSplitType * (int)eSplitType)) {
            break;
        }
    }

    for (auto it : m_listPreviewWidgets) {
        it.pDispWidget->EnableZoom(AX_FALSE);
        if (it.bDisp && eSplitType == SPLIT_TYPE::ONE) {
            it.pDispWidget->EnableZoom(AX_TRUE);
        }
    }

    m_listPreviewWidgets[index].pDispWidget->setFocus();
}

void SplitWidgetMgr::hide_view_videos() {
    m_nMaxWidgetIndex = -1;
    for (int i = 0; i < MAX_DEVICE_COUNT; i++) {
        this->visible_widget(m_listPreviewWidgets[i], false);
    }
}

void SplitWidgetMgr::visible_widget(NVR_PREVIEW_WIDGET_T &disp, bool bVisible,
                                    int row/*=-1*/, int column/*=-1*/, int rowSpan/*=-1*/, int columnSpan/*=-1*/) {

    disp.bDisp = bVisible;
    ScaleLabel *pDispWidget = disp.pDispWidget;
    pDispWidget->setVisible(bVisible);
    pDispWidget->EnableZoom(bVisible);
    pDispWidget->Reset();
    if (bVisible) {
        // if (rowSpan == -1 && columnSpan == -1 && row == -1 && column == -1) {
        //     m_pGridLayout->addWidget(pDispWidget);
        // }
        // else
        if (rowSpan == -1 && columnSpan == -1) {
            m_pGridLayout->addWidget(pDispWidget, row, column);
        }
        else {
            m_pGridLayout->addWidget(pDispWidget, row, column, rowSpan, columnSpan);
        }
    }
    else {
        m_pGridLayout->removeWidget(pDispWidget);
    }
}

int SplitWidgetMgr::get_channel_step(SPLIT_TYPE enSplitType) const {

    int nStep = 1;
    switch (enSplitType)
    {
    case SPLIT_TYPE::ONE:
        nStep = 1;
        break;
    case SPLIT_TYPE::FOUR:
        nStep = 4;
        break;
    case SPLIT_TYPE::EIGHT:
        nStep = 8;
        break;
    case SPLIT_TYPE::SIXTEEN:
        nStep = 16;
        break;
    case SPLIT_TYPE::THIRTYSIX:
        nStep = 36;
        break;
    case SPLIT_TYPE::SIXTYFOUR:
        nStep = 64;
        break;
    default:
        break;
    }
    return nStep;
}