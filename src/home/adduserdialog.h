#ifndef ADDUSERDIALOG_H
#define ADDUSERDIALOG_H

#include <QDialog>
#include "structs/datatypes.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class AddUserDialog;  // UI类名应与.ui文件名一致
}
QT_END_NAMESPACE

class AddUserDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddUserDialog(QWidget *parent = nullptr);
    explicit AddUserDialog(const QString& userId, QWidget *parent = nullptr); // 编辑模式构造函数
    ~AddUserDialog();

    // 获取用户输入的信息（新增/编辑通用）
    UserInfo getUserInfo() const;

    // 设置要编辑的用户信息（填充到弹窗）
    void setEditUserInfo(const UserInfo &user);

signals:
    void userChanged();  // 用户信息改变信号

private slots:
    void on_btnSave_clicked();
    void on_btnCancel_clicked();  // 取消按钮

private:
    // 初始化UI（区分新增/编辑模式的标题、按钮文案）
    void initUI();
    // 校验输入合法性
    bool validateInput();
    // 加载权限下拉框选项
    void loadRoleOptions();

private:
    Ui::AddUserDialog *ui;  // UI类名应与.ui文件名一致
    UserInfo m_editUser; // 存储要编辑的用户信息（ID用于区分新增/编辑模式）
    bool m_isEditMode = false;   // 标记是否为编辑模式（默认新增）
    QString m_currentUserId;     // 当前操作的用户ID
};

#endif // ADDUSERDIALOG_H
