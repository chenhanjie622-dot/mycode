#ifndef SHELFITEMDIALOG_H
#define SHELFITEMDIALOG_H

#include <QDialog>
#include <QTcpSocket>

namespace Ui { class ShelfItemDialog; }

class ShelfItemDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ShelfItemDialog(QWidget *parent = nullptr);
    ~ShelfItemDialog();

    void SetShelfIndex(QString value);
    void SetShelfName(QString value); // 用于从本地DB初始化文本框

    // ---注入底层硬件信息 ---
    void setHardwareInfo(QTcpSocket* socket, quint8 addr);
    // 供主界面调用的实时数量更新接口
    void updateRealTimeQuantity(int qty);
    void SetShelfSpec(QString value);
    void SetShelfCode(QString value);

private slots:
    // 硬件操作按钮槽函数
    void on_btnUpdateName_clicked();
    void on_btnUpdateSpec_clicked();
    void on_btnUpdateCode_clicked();
    void on_btnUpdateQuantity_clicked();
    void on_btnUpdatePlanQty_clicked();
    void on_btnClearOperation_clicked();

    // 弹窗基础按钮
    void on_btn_save_db_clicked();
    void on_pushButton_clicked();

private:
    Ui::ShelfItemDialog *ui;

    // 硬件通讯变量
    QTcpSocket* m_socket;
    quint8 m_deviceAddr;
    // --- 记录从硬件传来的真实数量 ---
    int m_currentRealQty = 0;
    // Modbus 指令引擎
    quint16 calculateCRC16(const QByteArray &data);
    void sendModbusCommand(quint8 funcCode, quint16 regAddress, const QByteArray &data);
    void sendStringCommand(quint16 reg, const QString &text, int maxBytes);
    void sendInt32Command(quint16 reg, int value);
    void updateCurrentDeviceAddr(); // 新增：动态计算当前货位的物理地址
};

#endif // SHELFITEMDIALOG_H
