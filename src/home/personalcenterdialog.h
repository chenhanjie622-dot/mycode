#ifndef PERSONALCENTERDIALOG_H
#define PERSONALCENTERDIALOG_H

#include <QDialog>
#include "structs/datatypes.h"

namespace Ui {
class PersonalCenterDialog;
}

class PersonalCenterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PersonalCenterDialog(const UserInfo& currentUser, QWidget *parent = nullptr);
    ~PersonalCenterDialog();
    UserInfo getUpdatedUserInfo() const;

private slots:
    void on_btnSave_clicked();
    void on_btnCancel_clicked();
    void on_btnTogglePwd_clicked();

private:
    Ui::PersonalCenterDialog *ui;
    UserInfo m_user;
    void initUI();
    bool validateInput();
};

#endif // PERSONALCENTERDIALOG_H
