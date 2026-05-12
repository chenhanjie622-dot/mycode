#include "useroperationdialog.h"
#include "qstyle.h"
#include "ui_useroperationdialog.h"

#include <QApplication>
#include <QCheckBox>
#include <QFrame>
#include <QVBoxLayout>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QResizeEvent>

// useroperationdialog.cpp

UserOperationDialog::UserOperationDialog(QWidget *parent)
    : QDialog(parent),
    ui(new Ui::UserOperationDialog),
    leftAnim(nullptr),
    rightAnim(nullptr),
    m_assembly(new UserAssemblyDialog(this)),
    m_settings(new UserSettingsDialog(this)),
    // 关键：如果初始 checkBox 是未选中的，则初始状态应为收缩
    // 请确保 UI 文件中的 checkBox 初始状态是 Unchecked
    isLeftSidebarExpanded(false)
{
    ui->setupUi(this);

    InitUI();

    InitConnect();
}

UserOperationDialog::~UserOperationDialog()
{

}

void UserOperationDialog::InitUI()
{
    QFrame *leftSidebar = ui->leftSidebar;
    QFrame *rightSidebar = ui->rightSidebar;
    QCheckBox *checkBox = ui->cbShrink;
    checkBox->setFocusPolicy(Qt::NoFocus);
    int collapsedWidth = 60;

    // 初始时设置为收缩状态的最小宽度
    leftSidebar->setMinimumWidth(collapsedWidth);
    rightSidebar->setMinimumWidth(collapsedWidth);

    // 创建动画对象
    leftAnim = new QPropertyAnimation(leftSidebar, "minimumWidth");
    leftAnim->setDuration(300);
    leftAnim->setEasingCurve(QEasingCurve::OutCubic);

    rightAnim = new QPropertyAnimation(rightSidebar, "minimumWidth");
    rightAnim->setDuration(300);
    rightAnim->setEasingCurve(QEasingCurve::OutCubic);

    // 连接到 checkStateChanged(Qt::CheckState) 信号
    connect(checkBox, &QCheckBox::checkStateChanged, this, &UserOperationDialog::onCheckBoxToggled);

    QSize iconS(40, 40);

    // --- 1. 处理 tbtn_icon (保持不变) ---
    QIcon icon_title(":/image/icon.png");
    icon_title.addFile(":/image/icon.png", QSize(), QIcon::Disabled, QIcon::On);
    icon_title.addFile(":/image/icon.png", QSize(), QIcon::Disabled, QIcon::Off);
    ui->tbtn_icon->setIcon(icon_title);
    ui->tbtn_icon->setIconSize(iconS);

    // --- 2. 处理其他按钮图标 (保持不变) ---
    QIcon icon_assembly(":/image/assembly.png");
    icon_assembly.addFile(":/image/assemblys.png", QSize(), QIcon::Normal, QIcon::On);
    ui->tbtn_assembly->setIcon(icon_assembly);
    ui->tbtn_assembly->setIconSize(iconS);

    QIcon icon_setting(":/image/setting.png");
    icon_setting.addFile(":/image/settings.png", QSize(), QIcon::Normal, QIcon::On);
    ui->tbtn_setting->setIcon(icon_setting);
    ui->tbtn_setting->setIconSize(iconS);

    // --- 3. 设置状态与默认选中 ---
    ui->tbtn_icon->setCheckable(false);
    ui->tbtn_icon->setEnabled(false);

    ui->tbtn_assembly->setCheckable(true);
    ui->tbtn_assembly->setChecked(true);  // 默认选中

    ui->tbtn_setting->setCheckable(true);

    // --- 4. 初始化 Frame 背景 ---~
    // 默认显示装配页面的背景
    ui->assembly_frame->setStyleSheet("QFrame {border-image: url(:/image/selectedBG.png);}");
    ui->setting_frame->setStyleSheet("background-color: transparent; border: none;");
    ui->icon_frame->setStyleSheet("background-color: transparent; border: none;");

    // --- 5. 设置字号 ---
    QFont font = ui->tbtn_icon->font();
    font.setPointSize(14);
    ui->tbtn_icon->setFont(font);

    font.setPointSize(18);
    ui->tbtn_assembly->setFont(font);
    ui->tbtn_setting->setFont(font);

    // --- 6. 设置文字 ---
    QString empty = "  ";
    ui->tbtn_assembly->setText(empty + tr("装配"));
    ui->tbtn_setting->setText(empty + tr("设置"));
    ui->tbtn_icon->setText(empty + tr("柔性装配指导系统"));

    // --- 7. 核心：设置按钮样式 (控制字体颜色) ---
    // 使用样式表控制：默认颜色 #7D8890，选中颜色 #00B9FF
    // 同时设置 background-color 防止背景图穿透按钮
    QString btnStyle =
        "QToolButton {"
        "   color: #%1;"                // 默认字体颜色
        "   background-color: transparent;" // 按钮背景透明（但在逻辑上不透明以遮挡下层）
        "   border: none;"
        "}"
        "QToolButton:checked {"
        "   color: #00B9FF;"                // 选中时字体颜色
        "}";

    ui->tbtn_assembly->setStyleSheet(btnStyle.arg("7D8890"));
    ui->tbtn_setting->setStyleSheet(btnStyle.arg("7D8890"));
    ui->tbtn_icon->setStyleSheet(btnStyle.arg("FFFFFF")); // 虽然 icon 按钮不选中，但统一样式比较好

    // --- 8. 页面与信号槽 ---
    ui->stackedWidget->setCurrentIndex(0); // 默认显示第0页
    ui->stackedWidget->insertWidget(0, m_assembly);
    ui->stackedWidget->insertWidget(1, m_settings);

    m_navGroup = new QButtonGroup(this);
    m_navGroup->addButton(ui->tbtn_assembly, 0);
    m_navGroup->addButton(ui->tbtn_setting, 1);
    m_navGroup->setExclusive(true);
}

void UserOperationDialog::InitConnect()
{
    connect(m_settings, &UserSettingsDialog::signalSwitchAccount, this, [this](){
        sendIndex(1);
    });

    connect(m_navGroup, &QButtonGroup::idClicked, this, [this](int buttonId){
        ui->stackedWidget->setCurrentIndex(buttonId);
    });

    // 连接“装配”按钮
    connect(ui->tbtn_assembly, &QToolButton::clicked, this, [this](bool checked) {
        if (checked) {
            ui->assembly_frame->setStyleSheet("QFrame {border-image: url(:/image/selectedBG.png);}");
        }

        ui->setting_frame->setStyleSheet("QFrame { background: transparent; border: none; border-image: none; }");
    });

    // 连接“设置”按钮
    connect(ui->tbtn_setting, &QToolButton::clicked, this, [this](bool checked) {
        if (checked) {
            ui->setting_frame->setStyleSheet("QFrame {border-image: url(:/image/selectedBG.png);}");
        }

         ui->assembly_frame->setStyleSheet("QFrame { background: transparent; border: none; border-image: none; }");
    });


}

// 修改槽函数参数类型为 Qt::CheckState
void UserOperationDialog::onCheckBoxToggled(Qt::CheckState state)
{
    QFrame *leftSidebar = ui->leftSidebar;
    QFrame *rightSidebar = ui->rightSidebar;

    // 计算动画的起始值，应该基于当前的 minimumWidth
    int currentLeftWidth = leftSidebar->minimumWidth();
    int currentRightWidth = rightSidebar->minimumWidth();

    if (state == Qt::Checked) {
        // 选中：展开
        QFont font = ui->tbtn_icon->font();
        font.setPointSize(10);
        ui->tbtn_icon->setFont(font);
        ui->tbtn_icon->setIconSize(QSize(25, 25));
        leftAnim->setStartValue(currentLeftWidth); // 从当前最小宽度开始
        leftAnim->setEndValue(160);
        rightAnim->setStartValue(currentRightWidth);
        rightAnim->setEndValue(160);
        isLeftSidebarExpanded = true; // 更新状态
        ui->tbtn_assembly->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextBesideIcon);
        ui->tbtn_setting->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextBesideIcon);
        ui->tbtn_icon->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextBesideIcon);
    } else {
        // 未选中：收缩
        QFont font = ui->tbtn_icon->font();
        font.setPointSize(14);
        ui->tbtn_icon->setFont(font);
        ui->tbtn_icon->setIconSize(QSize(40, 40));
        leftAnim->setStartValue(currentLeftWidth); // 从当前最小宽度开始
        leftAnim->setEndValue(60);
        rightAnim->setStartValue(currentRightWidth);
        rightAnim->setEndValue(60);
        isLeftSidebarExpanded = false; // 更新状态
        ui->tbtn_assembly->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonIconOnly);
        ui->tbtn_setting->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonIconOnly);
        ui->tbtn_icon->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonIconOnly);
    }

    // 启动动画
    leftAnim->start();
    rightAnim->start();
}

// resizeEvent 保持不变
void UserOperationDialog::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);

    QFrame *leftSidebar = ui->leftSidebar;
    QFrame *rightSidebar = ui->rightSidebar;

    // 根据当前状态计算宽度
    int leftWidth = isLeftSidebarExpanded ? 240 : 60;
    int rightWidth = isLeftSidebarExpanded ? 240 : 60;

    // 重新定位
    leftSidebar->setGeometry(0, 0, leftWidth, height());
    rightSidebar->setGeometry(width() - rightWidth, 0, rightWidth, height());

    // 通知布局系统无效化，让其重新计算（尽管 setGeometry 会覆盖布局结果）
    ui->horizontalLayout->invalidate();
    // 然后再次手动设置 geometry 以确保精确位置
    leftSidebar->setGeometry(0, 0, leftWidth, height());
    rightSidebar->setGeometry(width() - rightWidth, 0, rightWidth, height());
}
