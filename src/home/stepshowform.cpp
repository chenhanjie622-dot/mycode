#include "stepshowform.h"
#include "ui_stepshowform.h"
#include <QTimer>
#include <QMessageBox>
#include <QInputDialog>
#include <QPixmap>

StepShowForm::StepShowForm(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::StepShowForm)
{
    ui->setupUi(this);

    // 删除
    connect(ui->btn_delete, &QPushButton::clicked, this, [this]() {
        if (QMessageBox::question(this, "确认", "确定删除该步骤？") == QMessageBox::Yes)
            emit signalDelete(this);
    });

    // 编辑
    connect(ui->btn_edit, &QPushButton::clicked, this, [this]() {
        bool ok;
        QString text = QInputDialog::getText(this, "编辑步骤",
                                             "步骤内容：",
                                             QLineEdit::Normal,
                                             m_stepText, &ok);
        if (ok && !text.isEmpty()) {
            setStepValue(text);
        }
    });

    // 上移
    connect(ui->btn_up, &QPushButton::clicked, this, [this]() {
        emit signalMoveUp(this);
    });

    // 下移
    connect(ui->btn_down, &QPushButton::clicked, this, [this]() {
        emit signalMoveDown(this);
    });

    // 样
    ui->btn_delete->setStyleSheet("background:red;color:white;border-radius:6px;");

    // 增加货架信息
    connect(ui->btn_addrack, &QPushButton::clicked, this, [this]() {
        if (isRackDataValid(m_rackdata)) {
            const int rackCount = m_rackdata.count() / 2;
            for (int var = 0; var < rackCount; ++var) {
                const int index = var * 2;
                createAndAddRackForm(m_rackForm, ui->Layout_arcklsit, m_rackdata.at(index), m_rackdata.at(index + 1));
            }
            m_rackdata.clear(); // 【极度关键的修复】：加载完保存的数据后必须清空！否则下次手动点 + 号时，会把之前的一大串历史记录全部重复塞进界面！
        } else {
            createAndAddRackForm(m_rackForm, ui->Layout_arcklsit);
        }
    });
}

StepShowForm::~StepShowForm()
{
    delete ui;
}

void StepShowForm::setStepValue(QString value)
{
    m_stepText = value;
    ui->label_step_value->setText(value);
}

void StepShowForm::setImageValue(QString value)
{
    m_imagePath = value;

    QPixmap pixmap(value);
    if (!pixmap.isNull()) {
        ui->label_image_value->setPixmap(
            pixmap.scaled(450,260,Qt::KeepAspectRatio,Qt::SmoothTransformation));
    }
}

void StepShowForm::setRackDataValue(QString value)
{
    if (value.isEmpty()) {
        return;
    }

    // 【修复1】：提取纯步骤名（去掉 "设备名:" 前缀），例如把 "PCR安装:准备" 变成 "准备"
    QString currentStepName = m_stepText.split(":").last();

    for (const QString &var : value.split("^", Qt::SkipEmptyParts)) {
        QStringList rack = var.split("|", Qt::SkipEmptyParts);

        // 【修复2】：必须使用精确匹配 (==)，否则 .contains() 会导致 "步骤1" 错误加载 "步骤10" 的数据！
        if (rack.size() <= 1 || rack.first() != currentStepName) {
            continue;
        }

        // 处理有效数据
        rack.removeFirst();
        m_rackdata = rack;
        QMetaObject::invokeMethod(ui->btn_addrack, "click", Qt::QueuedConnection);
    }
}

QString StepShowForm::getStepText() const { return m_stepText; }
QString StepShowForm::getImagePath() const { return m_imagePath; }

QVector<QSharedPointer<RackCFGForm>> StepShowForm::getRackForm() const
{
    return m_rackForm;
}

bool StepShowForm::isChecked() const
{
    return ui->check_select->isChecked();
}

void StepShowForm::setChecked(bool checked)
{
    ui->check_select->setChecked(checked);
}

void StepShowForm::setVideoPath(QString path)
{
    m_videoPath = path;
}

QString StepShowForm::getVideoPath() const
{
    return m_videoPath;
}

void StepShowForm::setProjectorImagePath(QString path)
{
    m_projectorImagePath = path;
}

QString StepShowForm::getProjectorImagePath() const
{
    return m_projectorImagePath;
}

bool StepShowForm::isRackDataValid(const QStringList& rackData) {
    if (rackData.isEmpty()) {
        return false;
    }
    if (rackData.count() < 2 || rackData.count() % 2 != 0) {
        qDebug() << "数据有问题，rackData:" << rackData << "，数量:" << rackData.count();
        return false;
    }
    return true;
}

void StepShowForm::createAndAddRackForm(QList<QSharedPointer<RackCFGForm>>& rackForm, QHBoxLayout* layout, const QString& rackNumber, const QString& quantityGoods) {
    QSharedPointer<RackCFGForm> rack(new RackCFGForm(this));
    if (!rackNumber.isEmpty() && !quantityGoods.isEmpty()) {
        rack->setRackNumber(rackNumber);
        rack->setQuantityGoods(quantityGoods);
    }

    // ======== 【新增代码：处理零件配置的删除逻辑】 ========
    connect(rack.data(), &RackCFGForm::signalRemoveRack, this, [this, rack, layout](RackCFGForm* form) {
        // 1. 先从界面的布局中移除并隐藏它，让用户立刻看到它在 UI 上消失
        layout->removeWidget(form);
        form->hide();

        // 2. 延迟从底层数据列表中移除（极度关键的安全设计！）
        // 为什么用延时？因为程序此刻还在执行 btnDelete 的点击事件槽函数，
        // 如果立即 removeOne 会导致 QSharedPointer 瞬间释放内存，进而引发程序奔溃(Crash)。
        QTimer::singleShot(0, this, [this, rack]() {
            m_rackForm.removeOne(rack);
        });
    });
    // ====================================================

    rackForm.append(rack);
    // 插入到布局中，排在最后的“+”号按钮前面
    layout->insertWidget(layout->count() - 1, rack.data());
}

void StepShowForm::setSopText(QString text)
{
    m_sopText = text;
}

QString StepShowForm::getSopText() const
{
    return m_sopText;
}

void StepShowForm::setProjectorVideoPath(QString path)
{
    m_projectorVideoPath = path;
}

QString StepShowForm::getProjectorVideoPath() const
{
    return m_projectorVideoPath;
}