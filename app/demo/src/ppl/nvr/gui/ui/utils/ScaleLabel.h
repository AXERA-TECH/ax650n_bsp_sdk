#pragma once
#include <QLabel>
#include <QPainter>
#include <QFutureWatcher>
#include <QFutureInterface>
#include <QThread>

#include "AXNVRFrameworkDefine.h"

class ScaleLabel : public QLabel
{
Q_OBJECT
private:
    virtual void mouseDoubleClickEvent(QMouseEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    // virtual void wheelEvent(QWheelEvent *envent) override;
    virtual void mouseReleaseEvent(QMouseEvent *ev) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    bool m_bZoomAndMove = false;
    int m_nLabelCropX = 0;
    int m_nLabelCropY = 0;
    int m_nLabelCropW = 0;
    int m_nLabelCropH = 0;

    int m_nLastWidth = 0;
    int m_nLastHeight = 0;

    QPainter m_painter;

    bool m_bMove = false;
    bool m_bMousePress = false;
    bool m_bMouseMove = false;
    bool m_bLastMove = false;
    QPoint m_ptBegin;
    QPoint m_ptEnd;
    QPoint m_ptPre;

    QFutureWatcher<int> m_watcherZoomAndMove;

public:
    ScaleLabel(AX_NVR_DEV_ID nDevId, AX_NVR_VIEW_TYPE enViewType, QWidget *parent = nullptr);
    ~ScaleLabel();
    void EnableZoom(bool bEnable) { m_bZoomAndMove = bEnable; };
    void Reset();
    void Restore();
    AX_NVR_RECT_T CalcCropRect();
    AX_NVR_RECT_T CalcMoveRect();

    AX_NVR_DEV_ID m_nDevId = -1;
    AX_NVR_VIEW_TYPE m_enViewType = AX_NVR_VIEW_TYPE::PREVIEW;

};