#ifndef CONFIRMDIALOG_H
#define CONFIRMDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>

class ConfirmDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConfirmDialog(const QString &title, QWidget *parent = nullptr);
    ~ConfirmDialog();

    static bool confirm(const QString &title, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QLabel *m_titleLabel;
    QPushButton *m_cancelBtn;
    QPushButton *m_confirmBtn;
};

#endif // CONFIRMDIALOG_H
