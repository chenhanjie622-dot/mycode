#include "logindialog.h"
#include "ui_logindialog.h"
#include "database/database_manager.h"
#include "database/fileconfigure.h"

#include <QMessageBox>
#include <QString>
#include <QDebug>
#include <QTimer>

LogInDialog::LogInDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LogInDialog)
{
    ui->setupUi(this);
    InitUI();
}

LogInDialog::~LogInDialog()
{
    delete ui;
}

void LogInDialog::InitUI()
{

    QString savedUser = g_fileConfigure.read<QString>("user_use", "");
    QString savedPwd = g_fileConfigure.read<QString>("user_pwd", "");


    ui->lineEdit_user->setText(savedUser);
    ui->lineEdit_password->setText(savedPwd);


    ui->cb_user->setChecked(!savedUser.isEmpty());


    qDebug() << "记住的用户名：" << savedUser << "，密码：" << savedPwd;

    ui->lineEdit_user->setLeadingIcon(QIcon(":/image/user.png"));
    ui->lineEdit_user->setIconSize(QSize(21, 21));


    //ui->passwordEdit->setPlaceholderText(QObject::tr("请输入密码"));
    ui->lineEdit_password->setLeadingIcon(QIcon(":/image/lock.png"));
    ui->lineEdit_password->setIconSize(QSize(21, 21));

    QString generalCheckboxStylesheet =
        "QCheckBox::indicator:unchecked {"
        "    image: url(:/image/password-en.png);"
        "    width: 20px;"
        "    height: 20px;"
        "}"
        "QCheckBox::indicator:checked {"
        "    image: url(:/image/password-dis.png);"
        "    width: 20px;"
        "    height: 20px;"
        "}";

    // --- 创建和配置 ---
    passwordCheckBox = new QCheckBox();
    // ... 设置样式表 ...
    passwordCheckBox->setStyleSheet(generalCheckboxStylesheet);
    passwordCheckBox->setFocusPolicy(Qt::NoFocus);
    passwordCheckBox->setFixedSize(20, 20);

    // --- 设置父控件 ---
    passwordCheckBox->setParent(ui->lineEdit_password);

    // 调整大小以适应样式表定义的指示器尺寸
    passwordCheckBox->adjustSize();

    // --- 连接信号槽 ---
    connect(passwordCheckBox, &QCheckBox::toggled, this, [this](bool checked){
        ui->lineEdit_password->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
    });

    QTimer::singleShot(0, this, [this]() {
        repositionPasswordCheckBox(); // 调用一个单独的函数来执行定位
    });
}

void LogInDialog::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);
    repositionPasswordCheckBox(); // 重新定位
}

void LogInDialog::repositionPasswordCheckBox()
{
    if (passwordCheckBox && ui->lineEdit_password) {
        // 确保控件已经可见并且布局已经应用
        if (passwordCheckBox->isVisible() && ui->lineEdit_password->isVisible()) {
            int requiredPadding = 15;
            int x_pos = ui->lineEdit_password->width() - passwordCheckBox->width() - requiredPadding;
            int y_pos = (ui->lineEdit_password->height() - passwordCheckBox->height()) / 2;
            passwordCheckBox->move(x_pos, y_pos);
        } else {
            qDebug() << "Cannot reposition: LineEdit or CheckBox not visible yet.";
        }
    }
}

void LogInDialog::on_btn_login_clicked()
{

    QString username = ui->lineEdit_user->text().trimmed();
    QString password = ui->lineEdit_password->text().trimmed();

    if (username.isEmpty()) {
        QMessageBox::warning(this, tr("警告"), tr("请输入用户名！"));
        ui->lineEdit_user->setFocus();
        return;
    }
    if (password.isEmpty()) {
        QMessageBox::warning(this, tr("警告"), tr("请输入密码！"));
        ui->lineEdit_password->setFocus();
        return;
    }


    UserInfo info;
    info.username = username;

    if (g_DatabaseManager.queryUserInfo(info)) {
        if (password == info.password) {

            emit signalLoginUserInfo(info);
            emit signalLoginSuccess(info.roleType.toInt());
            emit signalLogin();


            if (ui->cb_user->isChecked()) {

                g_fileConfigure.write<QString>("user_use", username);
                g_fileConfigure.write<QString>("user_pwd", password);
            } else {

                g_fileConfigure.write<QString>("user_use", "");
                g_fileConfigure.write<QString>("user_pwd", "");

                ui->lineEdit_password->clear();
            }
        } else {
            QMessageBox::warning(this, tr("警告"), tr("密码错误！"));
            ui->lineEdit_password->clear();
            ui->lineEdit_password->setFocus();
        }
    } else {
        QMessageBox::warning(this, tr("警告"), tr("用户不存在！"));
        ui->lineEdit_user->clear();
        ui->lineEdit_password->clear();
        ui->lineEdit_user->setFocus();
    }
}
