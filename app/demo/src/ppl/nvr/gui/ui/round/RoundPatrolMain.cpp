#include "RoundPatrolMain.h"
#include "ui_RoundPatrolMain.h"
#include <QDebug>
#include <QShowEvent>
#include <QString>
#include <unistd.h>

#include "AppLogApi.h"

#define UI_TAG "ROUND"

RoundPatrolMain::RoundPatrolMain(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::RoundPatrolMain)
{
    // printf("[%s]***************RoundPatrolMain ++\n", __func__);
    ui->setupUi(this);
    m_pSplitWidgetMgr = new SplitWidgetMgr(AX_NVR_VIEW_TYPE::POLLING, ui->gridLayout, this);

    m_tcpServer = new QTcpServer(this);
    connect(m_tcpServer, SIGNAL(newConnection()), this, SLOT(onNewConnection()));

    this->hide();
    // printf("[%s]***************RoundPatrolMain--\n", __func__);
}

RoundPatrolMain::~RoundPatrolMain()
{
    if (m_tcpServer->isListening()) {
        m_tcpServer->close();
        delete m_tcpServer;
        m_tcpServer = nullptr;
    }

    delete ui;
    delete m_pSplitWidgetMgr;
    m_pSplitWidgetMgr = nullptr;
}

void RoundPatrolMain::onNewConnection()
{
    m_tcpSocket = m_tcpServer->nextPendingConnection();

    connect(m_tcpSocket, SIGNAL(connected()), this, SLOT(onClientConnected()));
    connect(m_tcpSocket, SIGNAL(disconnected()), this, SLOT(onClientDisconnected()));
    connect(m_tcpSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(onSocketStateChange(QAbstractSocket::SocketState)));
    onSocketStateChange(m_tcpSocket->state());
    connect(m_tcpSocket, SIGNAL(readyRead()), this, SLOT(onSocketReadyRead()));
}


void RoundPatrolMain::onSocketReadyRead()
{
    auto compare_read = [this](QByteArray read, const char* cmp) {
        return (read == QByteArray(cmp));
    };
    SPLIT_TYPE enSplitType = SPLIT_TYPE::ONE;
    while (m_tcpSocket->canReadLine()) {
        QByteArray strRead = m_tcpSocket->readLine();
        if (compare_read(strRead, ROUND_SPLIT_CLOSE)) {
            printf("[SUCCESS] CLose\n");
            m_tcpSocket->deleteLater();
            QApplication::quit();
        } else if (compare_read(strRead, ROUND_SPLIT_FINISH)) {
            this->hide();
            printf("[SUCCESS] Finish\n");
        } else if (compare_read(strRead, ROUND_SPLIT_START)) {
            this->show();
            m_pSplitWidgetMgr->ChangeSplitWidgets(enSplitType);
            m_pSplitWidgetMgr->CalcLayouts(enSplitType, m_nWidth, m_nHeight,
                                    m_nLeftMargin, m_nTopMargin, m_nRightMargin, m_nBottomMargin);
            printf("[SUCCESS] Start\n");
        } else {
            SPLIT_TYPE enSplitTypeTmp = m_pSplitWidgetMgr->GetCurrentSplitType();
            if (compare_read(strRead, ROUND_SPLIT_ONE))     enSplitType = SPLIT_TYPE::ONE;
            if (compare_read(strRead, ROUND_SPLIT_FOUR))    enSplitType = SPLIT_TYPE::FOUR;
            if (compare_read(strRead, ROUND_SPLIT_EIGHT))   enSplitType = SPLIT_TYPE::EIGHT;
            if (compare_read(strRead, ROUND_SPLIT_SIXTEEN)) enSplitType = SPLIT_TYPE::SIXTEEN;
            if (compare_read(strRead, ROUND_SPLIT_THIRTYSIX)) enSplitType = SPLIT_TYPE::THIRTYSIX;
            m_pSplitWidgetMgr->ChangeSplitWidgets(enSplitType);
            m_pSplitWidgetMgr->CalcLayouts(enSplitType, m_nWidth, m_nHeight,
                                    m_nLeftMargin, m_nTopMargin, m_nRightMargin, m_nBottomMargin);

            printf("[SUCCESS] Change layout from %d to %d \n", (int)enSplitTypeTmp, (int)enSplitType);
        }
    }
}

void RoundPatrolMain::onClientConnected()
{
    printf("[%s][%d] socket:connect %s %d\n", __func__, __LINE__,
                    m_tcpSocket->peerAddress().toString().toStdString().c_str(),
                    m_tcpSocket->peerPort());

    // printf("[%s]socket:connect %s %d", __func__,
    //                 m_tcpSocket->peerAddress().toString().toStdString().c_str(),
    //                 m_tcpSocket->peerPort());

    // qDebug() << m_tcpSocket->peerAddress().toString() << QString::number(m_tcpSocket->peerPort());

}

void RoundPatrolMain::onClientDisconnected()
{
    // printf("[%s]socket:disconnect", __func__);
    printf("[%s][%d] socket:disconnect\n", __func__, __LINE__);
    m_tcpSocket->deleteLater();
}

void RoundPatrolMain::onSocketStateChange(QAbstractSocket::SocketState socketState)
{
    switch (socketState) {
    case QAbstractSocket::UnconnectedState:
        printf("[%s]socket:UnconnectedState\n", __func__);
        break;
    case QAbstractSocket::HostLookupState:
        printf("[%s]socket:HostLookupState\n", __func__);
        break;
    case QAbstractSocket::ConnectingState:
        printf("[%s]socket:ConnectingState\n", __func__);
        break;
    case QAbstractSocket::ConnectedState:
        printf("[%s]socket:ConnectedState\n", __func__);
        break;
    case QAbstractSocket::BoundState:
        printf("[%s]socket:BoundState\n", __func__);
        break;
    case QAbstractSocket::ClosingState:
        printf("[%s]socket:ClosingState\n", __func__);
        break;
    case QAbstractSocket::ListeningState:
        printf("[%s]socket:ListeningState\n", __func__);
        break;
    default:
        printf("[%s]socket:others...\n", __func__);
        break;
    }
}

void RoundPatrolMain::resizeEvent(QResizeEvent *event) {

    auto __INIT_FUNC__ = [this]() {
        QPoint pt = this->mapToGlobal(QPoint(0, 0));
        m_nLeftMargin   = pt.x()%2;
        m_nTopMargin    = pt.y()%2;
        m_nRightMargin  = 0;
        m_nBottomMargin = 0;
        m_nWidth        = this->geometry().width() - m_nLeftMargin - m_nRightMargin;
        m_nHeight       = this->geometry().height() - m_nTopMargin - m_nBottomMargin;
    };

    // printf("[%s]***************resizeEvent\n", __func__);
    __INIT_FUNC__();
    QApplication::setOverrideCursor(Qt::BlankCursor);
    m_pSplitWidgetMgr->ChangeSplitWidgets(m_pSplitWidgetMgr->GetCurrentSplitType());
    return QWidget::resizeEvent(event);
}

void RoundPatrolMain::showEvent(QShowEvent *event) {
    // printf("[%s]***************showEvent\n", __func__);
    return QWidget::showEvent(event);
}

void RoundPatrolMain::hideEvent(QHideEvent *event) {
    // printf("[%s]***************hideEvent\n", __func__);
    return QWidget::hideEvent(event);
}

bool RoundPatrolMain::eventFilter(QObject *watched, QEvent *pEvent)
{
    if (pEvent->type() == QEvent::KeyPress) {
        return true;
    }
    else if (pEvent->type() == QEvent::MouseButtonPress) {
        return true;
    }
    else {
        return QObject::eventFilter(watched, pEvent);
    }
}