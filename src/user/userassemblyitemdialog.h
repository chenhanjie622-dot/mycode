#ifndef USERASSEMBLYITEMDIALOG_H
#define USERASSEMBLYITEMDIALOG_H
#include <QWidget>
#include <QDialog>
#include <QPaintEvent>

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
    void setTaskInfo(const QString& deviceName, int stepCount, const QString& firstImagePath, const QString& taskDesc);

    void setSelected(bool selected);
    bool isSelected() const;

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    Ui::UserAssemblyItemDialog *ui;
    bool m_selected;
};

#endif // USERASSEMBLYITEMDIALOG_H
