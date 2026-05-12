#include "userassemblydialog.h"
#include "ui_userassemblydialog.h"

#include "userassemblyitemdialog.h"

// 引入数据库和文件处理头文件
#include "database_manager.h"
#include "database/fileprocessing.h"
#include <QDir>
#include <QFile>
#include <QTextStream>


#include <QDebug>
#include <QElapsedTimer>


UserAssemblyDialog::UserAssemblyDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::UserAssemblyDialog)
{
    QElapsedTimer timer;
    timer.start();

    qint64 t0 = timer.elapsed();
    ui->setupUi(this);
    qDebug() << "[Assembly] setupUi 耗时:" << (timer.elapsed() - t0) << "ms";

    qint64 t1 = timer.elapsed();
    InitUI();
    qDebug() << "[Assembly] InitUI 耗时:" << (timer.elapsed() - t1) << "ms";

    qDebug() << "[Assembly] 构造函数总耗时:" << timer.elapsed() << "ms";
}

UserAssemblyDialog::~UserAssemblyDialog()
{
    delete ui;
}

void UserAssemblyDialog::InitUI()
{




    QElapsedTimer uiTimer;
    uiTimer.start();

    // 1. 【外层背景】确保外层 Dialog 开启背景绘制
    this->setAutoFillBackground(true);
    // 设置外层样式（包含背景图）
    this->setStyleSheet(
        "UserAssemblyDialog {"
        "   background-image: url(:/image/tasklist.png);" // 替换成你的外层图
        "   background-repeat: no-repeat;"
        "   background-position: center;"
        "}"
        );

    // 2. 【关键修复】让滚动区域透明
    // 这一步是为了让 scrollArea 本身不遮挡外层 Dialog 的背景
    ui->scrollArea->setAutoFillBackground(false);
    ui->scrollArea->viewport()->setAutoFillBackground(false);
    ui->scrollArea->viewport()->setStyleSheet("background-color: transparent;");

    // 3. 【最关键修复】让滚动区域的“内容容器”透明
    // 默认情况下，scrollAreaWidgetContents 是白色的，会挡住外层背景！
    ui->scrollAreaWidgetContents->setAutoFillBackground(false);
    ui->scrollAreaWidgetContents->setStyleSheet("background-color: transparent;");

    qint64 t_items = uiTimer.elapsed();
    // 4. 循环创建 Item
    for(int i = 0; i < 4; i++){
        qint64 t_item = uiTimer.elapsed();
        UserAssemblyItemDialog *assitem = new UserAssemblyItemDialog(ui->scrollAreaWidgetContents);
        ui->verticalLayout_item->insertWidget(ui->verticalLayout_item->count() - 1, assitem);
        qDebug() << "[Assembly::InitUI] 创建 Item" << i << "耗时:" << (uiTimer.elapsed() - t_item) << "ms";
    }
    qDebug() << "[Assembly::InitUI] 创建所有 Item 总耗时:" << (uiTimer.elapsed() - t_items) << "ms";

}

void UserAssemblyDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    refreshTasks(); // 每次界面显示时，刷新任务列表
}

void UserAssemblyDialog::refreshTasks()
{
    // 1. 清空旧任务卡片
    for (int i = ui->verticalLayout_item->count() - 2; i >= 0; --i) {
        QWidget* w = ui->verticalLayout_item->itemAt(i)->widget();
        if (w) {
            ui->verticalLayout_item->removeWidget(w);
            w->deleteLater();
        }
    }

    // 2. 🚩【核心修复】：收集所有的任务标识（包含主目录和所有子目录）
    QStringList mainDevices = {"PCR安装", "大型一体机安装", "桌面一体机安装", "手持设备安装", "其他设备安装"};
    QStringList allTasks = mainDevices; // 保留主目录，兼容以前的数据

    // 遍历查出所有的子目录，拼接成 "主目录_子目录" 的格式
    for (const QString& mainCat : mainDevices) {
        QStringList subCats;
        DatabaseManager::instance().GetSubCategories(mainCat, subCats);
        for (const QString& subCat : subCats) {
            allTasks.append(mainCat + "_" + subCat);
        }
    }

    // 3. 遍历查询生成卡片
    for (const QString& deviceName : allTasks) {
        StepInfo info;
        info.stepname = deviceName;

        if (DatabaseManager::instance().queryStepInfo(info) && !info.steppath.isEmpty()) {

            // 🚩 【全新逻辑】：直接去大文件夹 (info.steppath) 里面找 txt 文件作为任务介绍
            QString taskDesc = "";
            QDir rootDir(info.steppath);
            QStringList txtFilters;
            txtFilters << "*.txt";
            // 扫描大文件夹下的 txt 文件（不进子文件夹）
            QFileInfoList txtFiles = rootDir.entryInfoList(txtFilters, QDir::Files);
            if (!txtFiles.isEmpty()) {
                QFile txtFile(txtFiles.first().absoluteFilePath());
                if (txtFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QTextStream in(&txtFile);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                    in.setEncoding(QStringConverter::Utf8);
#else
                    in.setCodec("UTF-8");
#endif
                    taskDesc = in.readAll();
                    txtFile.close();
                }
            }

            // 继续原来的解析小文件夹逻辑，获取步骤数和封面图
            int key = 0;
            FileProcessing::StepPathClear();
            FileProcessing::FindLeafFolders(info.steppath, key);
            QMap<int, QStringList> stepMap = FileProcessing::GetStepPath();

            int stepCount = stepMap.size();
            QString firstImagePath = "";

            if (stepCount > 0) {
                QStringList firstStepData = stepMap.first();
                if (firstStepData.size() > 1) {
                    firstImagePath = firstStepData.at(1);
                }

                // 把标识（如 "PCR安装_N1"）美化为 "PCR安装 - N1"
                QString displayTitle = deviceName;
                displayTitle.replace("_", " - ");

                UserAssemblyItemDialog *assitem = new UserAssemblyItemDialog(ui->scrollAreaWidgetContents);
                // 🚩 将刚才在大文件夹里读到的 taskDesc 传进去
                assitem->setTaskInfo(displayTitle, stepCount, firstImagePath, taskDesc);
                ui->verticalLayout_item->insertWidget(ui->verticalLayout_item->count() - 1, assitem);
            }
        }
    }
}