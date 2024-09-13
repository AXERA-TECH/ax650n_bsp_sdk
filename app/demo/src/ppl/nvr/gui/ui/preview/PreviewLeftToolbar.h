#ifndef PREVIEWLEFTTOOLBAR_H
#define PREVIEWLEFTTOOLBAR_H

#include <QWidget>
#include <QStandardItemModel>

QT_BEGIN_NAMESPACE
namespace Ui { class PreviewLeftToolbar; }
QT_END_NAMESPACE

class PreviewLeftToolbar : public QWidget
{
    Q_OBJECT

public:
    PreviewLeftToolbar(QWidget *parent = nullptr);
    ~PreviewLeftToolbar();

protected:
    virtual void showEvent(QShowEvent *event) override;


private:
    Ui::PreviewLeftToolbar *ui;
    QStandardItemModel *m_ItemModel;
};
#endif // PREVIEWLEFTTOOLBAR_H
