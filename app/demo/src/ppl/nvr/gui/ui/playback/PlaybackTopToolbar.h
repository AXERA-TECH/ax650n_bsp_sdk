#ifndef PLAYBACKTOPTOOLBAR_H
#define PLAYBACKTOPTOOLBAR_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class PlaybackTopToolbar; }
QT_END_NAMESPACE

class PlaybackTopToolbar : public QWidget
{
    Q_OBJECT

public:
    PlaybackTopToolbar(QWidget *parent = nullptr);
    ~PlaybackTopToolbar();

private slots:
    void OnChangeMainSub1();

private:
    Ui::PlaybackTopToolbar *ui;

signals:
    void signal_change_mainsub1(void);
};
#endif // PLAYBACKTOPTOOLBAR_H
