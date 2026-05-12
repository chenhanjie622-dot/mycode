#include "usermanagerdialog.h"
#include "ui_usermanagerdialog.h"
#include <QHeaderView>
#include "database/database_manager.h"
#include "src/home/adduserdialog.h"
#include <QTableWidgetItem>
#include <QMessageBox>
#include <QLineEdit>
#include <QComboBox>


#include "usersettingsdialog.h"

// 唯一的构造函数
// 在UserManagerDialog构造函数中添加
UserManagerDialog::UserManagerDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UserManagerDialog)
{
    ui->setupUi(this);
    connect(ui->btnClose, &QPushButton::clicked, this, [this](){
        emit signalBackHome();
    });
    connect(ui->on_btnSettings, &QPushButton::clicked, this, &UserManagerDialog::on_on_btnSettings_clicked);

    initUI();
    loadUsers();
}

// 析构函数
UserManagerDialog::~UserManagerDialog()
{
    delete ui;
}

void UserManagerDialog::initUI()
{
    // 表格基础设置
    ui->tableUser->setColumnCount(6);
    ui->tableUser->setHorizontalHeaderLabels(
        QStringList() << "ID" << "用户名" << "姓名" << "电话" << "权限" << "创建时间");

    ui->tableUser->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableUser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 表格行为设置
    ui->tableUser->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableUser->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // 设置权限下拉框选项（仅设置一次，避免重复）
    ui->comboBoxQueryRole->clear(); // 清除可能存在的默认项
    ui->comboBoxQueryRole->addItem("全部");
    ui->comboBoxQueryRole->addItem("管理员");
    ui->comboBoxQueryRole->addItem("普通用户");
    ui->comboBoxQueryRole->setCurrentIndex(0);

    // 连接回车键到查询按钮
    connect(ui->lineEditQueryUser, &QLineEdit::returnPressed, this, &UserManagerDialog::on_btnQuery_clicked);
}

// 加载用户列表
void UserManagerDialog::loadUsers()
{
    m_users.clear();

    if(!g_DatabaseManager.queryUserInfoAll(m_users))
        return;

    // 显示所有用户
    populateTable(m_users);
}

void UserManagerDialog::populateTable(const QVector<UserInfo>& users)
{
    clearTable();

    ui->tableUser->setRowCount(users.size());

    for(int i = 0; i < users.size(); i++)
    {
        const UserInfo &info = users[i];

        ui->tableUser->setItem(i, 0, new QTableWidgetItem(QString::number(info.id)));
        ui->tableUser->setItem(i, 1, new QTableWidgetItem(info.username));
        ui->tableUser->setItem(i, 2, new QTableWidgetItem(info.name));
        ui->tableUser->setItem(i, 3, new QTableWidgetItem(info.telephone));

        QString role = info.roleType == "0" ? "管理员" : "普通用户";
        ui->tableUser->setItem(i, 4, new QTableWidgetItem(role));

        ui->tableUser->setItem(i, 5, new QTableWidgetItem(info.createTime));
    }
}

void UserManagerDialog::clearTable()
{
    ui->tableUser->setRowCount(0);
}

// 查询按钮点击
void UserManagerDialog::on_btnQuery_clicked()
{
    QString username = ui->lineEditQueryUser->text().trimmed();
    QString role = ui->comboBoxQueryRole->currentText();

    QVector<UserInfo> filteredUsers = filterUsers(username, role);
    populateTable(filteredUsers);
}

// 重置查询按钮点击
void UserManagerDialog::on_btnResetQuery_clicked()
{
    ui->lineEditQueryUser->clear();
    ui->comboBoxQueryRole->setCurrentIndex(0);
    populateTable(m_users); // 显示所有用户
}

// 返回按钮点击
void UserManagerDialog::on_btnClose_clicked()
{
    emit signalBackHome();
}

// 过滤用户
QVector<UserInfo> UserManagerDialog::filterUsers(const QString& username, const QString& role)
{
    QVector<UserInfo> result;

    for (const auto& user : m_users) {
        // 检查用户名匹配（模糊匹配）
        bool usernameMatch = username.isEmpty() || user.username.contains(username, Qt::CaseInsensitive);

        // 检查权限匹配
        bool roleMatch = true;
        if (role == "管理员") {
            roleMatch = (user.roleType == "0");
        } else if (role == "普通用户") {
            roleMatch = (user.roleType == "1");
        }
        // 如果角色选择"全部"，则总是匹配

        if (usernameMatch && roleMatch) {
            result.append(user);
        }
    }

    return result;
}

// 新增用户按钮点击
void UserManagerDialog::on_btnAddUser_clicked()
{
    AddUserDialog dlg;

    if(dlg.exec()==QDialog::Accepted)
    {
        loadUsers();
    }
}

// 删除用户按钮点击
void UserManagerDialog::on_btnDeleteUser_clicked()
{
    int row = ui->tableUser->currentRow();

    if(row < 0)
    {
        QMessageBox::warning(this,"提示","请选择用户");
        return;
    }

    int id = ui->tableUser->item(row,0)->text().toInt();

    if(QMessageBox::question(this,"确认","确定删除用户?")==QMessageBox::Yes)
    {
        if(g_DatabaseManager.DelUserInfo(id))
        {
            loadUsers();
        }
        else
        {
            QMessageBox::critical(this,"错误","删除失败");
        }
    }
}

// 编辑用户按钮点击
void UserManagerDialog::on_btnEditUser_clicked()
{
    // 1. 检查是否选中行
    int row = ui->tableUser->currentRow();
    if(row < 0) {
        QMessageBox::warning(this, "提示", "请选择要编辑的用户！");
        return;
    }

    // 2. 获取选中用户的完整信息（从m_users里取，比从表格取更完整）
    UserInfo editUser = m_users[row];

    // 3. 打开编辑弹窗 - 使用正确的构造函数进入编辑模式
    AddUserDialog dlg(editUser.userId); // 传递用户ID进入编辑模式

    // 4. 用户确认编辑后更新数据
    if(dlg.exec() == QDialog::Accepted) {
        loadUsers(); // 刷新列表
    }
}

void UserManagerDialog::on_on_btnSettings_clicked()
{
    emit signalOpenSettings();
}

