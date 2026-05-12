#ifndef HOMEDIALOG_H
#define HOMEDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
// 添加前向声明
class StepShowForm;

namespace Ui {
class HomeDialog;
}

class HomeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HomeDialog(QWidget *parent = nullptr);
    ~HomeDialog();
    void setUserManagerBtnVisible(bool visible);

    void InitUI();
    void setUserPermissions(int roleType);  // 新增权限控制方法

protected:
    void showEvent(QShowEvent *event) override;

private:
    void analysisStep(QMap<int, QStringList> stepPath, QString deviceTypeName, QString rackdata = QString());

signals:
    void signalOpenUserManager();
    void signalOpenPersonalCenter();
    void signalLogout();
    void signalJumpShelf();
    void signalJumpAssembly(QMap<QString, QList<StepShowForm*>> data);

private slots:
    void on_btnUserManager_clicked();
    void on_btnPersonalCenter_clicked();
    void on_btnImport_clicked();
    void on_btnAssembly_clicked();
    void on_btnStorageRack_clicked();
    void on_btnLogout_clicked();
    void onDeviceTypeSelected(); // 新增设备类型选择槽函数
    void on_btnDeleteSelected_clicked();
    void on_btnSelectAll_clicked();
    void on_btnUnselectAll_clicked();
    void on_btnSave_clicked();
    void on_btnBackCategory_clicked();
    void on_btnAddSub_clicked();
    void on_btnDelSub_clicked();

private:
    Ui::HomeDialog *ui;
    void updateButtonLayout();  // 新增布局更新方法
    void extracted(QList<QPushButton *> &visibleButtons,
                   QHBoxLayout *&centerLayout);
    void rebuildButtonLayout(); // 重建按钮布局 - 必须在此处声明
    void displayDeviceTutorials(const QString& deviceType); // 添加此方法声明
    void clearTutorialDisplay(); // 清空教程显示的方法
    QString m_selectedDeviceType; // 记录当前选中的设备类型
    QMap<QString, QString> m_rootPath;
    QMap<QString, QList<StepShowForm*>> m_deviceTutorials; // 存储每个设备类型的教程
    QWidget *m_scrollAreaWidget; // 滚动区域的widget
    QVBoxLayout *m_tutorialLayout; // 教程布局
    QHBoxLayout *m_topButtonLayout;
    QPushButton *m_btnSelectAll;
    QPushButton *m_btnUnselectAll;
    QPushButton *m_btnDeleteSelected;
    QPushButton *m_btnSave;
    // ======== 目录层级相关变量 ========
    QWidget *m_categoryControlWidget;
    QPushButton *m_btnBackCategory;
    QPushButton *m_btnAddSub;
    QPushButton *m_btnDelSub;

    bool m_isInSubCategoryView = false; // 标记当前是否处于子目录视图
    QString m_currentMainCategory = ""; // 记录当前选中的主目录名

    void loadMainCategories();
    void loadSubCategories(const QString& mainCategory);
};

#endif // HOMEDIALOG_H
