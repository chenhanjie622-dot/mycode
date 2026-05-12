#ifndef USERMANAGERDIALOG_H
#define USERMANAGERDIALOG_H

#include <QDialog>
#include <QVector>
#include "structs/datatypes.h"

namespace Ui {
class UserManagerDialog;
}

class UserManagerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UserManagerDialog(QWidget *parent = nullptr);
    ~UserManagerDialog();

    void loadUsers();   // 加载用户列表

private slots:
    void on_btnAddUser_clicked();
    void on_btnDeleteUser_clicked();
    void on_btnEditUser_clicked();
    void on_btnQuery_clicked();
    void on_btnResetQuery_clicked();
    void on_btnClose_clicked();

    void on_on_btnSettings_clicked();

private:
    void initUI();
    void populateTable(const QVector<UserInfo>& users);
    void clearTable();
    QVector<UserInfo> filterUsers(const QString& username, const QString& role);

private:
    Ui::UserManagerDialog *ui;
    QVector<UserInfo> m_users;
    QVector<UserInfo> m_filteredUsers; // 存储过滤后的用户列表

signals:
    void signalBackHome();
    void signalOpenSettings(); // 打开设置界面的信号
    void signalLogout(); // 转发退出/切换登录信号

};

#endif
