#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "database_manager.h"
#include "adduserdialog.h"
#include "personalcenterdialog.h"
#include <QGuiApplication>
#include <QScreen>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    ,m_logdlg(new LogInDialog(this))
    ,m_homedlg(new HomeDialog(this))
    ,m_shelfdlg(new ShelfDisplayDialog(this))
    ,m_assemblydlg(new AssemblyDialog(this))
    ,m_userMgrDlg(new UserManagerDialog(this))
    ,m_userOperation(new UserOperationDialog(this))
{
    ui->setupUi(this);
    ui->stackedWidget->insertWidget(0, m_logdlg);
    ui->stackedWidget->insertWidget(1, m_homedlg);
    ui->stackedWidget->insertWidget(2, m_shelfdlg);
    ui->stackedWidget->insertWidget(3, m_assemblydlg);
    ui->stackedWidget->insertWidget(4, m_userMgrDlg);
    ui->stackedWidget->insertWidget(5, m_userOperation);

    InitData();
    InitConnect();

    this->setWindowTitle("你好，欢迎使用柔性装配指导系统");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::InitUI()
{
    ui->stackedWidget->insertWidget(0, m_logdlg);
    ui->stackedWidget->insertWidget(1, m_homedlg);
    ui->stackedWidget->insertWidget(2, m_shelfdlg);
    ui->stackedWidget->insertWidget(3, m_assemblydlg);
    ui->stackedWidget->insertWidget(4, m_userMgrDlg);
    ui->stackedWidget->insertWidget(5, m_userOperation);
    ui->stackedWidget->setCurrentIndex(0);
}

void MainWindow::InitData()
{

}

void MainWindow::InitConnect()
{
    // 用 DatabaseManager::instance() 替代 g_DatabaseManager
    connect(&DatabaseManager::instance(), &DatabaseManager::databaseInitialized, [](bool success) {
        if (success) {
            QMetaObject::invokeMethod(&DatabaseManager::instance(), &DatabaseManager::initializetable, Qt::QueuedConnection);
            qDebug() << "Database ready!";
        } else {
            qCritical() << "Database initialization failed!";
        }
    });

    QMetaObject::invokeMethod(&DatabaseManager::instance(), &DatabaseManager::initialize, Qt::QueuedConnection);

    connect(m_logdlg, &LogInDialog::signalLogin, this, [this](){
        ui->stackedWidget->setCurrentIndex(5);
    });

    // 角色信号连接（核心逻辑）
    connect(m_logdlg, &LogInDialog::signalLoginSuccess, this, &MainWindow::onLoginSuccess);
    // ===== 新增：连接完整用户信息信号 =====
    connect(m_logdlg, &LogInDialog::signalLoginUserInfo, this, &MainWindow::onLoginUserInfo);
    // =====================================

    // ===== 首页个人中心按钮信号 =====
    connect(m_homedlg, &HomeDialog::signalOpenPersonalCenter, this, &MainWindow::openPersonalCenter);
    // =========================================
    connect(m_homedlg, &HomeDialog::signalLogout, this, [this]() {
        ui->stackedWidget->setCurrentIndex(0); // 回到登录页
    });

    // 也可以改成弹窗的。
    connect(m_homedlg, &HomeDialog::signalJumpShelf, this, [this]() {
        ui->stackedWidget->setCurrentIndex(2); // 跳转货架
    });

    connect(m_shelfdlg, &ShelfDisplayDialog::signalBackHome, this, [this]() {
        ui->stackedWidget->setCurrentIndex(1); // 跳转货架
    });
    // =================组装界面的跳转逻辑 =================

    // 1. 接收 Home 页面的组装按钮信号，更新数据并切页面
    connect(m_homedlg, &HomeDialog::signalJumpAssembly, this, [this](QMap<QString, QList<StepShowForm*>> data) {
        m_assemblydlg->updateData(data);       // 将最新的教程数据传给组装界面
        ui->stackedWidget->setCurrentIndex(3); // 切换到组装界面（第4页）
    });

    // 2. 接收组装界面的返回信号，切回首页
    connect(m_assemblydlg, &AssemblyDialog::signalBackHome, this, [this]() {
        ui->stackedWidget->setCurrentIndex(1); // 切换回首页（第2页）
    });

    // =================用户管理界面的跳转逻辑 =================
    // 1. 首页点击“用户管理”时，主窗口切换页面
    connect(m_homedlg, &HomeDialog::signalOpenUserManager, this, [this]() {
        m_userMgrDlg->loadUsers(); // 可以在进入前刷新一下数据
        ui->stackedWidget->setCurrentIndex(4);
    });

    // 2. 用户管理点击“关闭”时，切回首页
    connect(m_userMgrDlg, &UserManagerDialog::signalBackHome, this, [this]() {
        ui->stackedWidget->setCurrentIndex(1); // 1 是 HomeDialog 所在的索引
    });

    // 3. 用户管理界面发出的“切换账号/退出”信号处理
    connect(m_userMgrDlg, &UserManagerDialog::signalLogout, this, [this]() {
            ui->stackedWidget->setCurrentIndex(0); // 0 是 LogInDialog 所在的索引
    });

    // 连接用户管理界面的设置信号
    connect(m_userMgrDlg, &UserManagerDialog::signalOpenSettings, this, [this]() {
        ui->stackedWidget->setCurrentIndex(5); // 切换到设置页面
    });

    connect(m_userOperation, &UserOperationDialog::sendIndex, this, [this](int index){
        ui->stackedWidget->setCurrentIndex(index); // 跳回首页
    });
}


void MainWindow::onLoginSuccess(int roleType)
{
    m_homedlg->setUserPermissions(roleType);
}

// ===== 保存当前登录用户信息 =====
void MainWindow::onLoginUserInfo(const UserInfo& userInfo)
{
    m_currentUser = userInfo;
}
// =====================================

// ===== 打开个人中心对话框 =====
void MainWindow::openPersonalCenter()
{
    PersonalCenterDialog dlg(m_currentUser, this); // 传入当前用户信息
    if(dlg.exec() == QDialog::Accepted) {
        // 保存成功后，更新主窗口的当前用户信息
        m_currentUser = dlg.getUpdatedUserInfo();
    }
}
// =====================================

void MainWindow::on_btnAddUser_clicked()
{
    AddUserDialog dlg(this);
    dlg.exec();
}

// 在文件最末尾添加这段代码
// ================= 窗口抢占机制统一交由主窗口管理 =================
void MainWindow::showEvent(QShowEvent *event) {
    QMainWindow::showEvent(event);

    // 获取系统所有的屏幕
    QList<QScreen *> screens = QGuiApplication::screens();

    // 如果屏幕大于等于3块，就把整个主窗口（带着里面所有的子界面）强制扔到触控屏（索引 2）并全屏
    if (screens.size() >= 3) {
        this->setGeometry(screens.at(2)->geometry());
        this->showMaximized();
    }
}
