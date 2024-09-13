#pragma once
#include <QWidget>
#include <QGridLayout>
#include <vector>
#include <tuple>

using namespace std;

#include "global/UiGlobalDef.h"
#include "AXNVRFrameworkDefine.h"
#include "ScaleLabel.h"


typedef struct _NVR_PREVIEW_WIDGET_T {
    int nIndex = -1;
    ScaleLabel *pDispWidget = nullptr;
    bool bDisp = false;
    NVR_RECT_T rect;
} NVR_PREVIEW_WIDGET_T;

class SplitWidgetMgr : public QObject {

    Q_OBJECT

public:
    SplitWidgetMgr(AX_NVR_VIEW_TYPE enViewType, QGridLayout *gridLayout, QWidget *parent);
    ~SplitWidgetMgr();
    void Reset(void);

    void MinMaxSelected(int nWighetIndexSeled);

    const vector<AX_NVR_RECT_T> CalcLayouts(SPLIT_TYPE enSplitType,
                                            int nWidth, int nHeight,
                                            int nLeftMargin, int nTopMargin, int nRightMargin, int nBottomMargin) const;

    const ax_nvr_channel_vector GetViewChannels(SPLIT_TYPE enSplitType,
                                            PREV_NEXT_TYPE enPrevNextType = PREV_NEXT_TYPE::UNKNOW) const;
    const vector<AX_NVR_DEV_ID> GetViewChns(SPLIT_TYPE enSplitType,
                                            PREV_NEXT_TYPE enPrevNextType = PREV_NEXT_TYPE::UNKNOW) const;
    int GetWidgetIndex(QWidget *pWidget);
    int GetWidgetDevid(QWidget *pWidget);
    QWidget *GetCurrentStartWidget() {
        return m_listPreviewWidgets[m_nCurrentStartIndex].pDispWidget;
    };
    // const vector<AX_NVR_DEV_ID> IndexMapDevID(const vector<int>);

    SPLIT_TYPE GetCurrentSplitType() const { return m_eCurrentSplitType; };
    void ChangeSplitWidgets(SPLIT_TYPE eSplitType);
    bool ChangePrevNextWidgets(PREV_NEXT_TYPE ePrevNextType, bool bTest=false);
    void SetChannelsText(PLAYBACK_ACTION_TYPE enActionType, const ax_nvr_channel_vector &vecChannels, const QString& strAppendText = "");
    QList<NVR_PREVIEW_WIDGET_T>& GetPreviewWidgetList() { return m_listPreviewWidgets; };

    AX_NVR_VIEW_TYPE m_enViewType = AX_NVR_VIEW_TYPE::PREVIEW;
    int m_nMaxWidgetIndex = -1;

private:
    QGridLayout *m_pGridLayout {nullptr};
    QWidget *m_pParentWidget {nullptr};

    int m_nCurrentStartIndex {0};
    int m_nCurrentIndex {-1};
    SPLIT_TYPE m_eSupportSplitType {SPLIT_TYPE::SIXTYFOUR};
    SPLIT_TYPE m_eCurrentSplitType {SPLIT_TYPE::SIXTYFOUR};

    QList<NVR_PREVIEW_WIDGET_T> m_listPreviewWidgets;

private:
    int get_channel_step(SPLIT_TYPE enSplitType) const;

    void init(void);
    void deinit(void);

    void hide_view_videos(void);
    void change_view_videos(void);
    void change_video(int index, SPLIT_TYPE enSplitType);
    void change_video_8(int index);
    void visible_widget(NVR_PREVIEW_WIDGET_T &disp, bool bVisible, int row=-1, int column=-1, int rowSpan=-1, int columnSpan=-1);
};