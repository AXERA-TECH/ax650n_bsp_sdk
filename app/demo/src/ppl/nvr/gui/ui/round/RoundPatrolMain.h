#ifndef ROUNDPATROLMAIN_H
#define ROUNDPATROLMAIN_H

#include <QWidget>
#include <QLabel>
#include <QFuture>

#include <QTcpServer>
#include <QTcpSocket>
#include <QHostInfo>

#include "ax_base_type.h"
#include "global/UiGlobalDef.h"
#include "utils/SplitWidgetMgr.h"

QT_BEGIN_NAMESPACE
namespace Ui { class RoundPatrolMain; }
QT_END_NAMESPACE

class RoundPatrolMain : public QWidget
{
    Q_OBJECT

public:
    RoundPatrolMain(QWidget *parent = nullptr);
    ~RoundPatrolMain();
    QTcpServer* m_tcpServer;

private slots:
    void onSocketReadyRead();
    void onClientConnected();
    void onClientDisconnected();
    void onNewConnection();
    void onSocketStateChange(QAbstractSocket::SocketState socketState);

protected:
    virtual bool eventFilter(QObject *watched, QEvent *event) override;
    virtual void showEvent(QShowEvent *event) override;
    virtual void hideEvent(QHideEvent *event) override;
    virtual void resizeEvent(QResizeEvent *event) override;

private:
    Ui::RoundPatrolMain *ui;
    SplitWidgetMgr *m_pSplitWidgetMgr = nullptr;

    int m_nLeftMargin = 0;
    int m_nTopMargin = 0;
    int m_nRightMargin = 0;
    int m_nBottomMargin = 0;
    int m_nWidth = 0;
    int m_nHeight = 0;
    int m_nXOffset = 0;
    int m_nYOffset = 0;

    QTcpSocket* m_tcpSocket;
};
#endif // ROUNDPATROLMAIN_H