#ifndef PLAYBACKTIMELINE_H
#define PLAYBACKTIMELINE_H

#include <QWidget>
#include <QMap>
#include <QWidget>
// #include "utils/VideoListTimeLine.h"
#include <string>
#include "AXNVRFrameworkDefine.h"

using namespace std;

QT_BEGIN_NAMESPACE
namespace Ui { class PlaybackTimeLine; }
QT_END_NAMESPACE

struct st_playbackdate
{
    int type; // 进度条颜色类型
    int start;
    int end;
};

struct TimePoint
{
    int hour;
    int minitue;
    int second;
};


class PlaybackTimeLine : public QWidget
{
    Q_OBJECT

public:
    PlaybackTimeLine(QWidget *parent = nullptr);
    ~PlaybackTimeLine();

private:
    Ui::PlaybackTimeLine *ui;

protected:
    virtual void paintEvent(QPaintEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *) override;
    virtual void mouseMoveEvent(QMouseEvent * event) override;
    virtual void mouseReleaseEvent(QMouseEvent *e) override;
    virtual void resizeEvent(QResizeEvent *event) override;

public:
    AX_U32 GetCurrentHMS();
    void Update(); // for test
    void UpdateTimeline(const ax_nvr_channel_vector &vecViewChn, const string &strYYYYmmdd);
    void Locate(int nHHMM);

private:
    QList<QRectF> convertTimeSegmentToRect();
    void setVideoListTime(QList<QPair<QTime, QTime> > recordList);
    void setTime(int);  // ShowCurrentTime

    int m_n24HoursWidth = 0;
    int m_LeftMargin = 30;
    int m_RightMargin = 30;
    int m_TopMargin = 5;
    int m_BottomMargin = 5;
    int timeLineHeight = 25;

    // Slider Bar Control
    QRect m_rectTimeLineCtrl;
    QLine m_Line;
    int m_xline = m_LeftMargin - 5;
    bool m_bTimeLineCtrlClicked {false};

    QString m_ShowCurrentTime = "00:00:00";
    AX_U32 m_nCurrentTime = 0;
    QList<QPair<QTime, QTime>> videoList;

    bool m_bUpdateTest = true;
};
#endif // PLAYBACKTIMELINE_H
