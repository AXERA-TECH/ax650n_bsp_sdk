#include "ScaleLabel.h"
#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QRect>
#include <QLabel>
#include <stdlib.h>

#include "AppLogApi.h"
#include "AXNVRFramework.h"

#define TAG "CHNLAB"

#ifndef ALIGN_UP
#define ALIGN_UP(x, align) (((x) + ((align)-1)) & ~((align)-1))
#endif

#ifndef ALIGN_DOWN
#define ALIGN_DOWN(x, align) ((x) & ~((align)-1))
#endif

ScaleLabel::ScaleLabel(AX_NVR_DEV_ID nDevId, AX_NVR_VIEW_TYPE enViewType, QWidget *parent)
    : QLabel(parent)
    , m_nDevId(nDevId)
    , m_enViewType(enViewType) {

}

ScaleLabel::~ScaleLabel() {

}


AX_NVR_RECT_T ScaleLabel::CalcMoveRect() {
    AX_NVR_RECT_T rectCrop;

    do {
        AX_U32 nImageW = 0;
        AX_U32 nImageH = 0;
        CAXNVRFramework::GetInstance()->GetResolution(m_nDevId, m_enViewType, nImageW, nImageH);
        if (nImageW == 0 || nImageH == 0) {
            break;
        }

        float x_y = (float)this->width() / (float)this->height();
        float x_scale_factor = (float)this->width() / nImageW;
        float y_scale_factor = (float)this->height() / nImageH;

        m_nLabelCropX -= (m_ptEnd.x() - m_ptPre.x());
        m_nLabelCropY -= (m_ptEnd.y() - m_ptPre.y());

        if (m_nLabelCropX < 0) m_nLabelCropX = 0;
        if (m_nLabelCropY < 0) m_nLabelCropY = 0;
        if (m_nLabelCropX + m_nLabelCropW > this->width()) m_nLabelCropX = this->width() - m_nLabelCropW;
        if (m_nLabelCropY + m_nLabelCropH > this->height()) m_nLabelCropY = this->height() - m_nLabelCropH;

        rectCrop.x = ALIGN_DOWN(int(m_nLabelCropX / x_scale_factor), 2);
        rectCrop.y = ALIGN_DOWN(int(m_nLabelCropY / y_scale_factor), 2);
        rectCrop.w = ALIGN_DOWN(int(m_nLabelCropW / x_scale_factor), 2);
        rectCrop.h = ALIGN_DOWN(int(m_nLabelCropH / y_scale_factor), 2);

    #if 0
        LOG_MM_C(TAG, "move (%d, %d) -> (%d, %d), label: (%d, %d) %dx%d, %dx%d, img: %dx%d => (%d, %d) %dx%d", \
                        m_ptPre.rx(), m_ptPre.ry(), m_ptEnd.rx(), m_ptEnd.ry(), \
                        m_nLabelCropX, m_nLabelCropY, m_nLabelCropW, m_nLabelCropH,
                        width(), height(), nImageW, nImageH, \
                        rectCrop.x, rectCrop.y, rectCrop.w, rectCrop.h);
    #endif

    } while(0);

    return rectCrop;
}

AX_NVR_RECT_T ScaleLabel::CalcCropRect() {
    AX_NVR_RECT_T rectCrop;

    do {

        AX_U32 nImageW = 0;
        AX_U32 nImageH = 0;
        CAXNVRFramework::GetInstance()->GetResolution(m_nDevId, m_enViewType, nImageW, nImageH);
        if (nImageW == 0 || nImageH == 0) {
            break;
        }

        float x_y = (float)this->width() / (float)this->height();

        float x_scale_factor = (float)this->width() / nImageW;
        float y_scale_factor = (float)this->height() / nImageH;
        m_nLabelCropX = m_ptBegin.rx() < m_ptEnd.rx() ? m_ptBegin.rx() : m_ptEnd.rx();
        m_nLabelCropY = m_ptBegin.ry() < m_ptEnd.ry() ? m_ptBegin.ry() : m_ptEnd.ry();
        m_nLabelCropW = abs(m_ptEnd.rx() - m_ptBegin.rx());
        m_nLabelCropH = abs(m_ptEnd.ry() - m_ptBegin.ry());

        if (this->width()/(float)m_nLabelCropW < this->height()/(float)m_nLabelCropH) {
            m_nLabelCropH = m_nLabelCropW / x_y;
        } else {
            m_nLabelCropW = m_nLabelCropH * x_y;
        }

        if (m_nLabelCropX < 0) m_nLabelCropX = 0;
        if (m_nLabelCropY < 0) m_nLabelCropY = 0;
        if (m_nLabelCropX + m_nLabelCropW > this->width()) m_nLabelCropX = this->width() - m_nLabelCropW;
        if (m_nLabelCropY + m_nLabelCropH > this->height()) m_nLabelCropY = this->height() - m_nLabelCropH;

        rectCrop.x = ALIGN_DOWN(int(m_nLabelCropX / x_scale_factor), 2);
        rectCrop.y = ALIGN_DOWN(int(m_nLabelCropY / y_scale_factor), 2);
        rectCrop.w = ALIGN_DOWN(int(m_nLabelCropW / x_scale_factor), 2);
        rectCrop.h = ALIGN_DOWN(int(m_nLabelCropH / y_scale_factor), 2);

    #if 0
        LOG_MM_C(TAG, "crop (%d, %d) -> (%d, %d), %dx%d, img: %dx%d => (%d, %d) %dx%d", \
                        m_ptBegin.rx(), m_ptBegin.ry(), m_ptEnd.rx(), m_ptEnd.ry(), \
                        width(), height(), nImageW, nImageH, \
                        rectCrop.x, rectCrop.y, rectCrop.w, rectCrop.h);
    #endif
    } while(0);

    return rectCrop;
}

void ScaleLabel::Reset() {
    if (m_bMove) {
        m_nLastWidth = this->width();
        m_nLastHeight = this->height();
    }

    m_bMove = false;
    m_bMousePress = false;
    m_bMouseMove = false;
    m_ptBegin = QPoint(0, 0);
    m_ptEnd = QPoint(0, 0);
    m_ptPre = QPoint(0, 0);
}

void ScaleLabel::Restore() {
    m_bMove = m_bLastMove;
}

void ScaleLabel::mousePressEvent(QMouseEvent *ev) {
    if (ev->button() == Qt::RightButton) {
        m_bMousePress = true;
        m_bMouseMove = false;
        m_ptBegin = ev->pos();
        if (m_bMove) {
            m_ptPre = ev->pos();
            setCursor(Qt::OpenHandCursor);
        }
    }
}

void ScaleLabel::mouseReleaseEvent(QMouseEvent *ev) {
    if (ev->button() == Qt::RightButton) {
        m_ptEnd = ev->pos();
        if (m_ptEnd.rx() < 0) m_ptEnd.setX(0);
        if (m_ptEnd.rx() > this->width()) m_ptEnd.setX(this->width());
        if (m_ptEnd.ry() < 0) m_ptEnd.setY(0);
        if (m_ptEnd.ry() > this->height()) m_ptEnd.setY(this->height());

        if (m_bMousePress && m_bMouseMove && !m_bMove) {
            AX_BOOL bWatcherWorking = AX_FALSE;
            do {
                if (!m_watcherZoomAndMove.isFinished()) {
                    LOG_M_I(TAG, "[%s][0x%x] Still Active.", __func__, QThread::currentThread());
                    break;
                }

                AX_NVR_RECT_T rectCrop = this->CalcCropRect();
                if (rectCrop.w <= 0 || rectCrop.h <= 0) {
                    break;
                }

                AX_NVR_DEV_ID nDevId = m_nDevId;

                QFutureInterface<int> interface;
                interface.reportStarted();
                m_watcherZoomAndMove.setFuture(interface.future());
                bWatcherWorking = AX_TRUE;
                std::thread thread([this, interface, nDevId, rectCrop]() mutable {
                    LOG_M_D(TAG, "[%s][0x%x] +++", __func__, QThread::currentThread());

                    AX_BOOL bRectSelectOK = AX_FALSE;
                    if (AX_NVR_VIEW_TYPE::PREVIEW == m_enViewType) {
                        CAXNVRPreviewCtrl *pPreview = CAXNVRFramework::GetInstance()->PreviewCtrl();
                        if (nullptr != pPreview) {
                            bRectSelectOK = pPreview->ZoomAndMove(nDevId, rectCrop, AX_TRUE);
                        }
                    }
                    else if (AX_NVR_VIEW_TYPE::PLAYBACK == m_enViewType) {
                        CAXNVRPlaybakCtrl *pPlayback = CAXNVRFramework::GetInstance()->PlaybakCtrl();
                        if (nullptr != pPlayback) {
                            bRectSelectOK = pPlayback->ZoomAndMove(nDevId, rectCrop, AX_TRUE);
                        }
                    }

                    interface.reportResult(bRectSelectOK);
                    interface.reportFinished();

                    LOG_M_D(TAG, "[%s][0x%x] ---", __func__, QThread::currentThread());
                });

                // detach thred, Use Future to ensure exit
                thread.detach();
            } while(0);

            int nZoomAndMoveRet = 0;
            if (bWatcherWorking) {
                m_watcherZoomAndMove.waitForFinished();
                nZoomAndMoveRet = m_watcherZoomAndMove.result();
            }

            if (nZoomAndMoveRet) {
                m_bMove = true;
                m_bLastMove = m_bMove;
            }

            update();
        }
        m_bMousePress = false;
        m_bMouseMove = false;
        setCursor(Qt::ArrowCursor);
    }
}

void ScaleLabel::mouseMoveEvent(QMouseEvent *ev) {
    if (ev->button() == Qt::RightButton) {
        if (m_bMove ) {
            do {
                if (!m_watcherZoomAndMove.isFinished()) {
                    LOG_M_I(TAG, "[%s][0x%x] Still Active.", __func__, QThread::currentThread());
                    break;
                }

                QFutureInterface<int> interface;
                interface.reportStarted();
                m_watcherZoomAndMove.setFuture(interface.future());

                if (m_nLastWidth != 0  && m_nLastHeight != 0) {
                    m_nLabelCropW = m_nLabelCropW * this->width() / m_nLastWidth;
                    m_nLabelCropH = m_nLabelCropH * this->height() / m_nLastHeight;
                    m_nLabelCropX = m_nLabelCropX * this->width() / m_nLastWidth;
                    m_nLabelCropY = m_nLabelCropY * this->height() / m_nLastHeight;
                    m_nLastWidth = 0;
                    m_nLastHeight = 0;
                }

                AX_NVR_DEV_ID nDevId = m_nDevId;
                m_ptEnd = ev->pos();
                AX_NVR_RECT_T rectCrop = this->CalcMoveRect();
                m_ptPre = ev->pos();
                std::thread thread([this, interface, nDevId, rectCrop]() mutable {
                    LOG_M_D(TAG, "[%s][0x%x] +++", __func__, QThread::currentThread());

                    if (AX_NVR_VIEW_TYPE::PREVIEW == m_enViewType) {
                        CAXNVRPreviewCtrl *pPreview = CAXNVRFramework::GetInstance()->PreviewCtrl();
                        if (nullptr != pPreview) {
                            pPreview->ZoomAndMove(nDevId, rectCrop, AX_TRUE);
                        }
                    }
                    else if (AX_NVR_VIEW_TYPE::PLAYBACK == m_enViewType) {
                        CAXNVRPlaybakCtrl *pPlayback = CAXNVRFramework::GetInstance()->PlaybakCtrl();
                        if (nullptr != pPlayback) {
                            pPlayback->ZoomAndMove(nDevId, rectCrop, AX_TRUE);
                        }
                    }

                    interface.reportResult(1);
                    interface.reportFinished();

                    LOG_M_D(TAG, "[%s][0x%x] ---", __func__, QThread::currentThread());
                });

                // detach thred, Use Future to ensure exit
                thread.detach();

            } while(0);

        } else{

            m_bMouseMove = true;
            m_ptEnd = ev->pos();
            if (m_ptEnd.rx() < 0) m_ptEnd.setX(0);
            if (m_ptEnd.rx() > this->width()) m_ptEnd.setX(this->width());
            if (m_ptEnd.ry() < 0) m_ptEnd.setY(0);
            if (m_ptEnd.ry() > this->height()) m_ptEnd.setY(this->height());
        }

        update();
    }
}

void ScaleLabel::paintEvent(QPaintEvent *ev)
{
    QLabel::paintEvent(ev);

    if (m_enViewType == AX_NVR_VIEW_TYPE::POLLING || m_enViewType == AX_NVR_VIEW_TYPE::PIP) {
        return;
    }

    m_painter.begin(this);
    m_painter.setPen(QPen(Qt::red, 2));
    if (m_bMouseMove && !m_bMove) {
        m_painter.drawRect(QRect(m_ptBegin, m_ptEnd));
    }
    m_painter.end();
}

void ScaleLabel::mouseDoubleClickEvent(QMouseEvent *ev) {
    if (ev->button() == Qt::RightButton) {
        do {
            m_bMove = false;
            m_bLastMove = m_bMove;

            if (!m_watcherZoomAndMove.isFinished()) {
                LOG_M_I(TAG, "[%s][0x%x] Still Active.", __func__, QThread::currentThread());
                break;
            }

            QFutureInterface<int> interface;
            interface.reportStarted();
            m_watcherZoomAndMove.setFuture(interface.future());

            AX_NVR_DEV_ID nDevId = m_nDevId;
            AX_NVR_RECT_T rectCrop = {1, 0, 0, 0, 0};

            std::thread thread([this, interface, nDevId, rectCrop]() mutable {
                LOG_M_D(TAG, "[%s][0x%x] +++", __func__, QThread::currentThread());

                if (AX_NVR_VIEW_TYPE::PREVIEW == m_enViewType) {
                    CAXNVRPreviewCtrl *pPreview = CAXNVRFramework::GetInstance()->PreviewCtrl();
                    if (nullptr != pPreview) {
                        pPreview->ZoomAndMove(nDevId, rectCrop, AX_FALSE);
                    }
                }
                else if (AX_NVR_VIEW_TYPE::PLAYBACK == m_enViewType) {
                    CAXNVRPlaybakCtrl *pPlayback = CAXNVRFramework::GetInstance()->PlaybakCtrl();
                    if (nullptr != pPlayback) {
                        pPlayback->ZoomAndMove(nDevId, rectCrop, AX_FALSE);
                    }
                }

                interface.reportResult(1);
                interface.reportFinished();

                LOG_M_D(TAG, "[%s][0x%x] ---", __func__, QThread::currentThread());
            });

            // detach thred, Use Future to ensure exit
            thread.detach();

        } while(0);
    }
}

void ScaleLabel::resizeEvent(QResizeEvent *event) {
    /* update current window size */
    m_nLastWidth = width();
    m_nLastHeight = height();
    return QWidget::resizeEvent(event);
}
