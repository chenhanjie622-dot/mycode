#include "shelfitemdialog.h"
#include "ui_shelfitemdialog.h"
#include "shelfdatabase.h"
#include <QMessageBox>
#include <QDebug>

ShelfItemDialog::ShelfItemDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::ShelfItemDialog), m_socket(nullptr), m_deviceAddr(0x31)
{
    ui->setupUi(this);
}

ShelfItemDialog::~ShelfItemDialog() { delete ui; }

void ShelfItemDialog::SetShelfIndex(QString value) { ui->label_shelfindex_value->setText(value); }
void ShelfItemDialog::SetShelfName(QString value)  { ui->lineEditName->setText(value); }

void ShelfItemDialog::setHardwareInfo(QTcpSocket* socket, quint8 addr) {
    m_socket = socket;
    m_deviceAddr = addr;
}

void ShelfItemDialog::updateRealTimeQuantity(int qty) {
    m_currentRealQty = qty;
    ui->labelRealTimeQty->setText(QString("现存总数: %1 个").arg(qty));
}

quint16 ShelfItemDialog::calculateCRC16(const QByteArray &data) {
    quint16 crc = 0xFFFF;
    for (int pos = 0; pos < data.length(); pos++) {
        crc ^= (quint8)data.at(pos);
        for (int i = 8; i != 0; i--) {
            if ((crc & 0x0001) != 0) { crc >>= 1; crc ^= 0xA001; } else { crc >>= 1; }
        }
    }
    return crc;
}

void ShelfItemDialog::sendModbusCommand(quint8 funcCode, quint16 regAddress, const QByteArray &dataParams) {
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) {
        QMessageBox::warning(this, "通讯错误", "未连接到智能货架主控！");
        return;
    }

    QByteArray payload = dataParams;
    if (payload.size() % 2 != 0) payload.append(char(0x00));

    QByteArray frame;
    frame.append(m_deviceAddr);
    frame.append(funcCode);
    frame.append(char((regAddress >> 8) & 0xFF));
    frame.append(char(regAddress & 0xFF));

    quint16 regCount = payload.size() / 2;
    frame.append(char((regCount >> 8) & 0xFF));
    frame.append(char(regCount & 0xFF));
    frame.append(char(payload.size() & 0xFF));
    frame.append(payload);

    quint16 crc = calculateCRC16(frame);
    frame.append(char(crc & 0xFF));
    frame.append(char((crc >> 8) & 0xFF));

    m_socket->write(frame);
    m_socket->flush();
}

void ShelfItemDialog::sendStringCommand(quint16 reg, const QString &text, int maxBytes) {
    if (text.isEmpty()) return;
    QByteArray gbkData = text.toLocal8Bit();
    if (gbkData.size() > maxBytes) { QMessageBox::warning(this, "提示", "输入的字符过长！"); return; }
    sendModbusCommand(0x10, reg, gbkData);
}

void ShelfItemDialog::sendInt32Command(quint16 reg, int value) {
    QByteArray data;
    data.append(char((value >> 24) & 0xFF)); data.append(char((value >> 16) & 0xFF));
    data.append(char((value >> 8) & 0xFF));  data.append(char(value & 0xFF));
    sendModbusCommand(0x10, reg, data);
}

void ShelfItemDialog::on_btnUpdateName_clicked() {
    updateCurrentDeviceAddr();
    sendStringCommand(0x0100, ui->lineEditName->text(), 20);
}
void ShelfItemDialog::on_btnUpdateSpec_clicked() { sendStringCommand(0x0101, ui->lineEditSpec->text(), 20); }
void ShelfItemDialog::on_btnUpdateCode_clicked() { sendStringCommand(0x0102, ui->lineEditCode->text(), 91); }

void ShelfItemDialog::on_btnUpdateQuantity_clicked() {
    updateCurrentDeviceAddr();
    int qty = ui->spinBoxQuantity->value();
    if (qty == 0) sendInt32Command(0x0105, 0);
    else sendInt32Command(0x0106, qty);
}

void ShelfItemDialog::on_btnUpdatePlanQty_clicked() {
    updateCurrentDeviceAddr();
    sendInt32Command(0x010B, ui->spinBoxPlanQty->value());
}
void ShelfItemDialog::on_btnClearOperation_clicked() {
    QByteArray data(4, 0x00);
    sendModbusCommand(0x10, 0x010C, data);
}

void ShelfItemDialog::on_btn_save_db_clicked()
{
    QString id = ui->label_shelfindex_value->text();
    QString name = ui->lineEditName->text();
    QString spec = ui->lineEditSpec->text();
    QString code = ui->lineEditCode->text();

    on_btnUpdateName_clicked();
    on_btnUpdateSpec_clicked();
    on_btnUpdateCode_clicked();

    if(ShelfDatabase::getInstance().updateItem(id, name, spec, code, m_currentRealQty)) {
        QMessageBox::information(this, "同步成功", "信息已成功下发至货架并同步至本地！");
        this->accept();
    } else {
        QMessageBox::critical(this, "错误", "本地数据库保存失败！");
    }
}

void ShelfItemDialog::on_pushButton_clicked() { this->reject(); }

// 【修改】仅计算纯粹的单板物理地址，不关心TCP所属
void ShelfItemDialog::updateCurrentDeviceAddr() {
    QString cellId = ui->label_shelfindex_value->text();
    QStringList parts = cellId.split("-");
    if(parts.size() != 3) return;

    QString side = parts[0];
    int row = parts[1].toInt();
    int col = parts[2].toInt();

    int localCol = col;
    if (row == 4) {
        if (col > 3) localCol = col - 3;
    } else {
        if (col > 5) localCol = col - 5;
    }

    quint8 addr = 0x31;
    if (side == "A") {
        if (row == 1) addr = 0x43 + (localCol - 1);
        else if (row == 2) addr = 0x39 + (localCol - 1);
        else if (row == 3) addr = 0x34 + (localCol - 1);
        else if (row == 4) addr = 0x31 + (localCol - 1);
    } else if (side == "B") {
        if (row == 1) addr = 0x4C - (localCol - 1);
        else if (row == 2) addr = 0x42 - (localCol - 1);
    }

    m_deviceAddr = addr;
}

void ShelfItemDialog::SetShelfSpec(QString value) { ui->lineEditSpec->setText(value); }
void ShelfItemDialog::SetShelfCode(QString value) { ui->lineEditCode->setText(value); }