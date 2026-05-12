#include "userassemblydialog.h"
#include "ui_userassemblydialog.h"

#include "userassemblyitemdialog.h"
// 引入数据库和文件处理头文件
#include "database_manager.h"
#include "database/fileprocessing.h"


UserAssemblyDialog::UserAssemblyDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::UserAssemblyDialog)
{
    ui->setupUi(this);

    InitUI();
}

UserAssemblyDialog::~UserAssemblyDialog()
{
    delete ui;
}

void UserAssemblyDialog::InitUI()
{


}

void UserAssemblyDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    refreshTasks(); // 每次界面显示时，刷新任务列表
}

void UserAssemblyDialog::refreshTasks()
{
    // 1. 清空当前容器中除了底部弹簧(spacer)以外的所有旧任务卡片
    // 从倒数第二个元素开始删（避开最后一个占位用的 verticalSpacer）
    for (int i = ui->verticalLayout_item->count() - 2; i >= 0; --i) {
        QWidget* w = ui->verticalLayout_item->itemAt(i)->widget();
        if (w) {
            ui->verticalLayout_item->removeWidget(w);
            w->deleteLater();
        }
    }

    // 2. 定义系统支持的所有设备类型列表
    QStringList devices = {"PCR安装", "大型一体机安装", "桌面一体机安装", "手持设备安装", "其他设备安装"};

    // 3. 遍历数据库查询是否导入过对应教程
    for (const QString& deviceName : devices) {
        StepInfo info;
        info.stepname = deviceName;

        // 如果数据库里查得到路径，说明导入过该设备的教程
        if (DatabaseManager::instance().queryStepInfo(info) && !info.steppath.isEmpty()) {

            // 解析文件夹提取步骤信息
            int key = 0;
            FileProcessing::StepPathClear();
            FileProcessing::FindLeafFolders(info.steppath, key);
            QMap<int, QStringList> stepMap = FileProcessing::GetStepPath();

            int stepCount = stepMap.size();
            QString firstImagePath = "";

            // 提取第一个步骤(0号索引)的封面图片
            if (stepCount > 0) {
                QStringList firstStepData = stepMap.first();
                if (firstStepData.size() > 1) {
                    firstImagePath = firstStepData.at(1); // 索引 1 存放的是图片路径
                }

                // 创建并插入动态任务卡片
                UserAssemblyItemDialog *assitem = new UserAssemblyItemDialog(ui->scrollAreaWidgetContents);
                assitem->setTaskInfo(deviceName, stepCount, firstImagePath);

                // 插入到布局中，排在最后的底部弹簧前面
                ui->verticalLayout_item->insertWidget(ui->verticalLayout_item->count() - 1, assitem);
            }
        }
    }
}