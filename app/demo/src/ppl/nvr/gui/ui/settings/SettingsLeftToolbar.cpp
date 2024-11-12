#include "SettingsLeftToolbar.h"
#include "ui_SettingsLeftToolbar.h"
#include "global/UiGlobalDef.h"

SettingsLeftToolbar::SettingsLeftToolbar(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SettingsLeftToolbar)
{
    ui->setupUi(this);
    ui->pushButtonSystem->setStyleSheet(CSS_PUSHBUTTON);
    ui->pushButtonDevice->setStyleSheet(CSS_PUSHBUTTON);
    ui->pushButtonRecord->setStyleSheet(CSS_PUSHBUTTON);

    connect(ui->pushButtonSystem, &QPushButton::clicked, [=](){ emit signal_change_page((int)CONFIG_TYPE::SYSTEM); });
    connect(ui->pushButtonDevice, &QPushButton::clicked, [=](){ emit signal_change_page((int)CONFIG_TYPE::DEVICE); });
    connect(ui->pushButtonRecord, &QPushButton::clicked, [=](){ emit signal_change_page((int)CONFIG_TYPE::RECORD); });
}

SettingsLeftToolbar::~SettingsLeftToolbar()
{
    delete ui;
    // delete m_ItemModel;
}

// void SettingsLeftToolbar::InitPageList()
// {
//     m_ItemModel = new QStandardItemModel(this);
//     QStringList strList;
//     strList.append("系统配置");
//     strList.append("网络配置");
//     strList.append("通道管理");
//     strList.append("录像管理");

//     int nCount = strList.size();
//     for(int i = 0; i < nCount; i++)
//     {
//         QString string = static_cast<QString>(strList.at(i));
//         QStandardItem *item = new QStandardItem(string);
//         m_ItemModel->appendRow(item);
//     }

//     ui->listView->setModel(m_ItemModel);
//     connect(ui->listView, SIGNAL(clicked(QModelIndex)), this, SLOT(OnPageIndexChange(QModelIndex)));
// }

// void SettingsLeftToolbar::OnPageIndexChange(int index)
// {
//     emit signal_change_page(index);
// }
