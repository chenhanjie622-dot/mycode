#ifndef USERSETTINGSDIALOG_H
#define USERSETTINGSDIALOG_H

#include <QDialog>
#include <QButtonGroup>

namespace Ui {
class SettingsDialog;
}

class UserSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UserSettingsDialog(QWidget *parent = nullptr);
    ~UserSettingsDialog();

private:
    void initButtonGroup(); // 初始化左侧按钮的互斥逻辑

    Ui::SettingsDialog *ui;
    QButtonGroup *m_navGroup; // 用于管理导航按钮的选中状态


signals:
    void signalSwitchAccount();


};



#endif // USERSETTINGSDIALOG_H
