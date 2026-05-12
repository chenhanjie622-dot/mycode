#include "rackcfgform.h"
#include "ui_rackcfgform.h"
#include <QIntValidator> // 【新增】引入整数验证器，实现工业防呆
#include <QPushButton>
#include "database/shelfdatabase.h"
#include <QLabel>

RackCFGForm::RackCFGForm(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::RackCFGForm)
{
    ui->setupUi(this);

    // 1. 初始化下拉框选项
    ui->cbb_side->addItems({"A", "B"});
    updateRowOptions();

    // 2. 【核心防呆设计】：限制数量输入框只能输入 1 到 9999 的正整数！
    // 绝对不能让工人输入字母、符号或负数，否则组装界面下发 Modbus 指令时会导致货架系统崩溃。
    ui->lineedit_count->setValidator(new QIntValidator(1, 9999, this));
    ui->lineedit_count->setText("1"); // 默认最少拿 1 个
    // --- 动态添加名称联动标签 ---
    m_labelPartName = new QLabel("[空]", this);
    m_labelPartName->setStyleSheet("color: #999999; font-size: 13px; padding-left: 5px;");
    // 把它插入到水平布局中（放在数量的右边，删除按钮的左边）
    ui->horizontalLayout_5->addWidget(m_labelPartName);

    // 当面、排、列任何一个发生改变时，触发查询更新
    connect(ui->cbb_side, &QComboBox::currentTextChanged, this, &RackCFGForm::updatePartName);
    connect(ui->cbb_row, &QComboBox::currentTextChanged, this, &RackCFGForm::updatePartName);
    connect(ui->cbb_column, &QComboBox::currentTextChanged, this, &RackCFGForm::updatePartName);
    // ======== 【新增代码：动态创建删除按钮】 ========
    QPushButton *btnDelete = new QPushButton("✖", this);
    btnDelete->setFixedSize(28, 28);
    // 设置美观的红色警示样式
    btnDelete->setStyleSheet("background-color: #ff4d4f; color: white; border-radius: 4px; font-weight: bold; font-size: 14px;");
    btnDelete->setToolTip("删除此零件配置");

    // 将删除按钮追加到当前水平布局的最右侧（紧跟在数量后面）
    ui->horizontalLayout_5->addWidget(btnDelete);

    // 绑定点击事件，发出在 .h 中预留好的 signalRemoveRack 信号
    connect(btnDelete, &QPushButton::clicked, this, [this]() {
        emit signalRemoveRack(this);
    });
    // ==============================================
}

RackCFGForm::~RackCFGForm()
{
    delete ui;
}

// ---------------- 逻辑处理 ----------------

void RackCFGForm::updateRowOptions()
{
    ui->cbb_row->blockSignals(true);
    ui->cbb_row->clear();

    if (ui->cbb_side->currentText() == "A") {
        ui->cbb_row->addItems({"1", "2", "3", "4"});
    } else {
        ui->cbb_row->addItems({"1", "2"});
    }
    ui->cbb_row->blockSignals(false);
    updateColumnOptions();
}

void RackCFGForm::updateColumnOptions()
{
    ui->cbb_column->blockSignals(true);
    ui->cbb_column->clear();

    QString side = ui->cbb_side->currentText();
    int row = ui->cbb_row->currentText().toInt();
    int maxCol = 10;

    // 硬件物理特性限制：A面第4排只有6列，其余皆为10列
    if (side == "A" && row == 4) {
        maxCol = 6;
    }

    for (int i = 1; i <= maxCol; ++i) {
        ui->cbb_column->addItem(QString::number(i));
    }
    ui->cbb_column->blockSignals(false);
}

// ---------------- 槽函数 ----------------

void RackCFGForm::on_cbb_side_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    updateRowOptions(); // 切换面会联动刷新行
}

void RackCFGForm::on_cbb_row_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    updateColumnOptions(); // 切换行会联动刷新列
}

// ---------------- 外部接口 (Getter/Setter) ----------------

void RackCFGForm::setRackNumber(QString value)
{
    // 解析数据库传来的 "Side-Row-Col" 例如 "A-4-2"
    QStringList parts = value.split("-");
    if (parts.size() < 3) return;

    ui->cbb_side->setCurrentText(parts[0]);
    updateRowOptions(); // 强制更新行下拉框
    ui->cbb_row->setCurrentText(parts[1]);
    updateColumnOptions(); // 强制更新列下拉框
    ui->cbb_column->setCurrentText(parts[2]);
    ui->cbb_column->setCurrentText(parts[2]);
    updatePartName(); // <--- 加上这一行，保证回显历史数据时也能显示名字
}

void RackCFGForm::setQuantityGoods(QString value)
{
    // 【防呆设计】：从数据库读取时如果发生异常，强制转为正整数
    int val = value.toInt();
    if (val <= 0) val = 1;
    ui->lineedit_count->setText(QString::number(val));
}

QString RackCFGForm::getRackNumber() const
{
    // 拼接出 A-3-2 这种标准格式，供 AssemblyDialog 提取计算 0x31 物理地址
    return QString("%1-%2-%3").arg(ui->cbb_side->currentText(),
                                   ui->cbb_row->currentText(),
                                   ui->cbb_column->currentText());
}

QString RackCFGForm::getRackSide() const { return ui->cbb_side->currentText(); }
QString RackCFGForm::getRackRow() const { return ui->cbb_row->currentText(); }
QString RackCFGForm::getRackColumn() const { return ui->cbb_column->currentText(); }

QString RackCFGForm::getQuantityGoods() const
{
    // 【防呆设计】：如果工人手滑把输入框全删空了，默认返回 1，防止发送 0 导致货架不亮灯
    QString txt = ui->lineedit_count->text();
    return txt.isEmpty() ? "1" : txt;
}

void RackCFGForm::updatePartName()
{
    // 获取当前拼好的编号，去本地 SQLite 数据库里查
    QString rackNum = getRackNumber();
    ShelfData data = ShelfDatabase::getInstance().getItem(rackNum);

    if(data.name.isEmpty() || data.name == "空") {
        m_labelPartName->setText("[空]");
        m_labelPartName->setStyleSheet("color: #999999;");
    } else {
        m_labelPartName->setText(QString("[%1]").arg(data.name));
        m_labelPartName->setStyleSheet("color: #0055ff; font-weight: bold;"); // 有物料时显示醒目蓝色
    }
}
