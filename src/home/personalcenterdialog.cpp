#include "personalcenterdialog.h"
#include "ui_personalcenterdialog.h"
#include "database/database_manager.h"
#include <QMessageBox>
#include <QDateTime>

PersonalCenterDialog::PersonalCenterDialog(const UserInfo& currentUser, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PersonalCenterDialog)
    , m_user(currentUser)
{
    ui->setupUi(this);

    // 设置窗口标志以允许调整大小
    setWindowFlags(windowFlags() | Qt::CustomizeWindowHint | Qt::WindowMinMaxButtonsHint);

    // 初始化界面
    initUI();
}

PersonalCenterDialog::~PersonalCenterDialog()
{
    delete ui;
}

void PersonalCenterDialog::initUI()
{
    // 填充用户信息
    ui->lineEditUsername->setText(m_user.username);
    ui->lineEditName->setText(m_user.name);
    ui->lineEditTelephone->setText(m_user.telephone);
    ui->lineEditRole->setText(m_user.roleType == "0" ? "管理员" : "普通用户");
    ui->lineEditCreateTime->setText(m_user.createTime);
    ui->lineEditModifyTime->setText(m_user.modifyTime.isEmpty() ? "未修改过" : m_user.modifyTime);

    // 设置只读字段的样式
    ui->lineEditUsername->setStyleSheet("QLineEdit { background-color: #f0f0f0; color: #666666; }");
    ui->lineEditRole->setStyleSheet("QLineEdit { background-color: #f0f0f0; color: #666666; }");
    ui->lineEditCreateTime->setStyleSheet("QLineEdit { background-color: #f0f0f0; color: #666666; }");
    ui->lineEditModifyTime->setStyleSheet("QLineEdit { background-color: #f0f0f0; color: #666666; }");
}

void PersonalCenterDialog::on_btnSave_clicked()
{
    if (!validateInput()) {
        return;
    }

    // 更新用户信息
    m_user.name = ui->lineEditName->text().trimmed();
    m_user.telephone = ui->lineEditTelephone->text().trimmed();

    QString newPwd = ui->lineEditNewPwd->text().trimmed();
    if (!newPwd.isEmpty()) {
        m_user.password = newPwd;
    }

    m_user.modifyTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    bool updateOk = g_DatabaseManager.UpdateUserInfo(m_user);
    if (updateOk) {
        QMessageBox::information(this, "提示", "保存成功！");
        ui->lineEditModifyTime->setText(m_user.modifyTime);
        ui->lineEditOldPwd->clear();
        ui->lineEditNewPwd->clear();
        ui->lineEditConfirmPwd->clear();
        accept();
    } else {
        QMessageBox::critical(this, "错误", "保存失败！");
    }
}

void PersonalCenterDialog::on_btnCancel_clicked()
{
    reject();
}

void PersonalCenterDialog::on_btnTogglePwd_clicked()
{
    QLineEdit::EchoMode mode = ui->lineEditOldPwd->echoMode();
    QLineEdit::EchoMode newMode = (mode == QLineEdit::Password) ? QLineEdit::Normal : QLineEdit::Password;

    ui->lineEditOldPwd->setEchoMode(newMode);
    ui->lineEditNewPwd->setEchoMode(newMode);
    ui->lineEditConfirmPwd->setEchoMode(newMode);

    ui->btnTogglePwd->setText((newMode == QLineEdit::Password) ? "显示密码" : "隐藏密码");
}

bool PersonalCenterDialog::validateInput()
{
    QString name = ui->lineEditName->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "提示", "姓名不能为空！");
        ui->lineEditName->setFocus();
        return false;
    }

    QString oldPwd = ui->lineEditOldPwd->text().trimmed();
    QString newPwd = ui->lineEditNewPwd->text().trimmed();
    QString confirmPwd = ui->lineEditConfirmPwd->text().trimmed();

    if (!newPwd.isEmpty()) {
        if (oldPwd.isEmpty()) {
            QMessageBox::warning(this, "提示", "修改密码必须输入旧密码！");
            ui->lineEditOldPwd->setFocus();
            return false;
        }
        if (oldPwd != m_user.password) {
            QMessageBox::warning(this, "提示", "旧密码错误！");
            ui->lineEditOldPwd->clear();
            ui->lineEditOldPwd->setFocus();
            return false;
        }
        if (newPwd != confirmPwd) {
            QMessageBox::warning(this, "提示", "新密码和确认密码不一致！");
            ui->lineEditConfirmPwd->clear();
            ui->lineEditConfirmPwd->setFocus();
            return false;
        }
        if (newPwd == oldPwd) {
            QMessageBox::warning(this, "提示", "新密码不能和旧密码一样！");
            ui->lineEditNewPwd->clear();
            ui->lineEditConfirmPwd->clear();
            ui->lineEditNewPwd->setFocus();
            return false;
        }
    }

    return true;
}

UserInfo PersonalCenterDialog::getUpdatedUserInfo() const
{
    return m_user;
}
