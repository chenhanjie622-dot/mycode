#include "usersettingsdialog.h"
#include "ui_usersettingsdialog.h" // 这个文件是 Qt 编译 ui 后自动生成的

UserSettingsDialog::UserSettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    // 1. 去掉窗口标准边框（可选，如果需要像图中那样的自定义窗口）
    // setWindowFlags(Qt::FramelessWindowHint);

    // 2. 初始化导航按钮组
    initButtonGroup();
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
