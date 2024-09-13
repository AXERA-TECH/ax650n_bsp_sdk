/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include "PreviewPip.h"
#include "ui_PreviewPip.h"
#include "AXNVRFramework.h"
#include "global/UiGlobalDef.h"
#include <QPainter>
#include <QDebug>


PreviewPip::PreviewPip(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PreviewPip) {

    ui->setupUi(this);
    // this->setStyleSheet("QDialog{border:10px solid rgb(44, 44, 44);}");
    this->setStyleSheet("QDialog{background-color:rgb(44, 44, 44)}");
    m_pSplitWidgetMgr = new SplitWidgetMgr(AX_NVR_VIEW_TYPE::PIP, ui->gridLayout, this);
}

PreviewPip::~PreviewPip() {
    DEL_PTR(m_pSplitWidgetMgr)
    delete ui;
}

// void PreviewPip::resizeEvent(QResizeEvent *event) {
//     auto __INIT_FUNC__ = [this]() {
//         // QPoint pt = this->mapToGlobal(QPoint(0, 0));
//         // m_nLeftMargin   = 0;//pt.x()%2;
//         // m_nTopMargin    = 0;//pt.y()%2;
//         // m_nRightMargin  = 0;
//         // m_nBottomMargin = 0;
//         m_nWidth        = this->geometry().width() - m_nLeftMargin - m_nRightMargin;
//         m_nHeight       = this->geometry().height() - m_nTopMargin - m_nBottomMargin;
//     };

//     __INIT_FUNC__();
//     const SPLIT_TYPE enSplitType = m_pSplitWidgetMgr->GetCurrentSplitType();
//     m_pSplitWidgetMgr->ChangeSplitWidgets(enSplitType);

//     return QWidget::resizeEvent(event);
// }

// void PreviewPip::showEvent(QShowEvent *event) {
//     do {

//         SPLIT_TYPE enSplitType = m_pSplitWidgetMgr->GetCurrentSplitType();
//         // change view split layout
//         m_pSplitWidgetMgr->ChangeSplitWidgets(enSplitType);

//     } while (0);

//     return QWidget::showEvent(event);
// }

void PreviewPip::ShowPip(PreviewTopToolbar *pPreviewTopToolbar) {
    // QPoint pt = ui->stackedWidget->mapToGlobal(QPoint(0, 0));
    // this->setGeometry(pt.x(), pt.y(), 792, 488);
    m_pPreviewTopToolbar = pPreviewTopToolbar;
    this->show();
}

void PreviewPip::ClosePip() {
    // QPoint pt = ui->stackedWidget->mapToGlobal(QPoint(0, 0));
    // this->setGeometry(pt.x(), pt.y(), 792, 488);
    if (m_pPreviewTopToolbar) {
        m_pPreviewTopToolbar->CheckPip(false);
    }
    this->close();
}

AX_NVR_RECT_T &PreviewPip::GetDispRect() {
    QPoint pt = ui->stackedWidget->mapToGlobal(QPoint(0, 0));
    QRect rect = ui->stackedWidget->geometry();

    m_rect.bShow = AX_TRUE;
    m_rect.x = pt.x();
    m_rect.y = pt.y();
    m_rect.w = rect.width();
    m_rect.h = rect.height();
    return m_rect;
}

void PreviewPip::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setCompositionMode(QPainter::CompositionMode_Clear);
    painter.fillRect(ui->stackedWidget->geometry(), Qt::SolidPattern);
}