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

QT_BEGIN_NAMESPACE
namespace Ui { class DlgDevInfo; }
QT_END_NAMESPACE


class DlgDevInfo : public QDialog
{
    Q_OBJECT

public:
    DlgDevInfo(bool bEdit=false, QWidget *parent = nullptr);
    ~DlgDevInfo();

    const AX_NVR_DEV_INFO_T& GetDeviceInfo() const {return m_stDevInfo;} ;

protected:
    virtual void showEvent(QShowEvent *) override;

public:
    void SetDevInfo(const AX_NVR_DEV_INFO_T &stDevInfo);

protected slots:
    void accept() override;

// private:
//     void InitDlg();

private:
    Ui::DlgDevInfo *ui;

private:
    AX_NVR_DEV_INFO_T m_stDevInfo;
    bool m_bEdit;
};
#endif // DLGDEVINFO_H
