#include "nvrmainwindow.h"
#include "ui_nvrmainwindow.h"
#include <QPainter>
#include "AppLogApi.h"
#include "AXNVRFramework.h"
#include "ElapsedTimer.hpp"
#include "./utils/NoSignals.h"

#define TAG "MAIN"

#define MAX_TCP_CONNECT_TRY_COUNT (3)

NVRMainWindow::NVRMainWindow(unsigned int type, unsigned int w, unsigned int h, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::NVRMainWindow)
    , m_pPreviewLeftToolbar(new PreviewLeftToolbar)
    , m_pPreviewTopToolbar(new PreviewTopToolbar)
    , m_pPreviewMain(new PreviewMain)
    , m_pPlaybackLeftToolbar(new PlaybackLeftToolbar)
    , m_pPlaybackTopToolbar(new PlaybackTopToolbar)
    , m_pPlaybackMain(new PlaybackMain)
    , m_pSettingsLeftToolbar(new SettingsLeftToolbar)
    , m_pSettingsTopToolbar(new SettingsTopToolbar)
    , m_pSettingsMain(new SettingsMain)
{
    const int START_PAGE = 0;

    ui->setupUi(this);
    this->setGeometry(0, 0, w, h);
    if (type == 1) {
        m_pRoundPatrolMain = new(std::nothrow) RoundPatrolMain;

        ui->gridLayout->setRowStretch(0, 0);
        ui->gridLayout->setColumnStretch(0, 0);
        ui->widgetTop->hide();
        ui->widgetLogo->hide();
        ui->stackedWidgetLeftToolbar->hide();
        ui->verticalLayout_preview->addWidget(m_pRoundPatrolMain);
        ui->stackedWidget->setCurrentIndex(0);

        QHostAddress addr(QString(ROUND_SPLIT_IP));
        m_pRoundPatrolMain->m_tcpServer->listen(addr, ROUND_SPLIT_PORT);
        if (!m_pRoundPatrolMain->m_tcpServer->isListening()) {
            printf("[%s]Failed to start server: %s\n", __func__, m_pRoundPatrolMain->m_tcpServer->errorString().toStdString().c_str());
        }
    } else {

        ui->label_version->setText(NVR_VERSION_STR);

        ui->widgetTop->setStyleSheet(CSS_WIDGET);
        ui->widgetLogo->setStyleSheet(CSS_WIDGET);

        ui->stackedWidgetLeftToolbar->setStyleSheet(CSS_WIDGET_1);

        ui->pushButtonPreview->setStyleSheet(CSS_PUSHBUTTON);
        ui->pushButtonPlayback->setStyleSheet(CSS_PUSHBUTTON);
        ui->pushButtonPolling->setStyleSheet(CSS_PUSHBUTTON);

        ui->pushButtonPreview->setCheckable(true);
        ui->pushButtonPlayback->setCheckable(true);

        NoSignals::setChecked(ui->pushButtonPreview, true);

        // initialize ui
        ui->verticalLayout_lt_preview->addWidget(m_pPreviewLeftToolbar);
        ui->horizontalLayout_tt_preview->addWidget(m_pPreviewTopToolbar);
        ui->verticalLayout_preview->addWidget(m_pPreviewMain);
        m_pPreviewMain->m_pPreviewTopToolbar = m_pPreviewTopToolbar;

        ui->verticalLayout_lt_playback->addWidget(m_pPlaybackLeftToolbar);
        ui->horizontalLayout_tt_playback->addWidget(m_pPlaybackTopToolbar);
        ui->verticalLayout_playback->addWidget(m_pPlaybackMain);

        ui->verticalLayout_lt_settings->addWidget(m_pSettingsLeftToolbar);
        ui->horizontalLayout_tt_settings->addWidget(m_pSettingsTopToolbar);
        ui->verticalLayout_settings->addWidget(m_pSettingsMain);

        ui->stackedWidgetLeftToolbar->setCurrentIndex(START_PAGE);
        ui->stackedWidgetTopToolbar->setCurrentIndex(START_PAGE);
        ui->stackedWidget->setCurrentIndex(START_PAGE);

        AX_NVR_DISPVO_CONFIG_T tDipSecCfg = CNVRConfigParser::GetInstance()->GetSecondaryDispConfig();
        if (tDipSecCfg.nDevId != -1) {
            m_pTimerPolling = new QTimer(this);
            connect(m_pTimerPolling, SIGNAL(timeout()), this, SLOT(OnTimeOutPolling()));

            m_pTcpSocket = new QTcpSocket(this);

            connect(&m_watcherRPatrol, &QFutureWatcher<AX_NVR_ACTION_RES_T>::started, [=]{
                if (ui->pushButtonPolling) {
                    ui->pushButtonPolling->setEnabled(AX_FALSE);
                }
            });

            connect(&m_watcherRPatrol, &QFutureWatcher<AX_NVR_ACTION_RES_T>::finished, [=]{

                AX_NVR_ACTION_RES_T res = m_watcherRPatrol.resultAt(0);
                AX_U32 nCurrentSplitCnt = CAXNVRFramework::GetInstance()->GetCurrentSplitCnt();

                if (res.enResult != AX_NVR_PREVIEW_RES_TYPE::RPATROL_ERR) {
                    if (!m_pTcpSocket->isValid()) {
                        AX_U32 nTryCount = MAX_TCP_CONNECT_TRY_COUNT;
                        while (nTryCount-- > 0) {
                            m_pTcpSocket->connectToHost(ROUND_SPLIT_IP, ROUND_SPLIT_PORT);
                            if (m_pTcpSocket->waitForConnected()) {
                                LOG_M_W(TAG, "[%s][%d] connectToHost.", __func__, __LINE__);
                                break;
                            } else {
                                LOG_M_W(TAG, "[%s][%d] connectToHost failed", __func__, __LINE__);
                                CElapsedTimer::GetInstance()->mSleep(1000);
                            }
                        }
                    }
                }

                if (res.enResult == AX_NVR_PREVIEW_RES_TYPE::RPATROL_UPDATE) {

                    // LOG_M_W(TAG, "[%s][%d] **************** RPATROL_UPDATE.", __func__, __LINE__);
                    // if (!m_pTcpSocket->isValid()) {
                    //     m_pTcpSocket->connectToHost(ROUND_SPLIT_IP, ROUND_SPLIT_PORT);
                    //     if (m_pTcpSocket->waitForConnected(1000*300)) {
                    //         LOG_M_W(TAG, "[%s][%d] connectToHost.", __func__, __LINE__);
                    //     } else {
                    //         LOG_M_W(TAG, "[%s][%d] connectToHost failed", __func__, __LINE__);
                    //     }
                    // }

                    if (m_pTcpSocket->isValid()) {
                        switch (nCurrentSplitCnt)
                        {
                        case 1:
                            m_pTcpSocket->write(ROUND_SPLIT_ONE);
                            break;
                        case 4:
                            m_pTcpSocket->write(ROUND_SPLIT_FOUR);
                            break;
                        case 8:
                            m_pTcpSocket->write(ROUND_SPLIT_EIGHT);
                            break;
                        case 16:
                            m_pTcpSocket->write(ROUND_SPLIT_SIXTEEN);
                            break;
                        case 36:
                            m_pTcpSocket->write(ROUND_SPLIT_THIRTYSIX);
                            break;
                        default:
                            break;
                        }
                        m_pTcpSocket->waitForBytesWritten(300);
                    }
                    else {
                        LOG_M_E(TAG, "[%s][%d] TcpSocket connect failed.", __func__, __LINE__);
                    }
                }
                else if (res.enResult == AX_NVR_PREVIEW_RES_TYPE::RPATROL_STOP) {
                    if (m_pTcpSocket->isValid()) {
                        // LOG_M_W(TAG, "[%s][%d] **************** RPATROL_STOP.", __func__, __LINE__);
                        m_pTcpSocket->write(ROUND_SPLIT_FINISH);
                        m_pTcpSocket->waitForBytesWritten();
                        // m_pTcpSocket->disconnectFromHost();
                        // m_pTcpSocket->close();
                    } else {
                        LOG_M_E(TAG, "[%s][%d] TcpSocket connect failed.", __func__, __LINE__);
                    }
                }
                else if (res.enResult == AX_NVR_PREVIEW_RES_TYPE::RPATROL_START) {
                    if (m_pTcpSocket->isValid()) {
                        // LOG_M_W(TAG, "[%s][%d] **************** RPATROL_START.", __func__, __LINE__);
                        m_pTcpSocket->write(ROUND_SPLIT_START);
                        m_pTcpSocket->waitForBytesWritten();
                        // m_pTcpSocket->disconnectFromHost();
                        // m_pTcpSocket->close();
                    } else {
                        LOG_M_E(TAG, "[%s][%d] TcpSocket connect failed.", __func__, __LINE__);
                    }
                }

                if (ui->pushButtonPolling) {
                    ui->pushButtonPolling->setEnabled(AX_TRUE);
                }
            });

            connect(ui->pushButtonPolling, &QPushButton::clicked, this, &NVRMainWindow::OnEnablePolling);
        }
        else {
            ui->pushButtonPolling->setEnabled(false);
        }

        // action
        connect(ui->pushButtonPreview, &QPushButton::clicked, [&]() {
            bool isChecked = ui->pushButtonPreview->isChecked();
            if (isChecked) {
                NoSignals::setChecked(ui->pushButtonPlayback, false);
                ui->stackedWidgetLeftToolbar->setCurrentIndex(0);
                ui->stackedWidgetTopToolbar->setCurrentIndex(0);
                ui->stackedWidget->setCurrentIndex(0);
            }
            else {
                NoSignals::setChecked(ui->pushButtonPreview, true);
            }
        });

        connect(ui->pushButtonPlayback, &QPushButton::clicked, [&]() {
            bool isChecked = ui->pushButtonPlayback->isChecked();
            if (isChecked) {
                NoSignals::setChecked(ui->pushButtonPreview, false);
                ui->stackedWidgetLeftToolbar->setCurrentIndex(1);
                ui->stackedWidgetTopToolbar->setCurrentIndex(1);
                ui->stackedWidget->setCurrentIndex(1);
            }
            else {
                NoSignals::setChecked(ui->pushButtonPlayback, true);
            }
        });

        connect(ui->pushButtonSettings, &QPushButton::clicked, [&]() {
            NoSignals::setChecked(ui->pushButtonPreview, false);
            NoSignals::setChecked(ui->pushButtonPlayback, false);
            ui->stackedWidgetLeftToolbar->setCurrentIndex(2);
            ui->stackedWidgetTopToolbar->setCurrentIndex(2);
            ui->stackedWidget->setCurrentIndex(2);
        });

        connect(ui->pushButtonPower, &QPushButton::clicked, [&]() {
            this->close();
        });

        // preview
        connect(m_pPreviewTopToolbar, &PreviewTopToolbar::signal_change_split, m_pPreviewMain, &PreviewMain::OnChangeSplitVideo);
        connect(m_pPreviewTopToolbar, &PreviewTopToolbar::signal_change_prev_next, m_pPreviewMain, &PreviewMain::OnChangePrevNext);
        connect(m_pPreviewTopToolbar, &PreviewTopToolbar::signal_change_mainsub1, m_pPreviewMain, &PreviewMain::OnChangeMainSub1);
        connect(m_pPreviewTopToolbar, &PreviewTopToolbar::signal_enable_pip, m_pPreviewMain, &PreviewMain::OnEnablePip);

        // playback
        m_pPlaybackMain->SetCtrls(m_pPlaybackLeftToolbar);
        connect(m_pPlaybackLeftToolbar, &PlaybackLeftToolbar::signal_change_split, m_pPlaybackMain, &PlaybackMain::OnChangeSplitVideo);
        connect(m_pPlaybackLeftToolbar, &PlaybackLeftToolbar::signal_playback_action, m_pPlaybackMain, &PlaybackMain::OnDoPlaybackActionUpdate);
        connect(m_pPlaybackTopToolbar, &PlaybackTopToolbar::signal_change_mainsub1, m_pPlaybackMain, &PlaybackMain::OnChangeMainSub1);
        connect(m_pPlaybackMain, &PlaybackMain::signal_stop_status_changed, m_pPreviewMain, &PreviewMain::OnPlaybackStopStatusChanged);

        // settings
        connect(m_pSettingsLeftToolbar, &SettingsLeftToolbar::signal_change_page, m_pSettingsMain, &SettingsMain::OnChangePage);

        AX_NVR_TEST_SUITE_CONFIG_T tTsCfg = CNVRConfigParser::GetInstance()->GetTestSuiteConfig();
        if (AX_NVR_TS_RUN_MODE::DISABLE != tTsCfg.eMode) {
            m_tTestSuiteConfig = CTestSuiteConfigParser::GetInstance()->GetConfig();

            connect(this, &NVRMainWindow::signal_exit, this, [&]() {
                this->close();
            });

            RegisterTSWidgets();
            StartTestSuiteThread(&m_tTestSuiteConfig);
        }
    }
}

NVRMainWindow::~NVRMainWindow() {
    delete ui;

    // preivew
    DEL_PTR(m_pPreviewLeftToolbar);
    DEL_PTR(m_pPreviewTopToolbar);
    DEL_PTR(m_pPreviewMain);

    // playback
    DEL_PTR(m_pPlaybackLeftToolbar);
    DEL_PTR(m_pPlaybackTopToolbar);
    DEL_PTR(m_pPlaybackMain);

    // settings
    DEL_PTR(m_pSettingsLeftToolbar);
    DEL_PTR(m_pSettingsTopToolbar);
    DEL_PTR(m_pSettingsMain);

    // round
    DEL_PTR(m_pRoundPatrolMain);

    DEL_PTR(m_pTimerPolling)
    DEL_PTR(m_pTcpSocket)

    // Test suite modules
    DEL_PTR(m_pTsMain)
    DEL_PTR(m_pTsPreview)
    DEL_PTR(m_pTsPlayback)
}

void NVRMainWindow::__close(void) {
    AX_NVR_TEST_SUITE_CONFIG_T tTsCfg = CNVRConfigParser::GetInstance()->GetTestSuiteConfig();
    if (AX_NVR_TS_RUN_MODE::DISABLE != tTsCfg.eMode) {
        StopTestSuiteThread(&m_tTestSuiteConfig);
    }

    if (m_pTimerPolling) {
        if (!m_pTimerPolling->isActive()) {
            m_pTimerPolling->stop();
        }
    }

    if (m_bRoundPatrolMain && m_pTcpSocket) {
        if (!m_pTcpSocket->isValid()) {
            AX_U32 nTryCount = MAX_TCP_CONNECT_TRY_COUNT;
            while (nTryCount-- > 0) {
                m_pTcpSocket->connectToHost(ROUND_SPLIT_IP, ROUND_SPLIT_PORT);
                if (m_pTcpSocket->waitForConnected()) {
                    LOG_M_W(TAG, "[%s][%d] connectToHost.", __func__, __LINE__);
                    break;
                } else {
                    LOG_M_W(TAG, "[%s][%d] connectToHost failed", __func__, __LINE__);
                    CElapsedTimer::GetInstance()->mSleep(1000);
                }
            }
        }

        if (m_pTcpSocket->isValid()) {
            m_pTcpSocket->write(ROUND_SPLIT_CLOSE);
            m_pTcpSocket->waitForBytesWritten();
            m_pTcpSocket->disconnectFromHost();
        }

        m_pTcpSocket->close();
        m_bRoundPatrolMain = AX_FALSE;
    }

    if (m_threadTS.IsRunning()) {
        m_threadTS.Stop();
        m_threadTS.Join();
    }
}

void NVRMainWindow::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setCompositionMode(QPainter::CompositionMode_Clear);
    painter.fillRect(ui->stackedWidget->geometry(), Qt::SolidPattern);
}

void NVRMainWindow::closeEvent(QCloseEvent *event) {
    __close();
    return QWidget::closeEvent(event);
}

void NVRMainWindow::OnEnablePolling(bool bPolling) {
    do {
        if (!m_watcherRPatrol.isFinished()) {
            LOG_M_E(TAG, "[%s][%d] Still Active.", __func__, __LINE__);
            break;
        }

        if (!bPolling) {
            if (m_pTimerPolling->isActive()) {
                m_pTimerPolling->stop();
            }
        } else {
            const AX_NVR_RPATROL_CONFIG_T& info = CAXNVRFramework::GetInstance()->GetRoundPatrolInfo();
            m_pTimerPolling->start(info.uIntelval*1000);  // sec

// #if 1
// #if 1
//             QProcess process;
//             process.setProgram("/bin/sh");
//             process.setArguments(QStringList() << "./secondary.sh" << "-s" << "1" << "&");
//             process.startDetached();
//             process.close();
// #else
//             system("/bin/sh /opt/bin/NVRDemo/secondary.sh -s 1 &");
// #endif
// #else
//             QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
//             env.insert("LD_LIBRARY_PATH", "$LD_LIBRARY_PATH:/opt/lib/qt-5.15.7/lib");
//             env.insert("QT_QPA_PLATFORM_PLUGIN_PATH", "/opt/lib/qt-5.15.7/plugins");
//             env.insert("QT_QPA_FONTDIR", "/opt/lib/qt-5.15.7/fonts");
//             m_process.setProcessEnvironment(env);
//             m_process.start("/opt/bin/NVRDemo/nvrdemo", QStringList() << "-s" << "1" << "&");
// #endif
//             const AX_NVR_RPATROL_CONFIG_T& info = CAXNVRFramework::GetInstance()->GetRoundPatrolInfo();
//             m_pTimerPolling->start(info.uIntelval*1000);  // sec
        }

        QFutureInterface<AX_NVR_ACTION_RES_T> interface;
        interface.reportStarted();
        m_watcherRPatrol.setFuture(interface.future());

        AX_NVR_ACTION_RES_T res;
        std::thread thread([this, interface, bPolling, res]() mutable {

            if (bPolling) {
                if (!m_bRoundPatrolMain) {
                    m_bRoundPatrolMain = AX_TRUE;
                    if (!CAXNVRFramework::GetInstance()->InitSecondaryDispCtrl()) {
                        LOG_M_E(TAG, "[%s][%d] Init secondary screen failed.", __func__, __LINE__);
                    }
                    QProcess process;
                    process.setProgram("/bin/sh");
                    process.setArguments(QStringList() << "./secondary.sh" << "-s" << "1" << "&");
                    process.startDetached();
                    process.close();
                    sleep(1);
                }

                CAXNVRFramework::GetInstance()->StartRoundPatrol();
                res.enResult = AX_NVR_PREVIEW_RES_TYPE::RPATROL_START;
            }
            else {
                CAXNVRFramework::GetInstance()->StopRoundPatrol();
                res.enResult = AX_NVR_PREVIEW_RES_TYPE::RPATROL_STOP;
            }

            interface.reportResult(res, 0);
            interface.reportFinished();
        });

        // detach thred, Use Future to ensure exit
        thread.detach();

    } while(0);
}

void NVRMainWindow::OnTimeOutPolling() {

    LOG_M_I(TAG, "[%s][%d] +++", __func__, __LINE__);

    do {
        if (!m_watcherRPatrol.isFinished()) {
            LOG_M_I(TAG, "[%s][%d] Still Active.", __func__, __LINE__);
            break;
        }

        QFutureInterface<AX_NVR_ACTION_RES_T> interface;
        interface.reportStarted();
        m_watcherRPatrol.setFuture(interface.future());

        AX_NVR_ACTION_RES_T res;
        std::thread thread([this, interface, res]() mutable {

            CAXNVRFramework::GetInstance()->UpdateRoundPatrol();

            res.enResult = AX_NVR_PREVIEW_RES_TYPE::RPATROL_UPDATE;
            interface.reportResult(res, 0);
            interface.reportFinished();

        });

        // detach thred, Use Future to ensure exit
        thread.detach();

    } while(0);

    LOG_M_I(TAG, "[%s][%d] ---",  __func__, __LINE__);
}

void NVRMainWindow::StartTestSuiteThread(AX_NVR_TS_CONFIG_PTR pTsCfg) {
    if (!m_threadTS.Start([this](AX_VOID* pArg) -> AX_VOID { TestSuiteThread(pArg); }, pTsCfg, "TestSuite")) {
        LOG_MM_E(TAG, "Create test suite thread fail.");
    }
}

void NVRMainWindow::StopTestSuiteThread(AX_NVR_TS_CONFIG_PTR pTsCfg) {
    if (m_threadTS.IsRunning()) {
        if (pTsCfg) {
            for (auto &m : pTsCfg->vecModuleInfo) {
                if (m.bEnable) {
                    CTestSuiteBase* pTsInstance = FindTsInstance(m.strName);
                    if (nullptr == pTsInstance) {
                        LOG_M_W(TAG, "Configured test suite module<%s> is not implemented, ignore this case.", m.strName.c_str());
                        continue;
                    }

                    pTsInstance->Stop();
                }
            }
        }

        m_threadTS.Stop();
        m_threadTS.Join();
    }
}

void NVRMainWindow::OnOperateWidget(const TS_OPERATION_INFO_T& tOprInfo) {
    if (tOprInfo.target) {
        switch (tOprInfo.type) {
            case TS_OPERATION_TYPE::TS_OPR_BTN_CLICK: {
                 if (((QPushButton*)tOprInfo.target)->isEnabled()) {
                    ((QPushButton*)tOprInfo.target)->setChecked(tOprInfo.args.bBtnChecked);
                    ((QPushButton*)tOprInfo.target)->clicked();
                 } else {
                    LOG_M_W(TAG, "Button %s is disabled, operation ignored.", tOprInfo.target->objectName().toStdString().c_str());
                 }
                break;
            }
            case TS_OPERATION_TYPE::TS_OPR_TOGGLE_BTN_CLICK: {
                if (((QPushButton*)tOprInfo.target)->isEnabled()) {
                    bool bCurrState = ((QPushButton*)tOprInfo.target)->isChecked();
                    ((QPushButton*)tOprInfo.target)->setChecked(!bCurrState);
                    ((QPushButton*)tOprInfo.target)->clicked(!bCurrState);
                } else {
                    LOG_M_W(TAG, "Toggle button %s is disabled, operation ignored.", tOprInfo.target->objectName().toStdString().c_str());
                }
                break;
            }
            case TS_OPERATION_TYPE::TS_OPR_TAB_SELECT: {
                ((QTabWidget*)tOprInfo.target)->setCurrentIndex(tOprInfo.args.nTabSelIndex);
                break;
            }
            case TS_OPERATION_TYPE::TS_OPR_CALENDAR_SELECT: {
                ((QCalendarWidget*)tOprInfo.target)->setSelectedDate(tOprInfo.args.calendarSelDate);
                break;
            }
            default: break;
        }
    }
}

AX_VOID NVRMainWindow::TestSuiteThread(AX_VOID* pArg) {
    if (nullptr == pArg) {
        return;
    }

    AX_NVR_TS_CONFIG_PTR pConfig = (AX_NVR_TS_CONFIG_PTR)pArg;
    if (pConfig->bExportResultToFile) {
        CDiskHelper::CreateDir(pConfig->strResultDir.c_str(), AX_FALSE);
    }

    AX_NVR_TEST_SUITE_CONFIG_T tTsCfg = CNVRConfigParser::GetInstance()->GetTestSuiteConfig();

    do {
        static AX_S32 nRound = 0;
        LOG_MM_C(TAG, "Start %d round of test suite.", ++nRound);
        for (auto &m : pConfig->vecModuleInfo) {
            if (m.bEnable) {
                if (!SetTsModuleConfig(m.strName, pConfig)) {
                    LOG_M_E(TAG, "Can not find configured case for module: %s", m.strName.c_str());
                    continue;
                }

                CTestSuiteBase* pTsInstance = FindTsInstance(m.strName);
                if (nullptr == pTsInstance) {
                    LOG_M_W(TAG, "Configured test suite module<%s> is not implemented, ignore this case.", m.strName.c_str());
                    continue;
                }

                AX_CHAR szResultPath[256] = {0};
                if (pConfig->bExportResultToFile) {
                    sprintf(szResultPath, "%s/result_%s.txt", pConfig->strResultDir.c_str(), m.strName.c_str());
                    pTsInstance->Start();
                } else {
                    if (m.nRoundCount == 0 || nRound <= m.nRoundCount) {
                        if (m.bThread) {
                            std::thread thread([pTsInstance]() mutable {
                                pTsInstance->Start();
                            });
                            thread.detach();
                        } else {
                            pTsInstance->Start();
                        }
                    }
                }
            }

            if (!m_threadTS.IsRunning()) {
                break;
            }
        }
    } while (m_threadTS.IsRunning() && pConfig->bRepeat && AX_NVR_TS_RUN_MODE::STABILITY == tTsCfg.eMode && pConfig->vecModuleInfo.size() > 0);

    if (pConfig->bCloseOnFinish) {
        emit signal_exit();
    }
}

AX_BOOL NVRMainWindow::SetTsModuleConfig(const string& strModuleName, const AX_NVR_TS_CONFIG_PTR pModuleConfig) {
    if (nullptr == pModuleConfig) {
        return AX_FALSE;
    }

    for (auto& m: pModuleConfig->vecModuleInfo) {
        if (m.strName == strModuleName) {
            if (strModuleName == "main") {
                m_pTsMain->SetModuleConfig(m);
            } else if (strModuleName == "preview") {
                m_pTsPreview->SetModuleConfig(m);
            } else if (strModuleName == "playback") {
                m_pTsPlayback->SetModuleConfig(m);;
            }

            return AX_TRUE;
        }
    }

    return AX_FALSE;
}

CTestSuiteBase* NVRMainWindow::FindTsInstance(const string& strModuleName) {
    if (strModuleName == "main") {
        return m_pTsMain;
    } else if (strModuleName == "preview") {
        return m_pTsPreview;
    } else if (strModuleName == "playback") {
        return m_pTsPlayback;
    }

    return nullptr;
}

void NVRMainWindow::RegisterTSWidgets() {
    qRegisterMetaType<AX_NVR_ACTION_RES_T>("AX_NVR_ACTION_RES_T");
    qRegisterMetaType<TS_OPERATION_INFO_T>("TS_OPERATION_INFO_T");

    m_pTsMain = new CTestMain();
    if (nullptr != m_pTsMain) {
        m_pTsMain->RegisterWidget("BtnEntry", ui->pushButtonPreview);

        m_pTsMain->RegisterWidget("BtnPreview", ui->pushButtonPreview);
        m_pTsMain->RegisterWidget("BtnPlayback", ui->pushButtonPlayback);
        m_pTsMain->RegisterWidget("BtnSetting", ui->pushButtonSettings);
        m_pTsMain->RegisterWidget("BtnPolling", ui->pushButtonPolling);
        m_pTsMain->RegisterWidget("LabelVersion", ui->label_version);
        m_pTsMain->RegisterWidget("Label", ui->label);

        connect(this, &NVRMainWindow::signal_result_feedback, m_pTsMain, &CTestMain::OnRecvResult);
        connect(m_pTsMain, &CTestMain::signal_ts_widget_opr, this, &NVRMainWindow::OnOperateWidget);
    }

    m_pTsPreview = new CTestPreview();
    if (nullptr != m_pTsPreview) {
        m_pTsPreview->RegisterWidget("BtnEntry", ui->pushButtonPreview);

        m_pTsPreview->RegisterWidget("PreviewLeftToolbar", m_pPreviewLeftToolbar);
        m_pTsPreview->RegisterWidget("PreviewTopToolbar", m_pPreviewTopToolbar);
        m_pTsPreview->RegisterWidget("PreviewMain", m_pPreviewMain);

        connect(m_pPreviewMain, &PreviewMain::signal_result_feedback, m_pTsPreview, &CTestPreview::OnRecvResult);
        connect(m_pTsPreview, &CTestPreview::signal_ts_widget_opr, this, &NVRMainWindow::OnOperateWidget);
    }

    m_pTsPlayback = new CTestPlayback();
    if (nullptr != m_pTsPlayback) {
        m_pTsPlayback->RegisterWidget("BtnEntry", ui->pushButtonPlayback);

        m_pTsPlayback->RegisterWidget("PlayLeftToolbar", m_pPlaybackLeftToolbar);
        m_pTsPlayback->RegisterWidget("PlayTopToolbar", m_pPlaybackTopToolbar);
        m_pTsPlayback->RegisterWidget("PlayMain", m_pPlaybackMain);
        m_pTsPlayback->RegisterWidget("PlayCtrl", m_pPlaybackMain->findChild<QWidget*>("widget_playbackctrl"));

        connect(m_pPlaybackMain, &PlaybackMain::signal_result_feedback, m_pTsPlayback, &CTestPlayback::OnRecvResult);
        connect(m_pTsPlayback, &CTestPlayback::tableCellClicked, m_pPlaybackLeftToolbar, &PlaybackLeftToolbar::onTableCellClicked);
        connect(m_pTsPlayback, &CTestPlayback::timeLocate, m_pPlaybackMain, &PlaybackMain::OnLocateTime);
        connect(m_pTsPlayback, &CTestPlayback::signal_ts_widget_opr, this, &NVRMainWindow::OnOperateWidget);
    }
}