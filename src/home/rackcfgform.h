#ifndef RACKCFGFORM_H
#define RACKCFGFORM_H

#include <QWidget>
#include <QLabel>

namespace Ui {
class RackCFGForm;
}

class RackCFGForm : public QWidget
{
    Q_OBJECT

public:
    explicit RackCFGForm(QWidget *parent = nullptr);
    ~RackCFGForm();

    // 核心公共接口
    void setRackNumber(QString value);      // 外部传入格式如 "A-1-5"
    void setQuantityGoods(QString value);   // 外部传入数量

    QString getRackNumber() const;          // 供组装界面获取格式如 "A-1-5"
    QString getRackSide() const;
    QString getRackRow() const;
    QString getRackColumn() const;
    QString getQuantityGoods() const;       // 供组装界面获取数量字符串

signals:
    // 【新增】预留信号：如果你在 UI 里加了“减号/删除”按钮，
    // 可以发出这个信号，通知主界面把这一条零件配置删掉
    void signalRemoveRack(RackCFGForm* form);

private slots:
    void on_cbb_side_currentIndexChanged(int index);   // 切换 A/B 面联动
    void on_cbb_row_currentIndexChanged(int index);    // 切换行联动
    void updatePartName(); // 监听下拉框改变，实时去查数据库

private:
    Ui::RackCFGForm *ui;
    void updateRowOptions();    // 根据面更新行
    void updateColumnOptions(); // 根据面和行更新列
    QLabel *m_labelPartName; // 动态显示的标签
};

#endif // RACKCFGFORM_H
