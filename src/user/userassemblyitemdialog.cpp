#include "userassemblyitemdialog.h"
#include "ui_userassemblyitemdialog.h"
#include <QDateTime>




UserAssemblyItemDialog::UserAssemblyItemDialog(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::UserAssemblyItemDialog)
{
    ui->setupUi(this);

    // 【核心修复 1】强制开启背景自动填充
    this->setAutoFillBackground(true);

    // 【核心修复 2】确保样式表属性被激活
    this->setAttribute(Qt::WA_StyledBackground, true);
}

UserAssemblyItemDialog::~UserAssemblyItemDialog()
{
    delete ui;
}

void UserAssemblyItemDialog::setTaskInfo(const QString& deviceName, int stepCount, const QString& firstImagePath)
{
    // 1. 设置设备名称 (UI中的 label_8)
    ui->label_8->setText(deviceName);

    // 2. 设置模块/步骤数量 (UI中的 label_2)
    ui->label_2->setText(QString::number(stepCount));

    // 3. 动态计算个预计耗时 (假设每个步骤需要 5 分钟)，设置到 label_6
    ui->label_6->setText(QString::number(stepCount * 5));

    // 4. 设置封面图片 (UI中的 label_10)
    if (!firstImagePath.isEmpty()) {
        // 注意：Qt的样式表识别本地绝对路径时，需要将反斜杠替换为正斜杠
        QString fixedPath = firstImagePath;
        fixedPath.replace("\\", "/");
        QString style = QString("border-image: url('%1');").arg(fixedPath);
        ui->label_10->setStyleSheet(style);
    }
}