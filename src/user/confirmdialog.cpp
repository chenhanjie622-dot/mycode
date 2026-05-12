#include "confirmdialog.h"

#include <QPainter>
#include <QPainterPath>
#include <QGraphicsOpacityEffect>

ConfirmDialog::ConfirmDialog(const QString &title, QWidget *parent)
    : QDialog(parent)
{
    // 无边框，半透明背景用于遮罩效果
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    // 不设固定大小，由 confirm() 中根据父窗口设置

    // 整体布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 内容容器（带边框的圆角面板）
    QWidget *contentWidget = new QWidget(this);
    contentWidget->setObjectName("contentWidget");
    contentWidget->setFixedSize(264, 130);
    contentWidget->setStyleSheet(
        "QWidget#contentWidget {"
        "   background-color: #3D6387;"
        "   border: none;"
        "   border-radius: 8px;"
        "}"
    );

    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(8, 8, 8, 8);
    contentLayout->setSpacing(8);

    // === 标题区域 ===
    QWidget *titleWidget = new QWidget(contentWidget);
    titleWidget->setObjectName("titleWidget");
    titleWidget->setFixedWidth(248); // 120 + 8间距 + 120 = 248，与按钮总宽度一致
    titleWidget->setStyleSheet(
        "QWidget#titleWidget {"
        "   background-color: #042656;"
        "   border: none;"
        "   border-radius: 4px;"
        "}"
    );
    QVBoxLayout *titleLayout = new QVBoxLayout(titleWidget);
    titleLayout->setContentsMargins(8, 8, 8, 8);

    m_titleLabel = new QLabel(title, titleWidget);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet(
        "color: #FFFFFF;"
        "font: 16pt \"Microsoft YaHei UI\";"
        "background-color: transparent;"
        "border: none;"
    );
    titleLayout->addWidget(m_titleLabel);

    // 标题居中
    QHBoxLayout *titleOuterLayout = new QHBoxLayout();
    titleOuterLayout->addStretch();
    titleOuterLayout->addWidget(titleWidget);
    titleOuterLayout->addStretch();
    contentLayout->addLayout(titleOuterLayout);

    // === 按钮区域 ===
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(8);

    // 取消按钮
    m_cancelBtn = new QPushButton(tr("取消"), contentWidget);
    m_cancelBtn->setObjectName("cancelBtn");
    m_cancelBtn->setFixedSize(120, 40);
    m_cancelBtn->setStyleSheet(
        "QPushButton#cancelBtn {"
        "   color: #FFFFFF;"
        "   background-color: #042656;"
        "   border: none;"
        "   border-radius: 4px;"
        "   font: 12pt \"Microsoft YaHei UI\";"
        "}"
        "QPushButton#cancelBtn:hover {"
        "   background-color: #0a3a7a;"
        "}"
        "QPushButton#cancelBtn:pressed {"
        "   background-color: #061d45;"
        "}"
    );

    // 确定按钮
    m_confirmBtn = new QPushButton(tr("确定"), contentWidget);
    m_confirmBtn->setObjectName("confirmBtn");
    m_confirmBtn->setFixedSize(120, 40);
    m_confirmBtn->setStyleSheet(
        "QPushButton#confirmBtn {"
        "   color: #0078FF;"
        "   background-color: #042656;"
        "   border: none;"
        "   border-radius: 4px;"
        "   font: 12pt \"Microsoft YaHei UI\";"
        "}"
        "QPushButton#confirmBtn:hover {"
        "   background-color: #0a3a7a;"
        "}"
        "QPushButton#confirmBtn:pressed {"
        "   background-color: #061d45;"
        "}"
    );

    btnLayout->addStretch();
    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addWidget(m_confirmBtn);
    btnLayout->addStretch();
    contentLayout->addLayout(btnLayout);

    mainLayout->addStretch();
    mainLayout->addWidget(contentWidget, 0, Qt::AlignCenter);
    mainLayout->addStretch();

    // 信号槽
    connect(m_cancelBtn, &QPushButton::clicked, this, [this]() {
        reject();
    });
    connect(m_confirmBtn, &QPushButton::clicked, this, [this]() {
        accept();
    });
}

ConfirmDialog::~ConfirmDialog()
{
}

bool ConfirmDialog::confirm(const QString &title, QWidget *parent)
{
    ConfirmDialog dlg(title, parent);
    // 遮罩层覆盖整个父窗口
    if (parent) {
        dlg.resize(parent->size());
        dlg.move(parent->mapToGlobal(QPoint(0, 0)));
    }
    return dlg.exec() == QDialog::Accepted;
}

void ConfirmDialog::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制半透明遮罩层（覆盖整个对话框区域）
    QPainterPath path;
    path.addRect(rect());
    painter.fillPath(path, QColor(0, 0, 0, 120));
}
