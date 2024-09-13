#ifndef PLAYBACKLEFTTOOLBAR_H
#define PLAYBACKLEFTTOOLBAR_H

#include <QWidget>
#include <QIcon>

#include <vector>

#include "utils/SplitWidgetMgr.h"
#include "PlaybackTimeLine.h"

using namespace std;

QT_BEGIN_NAMESPACE
namespace Ui { class PlaybackLeftToolbar; }
QT_END_NAMESPACE

class PlaybackLeftToolbar : public QWidget
{
    Q_OBJECT

public:
    PlaybackLeftToolbar(QWidget *parent = nullptr);
    ~PlaybackLeftToolbar();

protected:
    virtual void showEvent(QShowEvent *event) override;
    virtual void resizeEvent(QResizeEvent *event) override;

private:
    Ui::PlaybackLeftToolbar *ui;
    QIcon m_iconChecked;
    QIcon m_iconUnchecked;

public slots:
    void onTableCellClicked(int row, int col);

protected slots:
    void onTabCurrentChanged(int index);
    void onCalendarChanged(void);
    void onClickedRefresh(void);

signals:
    void signal_change_split(SPLIT_TYPE enSpliteType);
    void signal_playback_action(PLAYBACK_ACTION_TYPE enActionType);

public:
    void SetTimeline(PlaybackTimeLine *pPlaybackTimeLine);

    std::string GetCurrentYMD(void);
    AX_U32 GetCurrentIntYMD(void);
    void SetCurrentYMD(bool bNext);
    ax_nvr_channel_vector GetChannelsSelected(void);

private:
    std::vector<AX_NVR_DEV_ID> m_vecDevInfo;

    int m_nCnt1x1 = 0;
    int m_nCnt2x2 = 0;
    int m_nCnt1_7 = 0;
    int m_nCnt4_4 = 0;

    void setTableData(void);
    void dumpStructure(const QObject *obj, int spaceCount);

};
#endif // PLAYBACKLEFTTOOLBAR_H
