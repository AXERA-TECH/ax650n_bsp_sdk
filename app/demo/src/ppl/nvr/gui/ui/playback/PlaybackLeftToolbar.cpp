#include "PlaybackLeftToolbar.h"
#include "ui_PlaybackLeftToolbar.h"
#include <QScrollBar>

#include "global/UiGlobalDef.h"
#include "AXNVRFramework.h"


#include <QDebug>


PlaybackLeftToolbar::PlaybackLeftToolbar(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PlaybackLeftToolbar)
{

    m_iconChecked = QIcon(":/img/checked.png");
    m_iconUnchecked = QIcon(":/img/unchecked.png");

    auto __INIT_TABLE_FUNC__ = [this](auto table) {
        table->setAlternatingRowColors(true);
        table->setStyleSheet(CSS_TABLE);
        table->horizontalHeader()->setStyleSheet(CSS_TABLE_HEAD);
        table->setSelectionBehavior(QTableWidget::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setRowCount(1);
        table->setColumnCount(2);
        table->setColumnWidth(0, 30);
        table->setColumnWidth(1, 100);

    };

    ui->setupUi(this);
    // this->dumpStructure(ui->calendarWidget, 0);

    this->setStyleSheet(CSS_WIDGET);
    ui->tabWidget->setStyleSheet(CSS_TABWIDGET);
    ui->calendarWidget->setStyleSheet(CSS_CALENDAR);
    ui->tableWidget_1x1->verticalScrollBar()->setStyleSheet(CSS_SCROLLBAR);
    ui->tableWidget_2x2->verticalScrollBar()->setStyleSheet(CSS_SCROLLBAR);
    ui->tableWidget_1_7->verticalScrollBar()->setStyleSheet(CSS_SCROLLBAR);
    ui->tableWidget_4x4->verticalScrollBar()->setStyleSheet(CSS_SCROLLBAR);

    QPixmap pic1(":/img/1x1.png");
    QPixmap pic2(":/img/2x2.png");
    QPixmap pic3(":/img/1+7.png");
    QPixmap pic4(":/img/4x4.png");

    ui->tabWidget->setTabIcon(0, pic1);
    ui->tabWidget->setTabIcon(1, pic2);
    ui->tabWidget->setTabIcon(2, pic3);
    ui->tabWidget->setTabIcon(3, pic4);

    connect(ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(onTabCurrentChanged(int)));

    connect(ui->tableWidget_1x1, &QTableWidget::cellClicked, this, &PlaybackLeftToolbar::onTableCellClicked);
    __INIT_TABLE_FUNC__(ui->tableWidget_1x1);

    connect(ui->tableWidget_2x2, &QTableWidget::cellClicked, this, &PlaybackLeftToolbar::onTableCellClicked);
    __INIT_TABLE_FUNC__(ui->tableWidget_2x2);

    connect(ui->tableWidget_1_7, &QTableWidget::cellClicked, this, &PlaybackLeftToolbar::onTableCellClicked);
    __INIT_TABLE_FUNC__(ui->tableWidget_1_7);

    connect(ui->tableWidget_4x4, &QTableWidget::cellClicked, this, &PlaybackLeftToolbar::onTableCellClicked);
    __INIT_TABLE_FUNC__(ui->tableWidget_4x4);

    connect(ui->calendarWidget, &QCalendarWidget::selectionChanged, this, &PlaybackLeftToolbar::onCalendarChanged);

    ui->tabWidget->setCurrentIndex(0);

    connect(ui->pushButton, &QPushButton::clicked, this, &PlaybackLeftToolbar::onClickedRefresh);
}

PlaybackLeftToolbar::~PlaybackLeftToolbar(void)
{
    delete ui;
}

void PlaybackLeftToolbar::onTabCurrentChanged(int index)
{
    SPLIT_TYPE type = SPLIT_TYPE::ONE;
    switch (index)
    {
    case 0: type = SPLIT_TYPE::ONE;         break;
    case 1: type = SPLIT_TYPE::FOUR;        break;
    case 2: type = SPLIT_TYPE::EIGHT;       break;
    case 3: type = SPLIT_TYPE::SIXTEEN;     break;
    default:
        break;
    }
    emit signal_change_split(type);
}

void PlaybackLeftToolbar::showEvent(QShowEvent *event) {
    this->setTableData();
    return QWidget::showEvent(event);
}

void PlaybackLeftToolbar::onCalendarChanged(void) {
    emit signal_playback_action(PLAYBACK_ACTION_TYPE::UPDATE);
}

void PlaybackLeftToolbar::onClickedRefresh(void) {
    this->setTableData();
}

void PlaybackLeftToolbar::onTableCellClicked(int row, int col) {
    auto __CHECK_FUNC__ = [this](auto table, auto row, auto col, auto *cnt, auto max, auto checked) {
        /* Toggle check status or determined by <checked> value */
        QTableWidgetItem *item = table->item(row, col);
        AX_S32 nChecked = (checked == -1 ? (item->data(Qt::UserRole).toInt() ? 0 : 1) : checked);
        if (nChecked) {
            if (*cnt >= max) {
                return;
            }
            item->setIcon(this->m_iconChecked);
            item->setData(Qt::UserRole, 1);
            *cnt += 1;
        } else {
            item->setIcon(this->m_iconUnchecked);
            item->setData(Qt::UserRole, 0);
            *cnt -= 1;
        }
    };

    if (col == 1) return;

    if (sender() == ui->tableWidget_1x1) {
        __CHECK_FUNC__(ui->tableWidget_1x1, row, col, &m_nCnt1x1, 1, -1);
    } else if (sender() == ui->tableWidget_2x2) {
        __CHECK_FUNC__(ui->tableWidget_2x2, row, col, &m_nCnt2x2, 4, -1);
    } else if (sender() == ui->tableWidget_1_7) {
        __CHECK_FUNC__(ui->tableWidget_1_7, row, col, &m_nCnt1_7, 8, -1);
    } else if (sender() == ui->tableWidget_4x4) {
        __CHECK_FUNC__(ui->tableWidget_4x4, row, col, &m_nCnt4_4, 16, -1);
    } else {
        /* triggered from test suite */
        QTabWidget* pTab = this->findChild<QTabWidget*>(QString("tabWidget"));
        int index = pTab->currentIndex();
        switch (index) {
            case 0: {
                __CHECK_FUNC__(ui->tableWidget_1x1, row, col, &m_nCnt1x1, 1, 1);
                break;
            }
            case 1: {
                __CHECK_FUNC__(ui->tableWidget_2x2, row, col, &m_nCnt2x2, 4, 1);
                break;
            }
            case 2: {
                __CHECK_FUNC__(ui->tableWidget_1_7, row, col, &m_nCnt1_7, 8, 1);
                break;
            }
            case 3: {
                __CHECK_FUNC__(ui->tableWidget_4x4, row, col, &m_nCnt4_4, 16, 1);
                break;
            }
            default: break;
        }
    }

    emit signal_playback_action(PLAYBACK_ACTION_TYPE::UPDATE);
}

void PlaybackLeftToolbar::resizeEvent(QResizeEvent *event) {

    return QWidget::resizeEvent(event);
}

ax_nvr_channel_vector PlaybackLeftToolbar::GetChannelsSelected(void) {

    auto __GETDEV_FUNC__ = [this](auto table, auto &list) {
        QTableWidgetItem *check = nullptr;
        QTableWidgetItem *channel = nullptr;
        int index = 0;
        for (int i = 0; i < table->rowCount(); i ++) {
            check = table->item(i, 0);
            channel = table->item(i, 1);
            if (check->data(Qt::UserRole).toInt() == 1) {
                list.emplace_back(make_tuple(index, channel->data(Qt::UserRole).toInt()));
                index ++;
            }
        }
    };

    ax_nvr_channel_vector vecDevices;
    // 1x1
    if (ui->tabWidget->currentIndex() == 0) {
        __GETDEV_FUNC__(ui->tableWidget_1x1, vecDevices);
    }
    // 2x2
    else if (ui->tabWidget->currentIndex() == 1) {
        __GETDEV_FUNC__(ui->tableWidget_2x2, vecDevices);
    }
    // 1+7
    else if (ui->tabWidget->currentIndex() == 2) {
        __GETDEV_FUNC__(ui->tableWidget_1_7, vecDevices);
    }
    // 4x4
    else if (ui->tabWidget->currentIndex() == 3) {
        __GETDEV_FUNC__(ui->tableWidget_4x4, vecDevices);
    }

    return vecDevices;
}

std::string PlaybackLeftToolbar::GetCurrentYMD(void) {
    return ui->calendarWidget->selectedDate().toString("yyyy-MM-dd").toStdString();
}

AX_U32 PlaybackLeftToolbar::GetCurrentIntYMD(void) {
    return ui->calendarWidget->selectedDate().toString("yyyyMMdd").toInt();
}

void PlaybackLeftToolbar::SetCurrentYMD(bool bNext) {
    QDate date = ui->calendarWidget->selectedDate();
    ui->calendarWidget->setSelectedDate(date.addDays(bNext?1:-1));
}

void PlaybackLeftToolbar::dumpStructure(const QObject *obj, int spaceCount) {
    qDebug() << QString("%1%2 : %3")
                .arg("", spaceCount)
                .arg(obj->metaObject()->className())
                .arg(obj->objectName());

    QObjectList list = obj->children();

    foreach (QObject * child, list) {
        dumpStructure(child, spaceCount + 4);
    }
}

void PlaybackLeftToolbar::setTableData(void) {

    auto __INIT_TABLE_FUNC__ = [this](auto table) {
        table->setRowCount(0);
        // FixMe: thread?
        CAXNVRPlaybakCtrl *pPlayback = CAXNVRFramework::GetInstance()->PlaybakCtrl();
        if (pPlayback == NULL) {
            return;
        }
        std::vector<AX_NVR_DEV_ID> vecDevInfo = pPlayback->GetDeviceFiles();
        m_vecDevInfo.swap(vecDevInfo);
        for (auto device : m_vecDevInfo) {
            int nRowIndex = table->rowCount();
            table->insertRow(nRowIndex);

            table->setItem(nRowIndex, 0, new QTableWidgetItem(this->m_iconUnchecked, ""));
            QTableWidgetItem *item = table->item(nRowIndex, 0);
            item->setFlags(item->flags() & (~Qt::ItemIsEditable));
            item->setData(Qt::UserRole, 0);

            table->setItem(nRowIndex, 1, new QTableWidgetItem(QString("device channel %1").arg(device + 1)));
            item = table->item(nRowIndex, 1);
            item->setFlags(item->flags() & (~Qt::ItemIsEditable));
            item->setData(Qt::UserRole, device);
        }
    };

    // FixMe: Get the devid of the video recording through the VIDEO-RETRIEVAL-COMPONENT
    m_nCnt1x1 = 0;
    m_nCnt2x2 = 0;
    m_nCnt1_7 = 0;
    m_nCnt4_4 = 0;

    __INIT_TABLE_FUNC__(ui->tableWidget_1x1);
    __INIT_TABLE_FUNC__(ui->tableWidget_2x2);
    __INIT_TABLE_FUNC__(ui->tableWidget_1_7);
    __INIT_TABLE_FUNC__(ui->tableWidget_4x4);
}