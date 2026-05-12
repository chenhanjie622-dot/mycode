#ifndef USERASSEMBLYDIALOG_H
#define USERASSEMBLYDIALOG_H

#include <QDialog>
#include <QShowEvent>



namespace Ui {
class UserAssemblyDialog;
}

class UserAssemblyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UserAssemblyDialog(QWidget *parent = nullptr);
    ~UserAssemblyDialog();

protected:
    // ======== 重写显示事件，确保每次切到该页面时刷新数据 ========
    void showEvent(QShowEvent *event) override;

private:
    void InitUI();
    // ======== 动态刷新任务列表的方法 ========
    void refreshTasks();

private:
    Ui::UserAssemblyDialog *ui;
};

#endif // USERASSEMBLYDIALOG_H
