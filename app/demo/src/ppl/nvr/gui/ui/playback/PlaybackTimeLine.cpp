#include "PlaybackTimeLine.h"
#include "ui_PlaybackTimeLine.h"
#include <qpainter.h>
#include <QImage>
#include <QMouseEvent>
#include <QDebug>
#include <QPainter>
#include <QTime>

#include "AXNVRPlaybackCtrl.h"
#include "AXNVRFramework.h"
#include "AXNVRFrameworkDefine.h"

const int SECONDS_IN_DAY = 60 * 60 * 24;


PlaybackTimeLine::PlaybackTimeLine(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PlaybackTimeLine)
{
    ui->setupUi(this);
}

PlaybackTimeLine::~PlaybackTimeLine()
{
    delete ui;
}

void PlaybackTimeLine::UpdateTimeline(const ax_nvr_channel_vector &vecViewChn, const string &strYYYYmmdd) {

    auto __GET_HSM__ = [](auto value) {
        time_t timep = value;
        struct tm *local_time = localtime(&timep);
        return QTime(local_time->tm_hour, local_time->tm_min, local_time->tm_sec);
    };


    QList<QPair<QTime, QTime>> videoListSegment;
    // FixMe: thread?
    CAXNVRPlaybakCtrl *pPlayback = CAXNVRFramework::GetInstance()->PlaybakCtrl();
    if (pPlayback == nullptr) {
        return;
    }
    for (auto chn: vecViewChn) {
        AX_NVR_DEV_ID devid = get<1>(chn);
        std::vector<AXDS_VIDEO_INFO_T> vecDevicInfo = pPlayback->GetDeviceFileInfo(devid, strYYYYmmdd);
        for (auto& info: vecDevicInfo) {
            AX_U64 start = info.tStart.uSec;
            AX_U64 end = info.tEnd.uSec;
            videoListSegment.append(qMakePair(__GET_HSM__(start), __GET_HSM__(end)));
        }
    }

    this->setVideoListTime(videoListSegment);
    this->update();
}

void PlaybackTimeLine::resizeEvent(QResizeEvent *event) {

    return QWidget::resizeEvent(event);
}

void PlaybackTimeLine::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    int nTempWidth = this->width() - (m_LeftMargin + m_RightMargin);
    int nHourWidth = nTempWidth / 24;
    m_n24HoursWidth = nHourWidth * 24;
    int Margin = m_LeftMargin;
    int nY = 50;
    int nLongH = 20;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // painter.begin(this);
    // draw 24Hours ruler
    painter.setPen(QPen(QColor(210, 210, 210), 0));
    painter.setBrush(QBrush(QColor(210, 210, 210)));
    for(int i = 0; i < 25; i++) {
        painter.drawLine(nHourWidth*i + Margin, nY, nHourWidth*i + Margin, nY - nLongH);
        painter.drawText(nHourWidth*i + Margin - 15, nY - nLongH - 2, QString::number(i) + ":00");
    }
    painter.drawLine(Margin, nY, Margin + m_n24HoursWidth, nY);

    // draw record background
    painter.setPen(QPen(QColor(108, 108, 108, 80), 0));
    painter.setBrush(QColor(108, 108, 108, 80));
    painter.drawRect(Margin, nY + 5, m_n24HoursWidth, timeLineHeight);

    // draw record
    QList<QRectF> timeRectList = convertTimeSegmentToRect();
    painter.setPen(QPen(QColor(102, 163, 52, 80), 0));
    painter.setBrush(QColor(102, 163, 52, 80));
    for (int i = 0; i < timeRectList.size(); i++)
    {
        QRectF rect = timeRectList.at(i);
        painter.drawRect(rect);
    }

    // draw time sliderbar-ctrl
    painter.setPen(QPen(QColor(210, 0, 0, 200), 0));
    painter.setBrush(QBrush(QColor(210, 0, 0, 200)));
    m_rectTimeLineCtrl = QRect(m_xline, 0, 10, 20);
    painter.drawRect(m_rectTimeLineCtrl);

    // draw time sliderbar-ctrl-time
    painter.drawText(m_xline , nY + 15, m_ShowCurrentTime);

    // draw time sliderbar-ctrl-line
    m_Line = QLine(m_xline + 5, 10, m_xline + 5, 500);
    painter.drawLine(m_Line);

    // painter.end();

}

void PlaybackTimeLine::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_rectTimeLineCtrl.contains(event->pos())) {
            m_bTimeLineCtrlClicked = true;
        }
    }
    this->update();
}

void PlaybackTimeLine::mouseMoveEvent(QMouseEvent * event)
{
    if (event->buttons() == Qt::LeftButton && m_bTimeLineCtrlClicked) {
        int x = event->pos().x();
        if ( x >= m_LeftMargin - 5 && x <= m_n24HoursWidth + m_LeftMargin - 5) {
            m_xline = x;
        }
        else if (x < m_LeftMargin - 5) {
            m_xline = m_LeftMargin - 5;
        }
        else if  (x > m_n24HoursWidth + m_LeftMargin - 5) {

            m_xline = m_n24HoursWidth + m_LeftMargin - 5;

        }
        setTime(m_xline);
    }
    this->update();
}

void PlaybackTimeLine::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_bTimeLineCtrlClicked) {
        m_bTimeLineCtrlClicked = false;
        // qDebug() << "PlaybackTimeLine::mouseReleaseEvent";
    }
    this->update();
}

void PlaybackTimeLine::setTime(int pos)
{
    TimePoint p;
    int second =  (pos - m_LeftMargin + 5) * 86400 / m_n24HoursWidth;
    p.hour = second /3600;
    p.minitue = (second % 3600) /60;
    p.second = second % 60;
    m_ShowCurrentTime = QString("%1").arg(p.hour, 2, 10, QLatin1Char('0'))
                + ":" + QString("%1").arg(p.minitue, 2, 10, QLatin1Char('0'))
                + ":" + QString("%1").arg(p.second, 2, 10, QLatin1Char('0'));
    m_nCurrentTime = p.hour * 10000 + p.minitue *100 + p.second;
    // qDebug() << "PlaybackTimeLine::setTime" << m_ShowCurrentTime;
}

void PlaybackTimeLine::Locate(int nHHMM) {
    int nHH = nHHMM / 100;
    int nMM = nHHMM % 100;

    m_xline = (nHH + nMM / 60.0) / 24 * m_n24HoursWidth + m_LeftMargin - 5;

    m_ShowCurrentTime = QString("%1").arg(nHH, 2, 10, QLatin1Char('0'))
                + ":" + QString("%1").arg(nMM, 2, 10, QLatin1Char('0'))
                + ":" + QString("%1").arg(0, 2, 10, QLatin1Char('0'));
    m_nCurrentTime = nHH * 10000 + nMM * 100 + 0;

    this->update();
}

void PlaybackTimeLine::setVideoListTime(QList<QPair<QTime, QTime>> recordList)
{
    videoList.clear();
    for (int i = 0; i < recordList.size(); i++) {
        if (recordList.at(i).first < recordList.at(i).second) {
            videoList.append(recordList.at(i));
        }
    }
}

QList<QRectF> PlaybackTimeLine::convertTimeSegmentToRect()
{
    int beginY = 50 + 5;
    int endY = beginY + timeLineHeight;
    int width = m_n24HoursWidth;

    QList<QRectF> timeRectSegment;
    for (int i=0; i< videoList.size(); i++)
    {
        double beginSec = videoList.at(i).first.msecsSinceStartOfDay() / 1000;
        double beginX = beginSec / SECONDS_IN_DAY * width;
        QPointF beginPoint = QPointF(m_LeftMargin + beginX, beginY);

        double endSec = videoList.at(i).second.msecsSinceStartOfDay() / 1000;
        double endX = endSec / SECONDS_IN_DAY * width;
        QPointF endPoint = QPointF(m_LeftMargin + endX, endY);

        timeRectSegment.append(QRectF(beginPoint, endPoint));
    }

    return timeRectSegment;
}

AX_U32 PlaybackTimeLine::GetCurrentHMS() {
    // qDebug() << "PlaybackTimeLine::GetCurrentHMS" << m_ShowCurrentTime;
    return m_nCurrentTime;
}