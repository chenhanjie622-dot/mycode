#include "src/home/adduserdialog.h"
#include "ui_adduserdialog.h"  // UI头文件名应与.ui文件名一致
#include "database/database_manager.h"
#include "structs/datatypes.h"

#include <QDateTime>
#include <QMessageBox>
#include <QDebug>
#include <QRegularExpression>  // 替换QRegExp>
#include <QVector>

AddUserDialog::AddUserDialog(QWidget *parent)
    : QDialog(parent),
    ui(new Ui::AddUserDialog)  // UI类名应与.ui文件名一致
{
    ui->setupUi(this);
    // 确保只连接一次信号
    disconnect(ui->btnSave, &QPushButton::clicked, nullptr, nullptr);
    disconnect(ui->btnCancel, &QPushButton::clicked, nullptr, nullptr);

    connect(ui->btnSave, &QPushButton::clicked, this, &AddUserDialog::on_btnSave_clicked);
    connect(ui->btnCancel, &QPushButton::clicked, this, &AddUserDialog::on_btnCancel_clicked);
    initUI();
}

// 编辑模式构造函数
// 编辑模式构造函数
AddUserDialog::AddUserDialog(const QString& userId, QWidget *parent)
    : QDialog(parent),
    ui(new Ui::AddUserDialog),  // UI类名应与.ui文件名一致
    m_currentUserId(userId),
    m_isEditMode(true)
{
    ui->setupUi(this);
    // 确保只连接一次信号
    disconnect(ui->btnSave, &QPushButton::clicked, nullptr, nullptr);
    disconnect(ui->btnCancel, &QPushButton::clicked, nullptr, nullptr);

    connect(ui->btnSave, &QPushButton::clicked, this, &AddUserDialog::on_btnSave_clicked);
    connect(ui->btnCancel, &QPushButton::clicked, this, &AddUserDialog::on_btnCancel_clicked);
    initUI();

    // 由于数据库管理器没有按ID查询的函数，我们需要遍历所有用户来查找特定ID的用户
    QVector<UserInfo> allUsers;
    if (g_DatabaseManager.queryUserInfoAll(allUsers)) {
        for (const auto& user : allUsers) {
            if (user.userId == userId) {
                setEditUserInfo(user);
                break;
            }
        }
    }
}

AddUserDialog::~AddUserDialog()
{
    delete ui;
}

void AddUserDialog::initUI()
{
    // 加载权限选项
    loadRoleOptions();

    // 新增模式：标题为"新增用户"；编辑模式：标题为"编辑用户"
    setWindowTitle(m_isEditMode ? "编辑用户" : "新增用户");

    // 设置标题标签文字
    ui->labelDialogTitle->setText(m_isEditMode ?
                                      "<html><head/><body><p align=\"center\"><span style=\" font-size:12pt; font-weight:600;\">编辑用户</span></p></body></html>" :
                                      "<html><head/><body><p align=\"center\"><span style=\" font-size:12pt; font-weight:600;\">新增用户</span></p></body></html>");

    // 编辑模式下：密码框可留空（不修改密码则沿用原密码）
    if (m_isEditMode) {
        ui->lineEditPassword->setPlaceholderText("留空则不修改密码");
        ui->lineEditConfirmPassword->setPlaceholderText("留空则不修改密码");
    } else {
        ui->lineEditPassword->setPlaceholderText("请输入密码");
        ui->lineEditConfirmPassword->setPlaceholderText("请再次输入密码");
    }
}

void AddUserDialog::loadRoleOptions()
{
    // 清空现有选项
    ui->comboBoxRole->clear();

    // 添加权限选项
    ui->comboBoxRole->addItem("普通用户", "1");
    ui->comboBoxRole->addItem("管理员", "0");

    // 默认选择普通用户
    if (!m_isEditMode) {
        ui->comboBoxRole->setCurrentIndex(0);
    }
}

void AddUserDialog::setEditUserInfo(const UserInfo &user)
{
    m_editUser = user;
    m_isEditMode = true;

    // 填充已有数据到控件
    ui->lineEditUsername->setText(user.username);
    ui->lineEditRealName->setText(user.name);
    ui->lineEditPhone->setText(user.telephone);

    // 根据权限类型选择对应的角色
    int roleIndex = -1;
    if (user.roleType == "0") {
        roleIndex = ui->comboBoxRole->findData("0");
    } else if (user.roleType == "2") {
        roleIndex = ui->comboBoxRole->findData("2");
    } else {
        roleIndex = ui->comboBoxRole->findData("1");
    }

    if (roleIndex >= 0) {
        ui->comboBoxRole->setCurrentIndex(roleIndex);
    }

    // 在编辑模式下，用户名通常不允许修改
    ui->lineEditUsername->setEnabled(false);
}

UserInfo AddUserDialog::getUserInfo() const
{
    UserInfo info;
    info.userId = m_isEditMode ? m_editUser.userId : "";
    info.username = ui->lineEditUsername->text().trimmed();
    info.name = ui->lineEditRealName->text().trimmed();
    info.telephone = ui->lineEditPhone->text().trimmed();

    // 获取选中的角色值
    int currentIndex = ui->comboBoxRole->currentIndex();
    if (currentIndex >= 0) {
        info.roleType = ui->comboBoxRole->itemData(currentIndex).toString();
    }

    // 如果密码字段为空，则保持原密码不变（编辑模式）
    QString password = ui->lineEditPassword->text().trimmed();
    if (!password.isEmpty()) {
        info.password = password;
    } else if (m_isEditMode) {
        // 编辑模式下如果密码为空，保持原来的密码
        info.password = m_editUser.password;
    }

    return info;
}

bool AddUserDialog::validateInput()
{
    QString username = ui->lineEditUsername->text().trimmed();
    QString realName = ui->lineEditRealName->text().trimmed();
    QString phone = ui->lineEditPhone->text().trimmed();
    QString password = ui->lineEditPassword->text().trimmed();
    QString confirmPassword = ui->lineEditConfirmPassword->text().trimmed();

    // 新增模式：用户名必须非空
    if (!m_isEditMode) {
        if (username.isEmpty()) {
            QMessageBox::warning(this, "提示", "用户名不能为空！");
            ui->lineEditUsername->setFocus();
            return false;
        }

        // 检查用户名是否已存在 - 使用新增的数据库函数
        if (g_DatabaseManager.IsUsernameExists(username)) {
            QMessageBox::warning(this, "提示", "用户名已存在，请更换用户名！");
            ui->lineEditUsername->setFocus();
            return false;
        }
    }

    // 如果密码字段不为空，需要校验密码强度和一致性
    if (!password.isEmpty()) {
        // 密码长度校验
        if (password.length() < 6) {
            QMessageBox::warning(this, "提示", "密码长度不能少于6位！");
            ui->lineEditPassword->setFocus();
            return false;
        }

        // 确认密码校验
        if (password != confirmPassword) {
            QMessageBox::warning(this, "提示", "两次输入的密码不一致！");
            ui->lineEditConfirmPassword->setFocus();
            return false;
        }
    }

    // 真实姓名校验
    if (realName.isEmpty()) {
        QMessageBox::warning(this, "提示", "真实姓名不能为空！");
        ui->lineEditRealName->setFocus();
        return false;
    }

    // 电话号码校验
    if (!phone.isEmpty()) {
        // 可以在这里添加长度限制（比如最多20个字符）
        if (phone.length() > 20) {
            QMessageBox::warning(this, "提示", "手机号码过长！");
            ui->lineEditPhone->setFocus();
            return false;
        }
    }

    return true;
}

void AddUserDialog::on_btnSave_clicked()
{
    if (!validateInput()) {
        return;
    }

    UserInfo info = getUserInfo();

    // 根据模式决定是新增还是更新
    bool ret = false;
    if (m_isEditMode) {
        // 编辑模式：更新用户信息
        // 首先查询当前用户的所有信息以获取数据库记录ID
        UserInfo searchUser;
        searchUser.username = m_editUser.username; // 使用原始用户名进行查询

        if (g_DatabaseManager.queryUserInfo(searchUser)) {
            // 将要更新的信息复制到查询结果上，保留数据库ID
            UserInfo updateInfo = searchUser;
            updateInfo.name = info.name;
            updateInfo.telephone = info.telephone;
            updateInfo.roleType = info.roleType;

            // 如果密码字段不为空，则更新密码
            if (!info.password.isEmpty()) {
                updateInfo.password = info.password;
            }

            // 更新修改时间
            updateInfo.modifyTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

            ret = g_DatabaseManager.UpdateUserInfo(updateInfo);
        } else {
            QMessageBox::critical(this, "错误", "找不到要编辑的用户信息！");
            return;
        }
    } else {
        // 新增模式：添加新用户
        // 生成用户ID
        info.userId = QString::number(QDateTime::currentSecsSinceEpoch());
        info.createTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        info.modifyTime = info.createTime; // 新增时修改时间等于创建时间

        ret = g_DatabaseManager.AddUserInfo(info);
    }

    // 只显示一次提示信息
    if (ret) {
        QString tip = m_isEditMode ? "用户编辑成功！" : "用户添加成功！";
        QMessageBox::information(this, "成功", tip);

        // 发送信号通知父窗口刷新数据
        emit userChanged();

        accept(); // 关闭弹窗并返回Accepted状态
    } else {
        QString tip = m_isEditMode ? "编辑用户失败！" : "添加用户失败！";
        QMessageBox::critical(this, "错误", tip);
    }
}

void AddUserDialog::on_btnCancel_clicked()
{
    reject(); // 关闭弹窗并返回Rejected状态
}
