#include "usersettingsdialog.h"
#include "ui_usersettingsdialog.h"
#include <QDebug>
#include <QElapsedTimer>

UserSettingsDialog::UserSettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    QElapsedTimer timer;
    timer.start();

    qint64 t0 = timer.elapsed();
    ui->setupUi(this);
    qDebug() << "[Settings] setupUi 耗时:" << (timer.elapsed() - t0) << "ms";

    qint64 t1 = timer.elapsed();
    initButtonGroup();
    qDebug() << "[Settings] initButtonGroup 耗时:" << (timer.elapsed() - t1) << "ms";

    qDebug() << "[Settings] 构造函数总耗时:" << timer.elapsed() << "ms";
}

UserSettingsDialog::~UserSettingsDialog()
{
    delete ui;
}

void UserSettingsDialog::initButtonGroup()
{
    m_navGroup = new QButtonGroup(this);

    // 将 .ui 文件中的按钮加入组，并设置互斥
    m_navGroup->addButton(ui->btn_network);
    m_navGroup->addButton(ui->btn_hardware);
    m_navGroup->addButton(ui->btn_upgrade);
    m_navGroup->addButton(ui->btn_about);

    m_navGroup->setExclusive(true); // 确保同一时间只有一个被选中

    //连接切换账号按钮的点击事件
    connect(ui->btn_switch, &QPushButton::clicked, this, [this](){
        emit signalSwitchAccount();
        this->accept(); // 点击后关闭设置对话框
    });

}
