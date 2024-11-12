/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#ifndef DLGDEVINFO_H
#define DLGDEVINFO_H

#include <QDialog>
#include "AXNVRFrameworkDefine.h"
#include "PreviewTopToolbar.h"
#include "global/UiGlobalDef.h"
#include "utils/SplitWidgetMgr.h"

QT_BEGIN_NAMESPACE
namespace Ui { class PreviewPip; }
QT_END_NAMESPACE


class PreviewPip : public QDialog
{
    Q_OBJECT

public:
    PreviewPip(QWidget *parent = nullptr);
    ~PreviewPip();

    void ShowPip(PreviewTopToolbar *pPreviewTopToolbar);
    void ClosePip();
    AX_NVR_RECT_T &GetDispRect();

protected:
    virtual void paintEvent(QPaintEvent *event) override;
    // virtual void resizeEvent(QResizeEvent *event) override;
    // virtual void showEvent(QShowEvent *event) override;

private:
    Ui::PreviewPip *ui;
    AX_NVR_RECT_T m_rect;
    SplitWidgetMgr *m_pSplitWidgetMgr = nullptr;

    PreviewTopToolbar *m_pPreviewTopToolbar = nullptr;

    // int m_nLeftMargin = 9;
    // int m_nTopMargin = 9;
    // int m_nRightMargin = 9;
    // int m_nBottomMargin = 9;
    // int m_nWidth = 0;
    // int m_nHeight = 0;
    // int m_nXOffset = 0;
    // int m_nYOffset = 0;

};
#endif // DLGDEVINFO_H
