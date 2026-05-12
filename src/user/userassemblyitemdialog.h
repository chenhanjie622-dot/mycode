#ifndef USERASSEMBLYITEMDIALOG_H
#define USERASSEMBLYITEMDIALOG_H
#include <QWidget>
#include <QDialog>

namespace Ui {
class UserAssemblyItemDialog;
}

class UserAssemblyItemDialog : public QWidget
{
    Q_OBJECT

public:
    explicit UserAssemblyItemDialog(QWidget *parent = nullptr);
    ~UserAssemblyItemDialog();
    // ======== 用于动态设置任务卡片信息的接口 ========
    void setTaskInfo(const QString& deviceName, int stepCount, const QString& firstImagePath);

private:
    Ui::UserAssemblyItemDialog *ui;
};

#endif // USERASSEMBLYITEMDIALOG_H
