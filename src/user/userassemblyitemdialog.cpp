#include "userassemblyitemdialog.h"
#include "ui_userassemblyitemdialog.h"
#include "confirmdialog.h"
#include <QDateTime>
#include <QPainter>
#include <QPixmap>
#include <QMouseEvent>

UserAssemblyItemDialog::UserAssemblyItemDialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UserAssemblyItemDialog),
    m_selected(false)
{
    ui->setupUi(this);

    // ==========================================
    // 1. 【Item 自身】背景图由 paintEvent 绘制，无需 QSS
    // ==========================================
    // 不设置任何 QSS 背景相关属性，避免被全局 QSS 或父级样式覆盖

    // ==========================================
    // 2. 【批量修复】所有 Frame (frame, frame_2, ...)
    // ==========================================
    QList<QFrame*> frames = {ui->frame, ui->frame_2, ui->frame_3, ui->frame_4};

    for (QFrame* f : frames) {
        if (f) {
            // Frame 背景图也由 paintEvent 绘制
            // 安装事件过滤器，在 paintEvent 中绘制背景
            f->installEventFilter(this);
        }
    }

    // ==========================================
    // 3. 【批量修复】所有 Label (label, label_2, ...)
    // ==========================================
    // 定义需要修复的 Label 对象列表
    QList<QLabel*> labels = {
        ui->label, ui->label_2, ui->label_3, ui->label_4,
        ui->label_5, ui->label_6, ui->label_7, ui->label_8, ui->label_9
    };

    for (QLabel* l : labels) {
        if (l) {
            l->setAutoFillBackground(true); // 必须开启，否则透明无效

            // 区分样式：
            // 如果是 label_2 和 label_6 (青色字)，其他是白色字
            if (l == ui->label_2 || l == ui->label_6) {
                l->setStyleSheet("color: rgb(0, 185, 255); background-color: transparent; font: 16pt \"Microsoft YaHei UI\";");
            } else {
                l->setStyleSheet("color: white; background-color: transparent;");
            }
        }
    }
}

void UserAssemblyItemDialog::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    QPixmap bg(":/image/userassemblyitem.png");
    if (!bg.isNull()) {
        painter.drawPixmap(this->rect(), bg);
    }

    // 选中时绘制边框
    if (m_selected) {
        painter.setPen(QPen(QColor(0x00B9FF), 6));
        painter.setBrush(Qt::NoBrush);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.drawRoundedRect(this->rect().adjusted(2, 2, -2, -2), 6, 6);
    }
}

bool UserAssemblyItemDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::Paint) {
        QFrame* f = qobject_cast<QFrame*>(watched);
        if (f) {
            QPainter painter(f);
            QPixmap bg(":/image/userassemblyitemI.png");
            if (!bg.isNull()) {
                painter.drawPixmap(f->rect(), bg);
            }
            return true; // 事件已处理，不再传递
        }
    }
    return QWidget::eventFilter(watched, event);
}

void UserAssemblyItemDialog::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // 点击立即显示边框
        m_selected = true;
        update();

        // 弹出自定义确认弹窗
        QWidget *topWidget = this->window();
        bool confirmed = ConfirmDialog::confirm(tr("是否选择该装配任务"), topWidget);

        // 无论确认还是取消，都去掉边框恢复原状
        m_selected = false;
        update();

        if (confirmed) {
            emit clicked();
        }
    }
    QWidget::mousePressEvent(event);
}

UserAssemblyItemDialog::~UserAssemblyItemDialog()
{
    delete ui;
}

void UserAssemblyItemDialog::setTaskInfo(const QString& deviceName, int stepCount, const QString& firstImagePath, const QString& taskDesc)
{
    // 1. 设置设备名称 (UI中的 label_8)
    ui->label_8->setText(deviceName);

    // 2. 设置模块/步骤数量 (UI中的 label_2)
    ui->label_2->setText(QString::number(stepCount));

    // 3. 动态计算个预计耗时 (假设每个步骤需要 5 分钟)，设置到 label_6
    ui->label_6->setText(QString::number(stepCount * 5));

    // 【新增】：设置任务介绍文本（为了防止没写txt，给个默认提示）
    if (taskDesc.trimmed().isEmpty()) {
        ui->label_7->setText("该任务暂无详细介绍。");
    } else {
        ui->label_7->setText(taskDesc);
    }

    // 4. 设置封面图片 (UI中的 label_10)
    if (!firstImagePath.isEmpty()) {
        // 注意：Qt的样式表识别本地绝对路径时，需要将反斜杠替换为正斜杠
        QString fixedPath = firstImagePath;
        fixedPath.replace("\\", "/");
        QString style = QString("border-image: url('%1');").arg(fixedPath);
        ui->label_10->setStyleSheet(style);
    }
}

void UserAssemblyItemDialog::setSelected(bool selected)
{
    if (m_selected != selected) {
        m_selected = selected;
        update();
    }
}

bool UserAssemblyItemDialog::isSelected() const
{
    return m_selected;
}
