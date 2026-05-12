#ifndef USEROPERATIONDIALOG_H
#define USEROPERATIONDIALOG_H

#include <QDialog> // 继承自 QDialog
#include <QPropertyAnimation>
#include <QButtonGroup>

#include "usersettingsdialog.h"
#include "userassemblydialog.h"

namespace Ui { class UserOperationDialog; }

class UserOperationDialog : public QDialog // 继承自 QDialog
{
    Q_OBJECT

public:
    explicit UserOperationDialog(QWidget *parent = nullptr);
    ~UserOperationDialog();

signals:
    void sendIndex(int index);

private:
    void InitUI();
    void InitConnect();

protected:
    void resizeEvent(QResizeEvent *event) override; // 重写 resizeEvent

private slots:
    void onCheckBoxToggled(Qt::CheckState state);

private:

    Ui::UserOperationDialog *ui; // UI 对象指针

    QPropertyAnimation *leftAnim;
    QPropertyAnimation *rightAnim;
    UserAssemblyDialog *m_assembly;
    UserSettingsDialog *m_settings;
    bool isLeftSidebarExpanded; // 记录当前侧边栏状态
    QButtonGroup *m_navGroup; // 用于管理导航按钮的选中状态
};
#endif // USEROPERATIONDIALOG_H
