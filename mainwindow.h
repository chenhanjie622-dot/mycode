#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "usermanagerdialog.h"
#include "logindialog.h"
#include "homedialog.h"
#include "datatypes.h"
#include "shelfdisplaydialog.h"
#include "assemblydialog.h"
#include "useroperationdialog.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


protected:
    void showEvent(QShowEvent *event) override;

private:
    void InitUI();
    void InitData();
    void InitConnect();

private slots:
    void on_btnAddUser_clicked();
    void onLoginSuccess(int roleType);
    void onLoginUserInfo(const UserInfo& userInfo);
    void openPersonalCenter();

private:
    Ui::MainWindow *ui;
    LogInDialog *m_logdlg;
    HomeDialog *m_homedlg;
    ShelfDisplayDialog *m_shelfdlg;
    AssemblyDialog *m_assemblydlg;
    UserManagerDialog *m_userMgrDlg;
    UserOperationDialog *m_userOperation;
    UserInfo m_currentUser;
};
#endif // MAINWINDOW_H
