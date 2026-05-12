#include "shelfdisplaydialog.h"
#include "ui_shelfdisplaydialog.h"
#include "shelfitemdialog.h"
#include "shelfdatabase.h"
#include <QPushButton>
#include <QLabel>
#include <QGraphicsDropShadowEffect>
#include <QMessageBox>
#include <QFile>
#include <QFileDialog>

ShelfDisplayDialog::ShelfDisplayDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ShelfDisplayDialog)
    , m_isBackSide(false)
{
    ui->setupUi(this);
    this->setWindowTitle("智控实验室 - 智能货架系统");
    this->setStyleSheet("QDialog { background-color: #f4f7f9; }");

    ui->Layout_Shelf->setSpacing(15);
    ui->Layout_Shelf->setContentsMargins(20, 20, 20, 20);

    ShelfDatabase::getInstance().initDB();
    InitUI();

    m_importTimer = new QTimer(this);
    connect(m_importTimer, &QTimer::timeout, this, &ShelfDisplayDialog::onImportTimerTimeout);

    // --- 启动底层硬件通讯总枢纽 ---
    tcpServerLeft = new QTcpServer(this);
    tcpServerRight = new QTcpServer(this);
    connect(tcpServerLeft, &QTcpServer::newConnection, this, &ShelfDisplayDialog::onNewConnectionLeft);
    connect(tcpServerRight, &QTcpServer::newConnection, this, &ShelfDisplayDialog::onNewConnectionRight);

    pollTimer = new QTimer(this);
    connect(pollTimer, &QTimer::timeout, this, &ShelfDisplayDialog::onPollTimeout);
}

ShelfDisplayDialog::~ShelfDisplayDialog() { delete ui; }

QString getShelfStyle(int count, bool isBig) {
    QString mainColor = (count > 0) ? "#3498db" : "#ffffff";
    QString borderColor = (count > 0) ? "#2980b9" : "#d1d5db";
    int height = isBig ? 120 : 80;
    return QString("QPushButton {"
                   "  background-color: %1; color: %2; border: 1px solid %3; border-radius: 4px; font-weight: bold; min-height: %4px;"
                   "  border-bottom: 4px solid %3; }"
                   "QPushButton:hover { background-color: %5; }"
                   "QPushButton:pressed { border-bottom: 1px solid %3; margin-top: 3px; }").arg(mainColor).arg((count > 0) ? "white" : "#4b5563").arg(borderColor).arg(height).arg((count > 0) ? "#217dbb" : "#f9fafb");
}

void ShelfDisplayDialog::InitUI()
{
    clearLayout();
    int totalSpan = 31;

    for (int col = 0; col < totalSpan; ++col) {
        if (col == 15) {
            ui->Layout_Shelf->setColumnStretch(col, 0);
            ui->Layout_Shelf->setColumnMinimumWidth(col, 40);
        } else {
            ui->Layout_Shelf->setColumnStretch(col, 1);
        }
    }
    for (int r = 0; r < 4; ++r) ui->Layout_Shelf->setRowStretch(r, 1);

    QFrame *divider = new QFrame(this);
    divider->setFrameShape(QFrame::VLine);
    divider->setFrameShadow(QFrame::Plain);
    divider->setStyleSheet("color: #9ca3af; border-left: 4px solid #9ca3af; border-radius: 2px;");
    ui->Layout_Shelf->addWidget(divider, 0, 15, 4, 1, Qt::AlignCenter);

    for(int row = 0; row < 4; row++) {
        bool isLastRow = (row == 3);
        int cellCount = isLastRow ? 6 : 10;
        int span = 15 / (cellCount / 2);

        if(m_isBackSide && row >= 2) {
            int minHeight = isLastRow ? 120 : 80;
            QString blockStyle = QString("background-color: #9ca3af; color: #374151; border-radius: 4px; font-weight: bold; border: 2px dashed #6b7280; min-height: %1px;").arg(minHeight);

            QLabel *blockLeft = new QLabel("左侧区域封闭\nCLOSED", this);
            blockLeft->setAlignment(Qt::AlignCenter); blockLeft->setStyleSheet(blockStyle);
            blockLeft->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            ui->Layout_Shelf->addWidget(blockLeft, row, 0, 1, 15);

            QLabel *blockRight = new QLabel("右侧区域封闭\nCLOSED", this);
            blockRight->setAlignment(Qt::AlignCenter); blockRight->setStyleSheet(blockStyle);
            blockRight->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            ui->Layout_Shelf->addWidget(blockRight, row, 16, 1, 15);
            continue;
        }

        for(int i = 0; i < cellCount; i++) {
            QString side = m_isBackSide ? "B" : "A";
            int displayIndex = i + 1;
            QString cellId = QString("%1-%2-%3").arg(side).arg(row + 1).arg(displayIndex);

            ShelfData data = ShelfDatabase::getInstance().getItem(cellId);
            QPushButton *btn = new QPushButton(cellId, this);
            btn->setObjectName("btn_" + cellId);
            btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            btn->setStyleSheet(getShelfStyle(data.count, isLastRow));

            QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
            shadow->setBlurRadius(10); shadow->setOffset(0, 3); shadow->setColor(QColor(0, 0, 0, 40));
            btn->setGraphicsEffect(shadow);
            connect(btn, &QPushButton::clicked, [this, cellId](){ onShelfClicked(cellId); });

            int startCol = 0;
            if (i < cellCount / 2) startCol = i * span;
            else startCol = 16 + (i - cellCount / 2) * span;
            ui->Layout_Shelf->addWidget(btn, row, startCol, 1, span);
        }
    }
}

void ShelfDisplayDialog::clearLayout() {
    while (QLayoutItem *child = ui->Layout_Shelf->takeAt(0)) {
        if (QWidget *w = child->widget()) { w->setObjectName(""); w->hide(); w->deleteLater(); }
        delete child;
    }
}

// ================== 公共混合地址转化器 ==================
int getCombinedAddress(QString cellId) {
    QStringList parts = cellId.split("-");
    if(parts.size() != 3) return 0;
    QString side = parts[0]; int row = parts[1].toInt(); int col = parts[2].toInt();

    bool isRight = false; int localCol = col;
    if (row == 4) { if (col > 3) { isRight = true; localCol = col - 3; } }
    else { if (col > 5) { isRight = true; localCol = col - 5; } }

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
    return isRight ? (addr + 256) : addr;
}

QString ShelfDisplayDialog::getCellIdByAddress(int combinedAddr) {
    bool isRight = combinedAddr >= 256; quint8 addr = combinedAddr % 256;
    int colOffset = isRight ? 5 : 0; int colOffsetRow4 = isRight ? 3 : 0;

    if (addr >= 0x43 && addr <= 0x47) return QString("A-1-%1").arg(addr - 0x43 + 1 + colOffset);
    if (addr >= 0x39 && addr <= 0x3D) return QString("A-2-%1").arg(addr - 0x39 + 1 + colOffset);
    if (addr >= 0x34 && addr <= 0x38) return QString("A-3-%1").arg(addr - 0x34 + 1 + colOffset);
    if (addr >= 0x31 && addr <= 0x33) return QString("A-4-%1").arg(addr - 0x31 + 1 + colOffsetRow4);
    if (addr <= 0x4C && addr >= 0x48) return QString("B-1-%1").arg(0x4C - addr + 1 + colOffset);
    if (addr <= 0x42 && addr >= 0x3E) return QString("B-2-%1").arg(0x42 - addr + 1 + colOffset);
    return "";
}

void ShelfDisplayDialog::onShelfClicked(QString id) {
    ShelfData data = ShelfDatabase::getInstance().getItem(id);
    ShelfItemDialog dlg(this);
    dlg.SetShelfIndex(data.id); dlg.SetShelfName(data.name); dlg.SetShelfSpec(data.spec); dlg.SetShelfCode(data.code);

    int combinedAddr = getCombinedAddress(data.id);
    bool isRight = combinedAddr >= 256;
    QTcpSocket *sock = isRight ? shelfSocketRight : shelfSocketLeft;
    quint8 hwAddr = combinedAddr % 256;

    dlg.setHardwareInfo(sock, hwAddr);
    this->m_currentPollingAddress = combinedAddr;

    connect(this, &ShelfDisplayDialog::realTimeQuantityReceived, &dlg, [&dlg, combinedAddr](int addr, int qty){
        if (addr == combinedAddr) dlg.updateRealTimeQuantity(qty);
    });

    if (dlg.exec() == QDialog::Accepted) InitUI();
    this->m_currentPollingAddress = 0;
}

void ShelfDisplayDialog::on_btn_shelfdlg_refresh_clicked() {
    m_isBackSide = !m_isBackSide;
    ui->btn_shelfdlg_refresh->setText(m_isBackSide ? "查看正面 (A)" : "查看背面 (B)");
    InitUI();
}

void ShelfDisplayDialog::on_btn_shelfdlg_configure_clicked() {
    ui->btn_shelfdlg_configure->setEnabled(false);
    ui->btn_shelfdlg_configure->setText("正在双重清零与扫描...");
    int delay = 0; int interval = 30;

    // 左侧和右侧共享同一套有效的硬件地址 (5个A行，5个B行)
    QList<quint8> validAddrs = {
        0x31, 0x32, 0x33,
        0x34, 0x35, 0x36, 0x37, 0x38,
        0x39, 0x3A, 0x3B, 0x3C, 0x3D,
        0x3E, 0x3F, 0x40, 0x41, 0x42,
        0x43, 0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4A, 0x4B, 0x4C
    };

    for (quint8 addr : validAddrs) {
        // 第一重清理
        QTimer::singleShot(delay, this, [this, addr]() {
            QByteArray frame; frame.append(addr); frame.append(char(0x10)); frame.append(char(0x01)); frame.append(char(0x0C));
            frame.append(char(0x00)); frame.append(char(0x02)); frame.append(char(0x04));
            frame.append(char(0x00)); frame.append(char(0x00)); frame.append(char(0x00)); frame.append(char(0x00));
            quint16 crc = calculateCRC16(frame); frame.append(char(crc & 0xFF)); frame.append(char((crc >> 8) & 0xFF));
            if (shelfSocketLeft && shelfSocketLeft->state() == QAbstractSocket::ConnectedState) { shelfSocketLeft->write(frame); shelfSocketLeft->flush(); }
            if (shelfSocketRight && shelfSocketRight->state() == QAbstractSocket::ConnectedState) { shelfSocketRight->write(frame); shelfSocketRight->flush(); }
        });
        delay += interval;

        // 第二重清理
        QTimer::singleShot(delay, this, [this, addr]() {
            QByteArray frame; frame.append(addr); frame.append(char(0x10)); frame.append(char(0x01)); frame.append(char(0x0B));
            frame.append(char(0x00)); frame.append(char(0x02)); frame.append(char(0x04));
            frame.append(char(0x00)); frame.append(char(0x00)); frame.append(char(0x00)); frame.append(char(0x00));
            quint16 crc = calculateCRC16(frame); frame.append(char(crc & 0xFF)); frame.append(char((crc >> 8) & 0xFF));
            if (shelfSocketLeft && shelfSocketLeft->state() == QAbstractSocket::ConnectedState) { shelfSocketLeft->write(frame); shelfSocketLeft->flush(); }
            if (shelfSocketRight && shelfSocketRight->state() == QAbstractSocket::ConnectedState) { shelfSocketRight->write(frame); shelfSocketRight->flush(); }
        });
        delay += interval;
    }

    QTimer::singleShot(delay + 100, this, [this, validAddrs]() {
        m_scanQueue.clear();
        for (quint8 addr : validAddrs) {
            m_scanQueue.append(addr);       // 添加左侧扫描
            m_scanQueue.append(addr + 256); // 添加右侧扫描
        }
        pollTimer->setInterval(100);
        if (!pollTimer->isActive()) pollTimer->start();
    });
}

void ShelfDisplayDialog::on_btn_shelfdlg_back_clicked() { emit signalBackHome(); }

// ================== 网络通讯与解析 ==================
void ShelfDisplayDialog::onNewConnectionLeft() {
    shelfSocketLeft = tcpServerLeft->nextPendingConnection();
    qDebug() << "【左货架总览】智能货架(1080)已接入！";
    connect(shelfSocketLeft, &QTcpSocket::disconnected, this, &ShelfDisplayDialog::onClientDisconnectedLeft);
    connect(shelfSocketLeft, &QTcpSocket::readyRead, this, &ShelfDisplayDialog::onReadyReadLeft);
    pollTimer->start(500);
    on_btn_shelfdlg_configure_clicked();
}

void ShelfDisplayDialog::onNewConnectionRight() {
    shelfSocketRight = tcpServerRight->nextPendingConnection();
    qDebug() << "【右货架总览】智能货架(1081)已接入！";
    connect(shelfSocketRight, &QTcpSocket::disconnected, this, &ShelfDisplayDialog::onClientDisconnectedRight);
    connect(shelfSocketRight, &QTcpSocket::readyRead, this, &ShelfDisplayDialog::onReadyReadRight);
    pollTimer->start(500);
    on_btn_shelfdlg_configure_clicked();
}

void ShelfDisplayDialog::onClientDisconnectedLeft() {
    qDebug() << "【左货架总览】意外断开！";
    m_bufferLeft.clear();
    if (shelfSocketLeft) { shelfSocketLeft->deleteLater(); shelfSocketLeft = nullptr; }
    if (!shelfSocketRight) if (pollTimer) pollTimer->stop();
}

void ShelfDisplayDialog::onClientDisconnectedRight() {
    qDebug() << "【右货架总览】意外断开！";
    m_bufferRight.clear();
    if (shelfSocketRight) { shelfSocketRight->deleteLater(); shelfSocketRight = nullptr; }
    if (!shelfSocketLeft) if (pollTimer) pollTimer->stop();
}

void ShelfDisplayDialog::onPollTimeout() {
    int targetCombinedAddr = 0;
    if (!m_scanQueue.isEmpty()) {
        targetCombinedAddr = m_scanQueue.takeFirst();
        if (m_scanQueue.isEmpty()) {
            ui->btn_shelfdlg_configure->setEnabled(true); ui->btn_shelfdlg_configure->setText("配置");
            pollTimer->setInterval(500);
        }
    } else if (m_currentPollingAddress != 0) {
        targetCombinedAddr = m_currentPollingAddress;
    } else return;

    bool isRight = targetCombinedAddr >= 256;
    QTcpSocket *sock = isRight ? shelfSocketRight : shelfSocketLeft;
    quint8 targetAddr = targetCombinedAddr % 256;

    if (!sock || sock->state() != QAbstractSocket::ConnectedState) return;

    QByteArray frame;
    frame.append(targetAddr); frame.append(0x03); frame.append(char(0x01)); frame.append(char(0x0F));
    frame.append(char(0x00)); frame.append(char(0x02));
    quint16 crc = calculateCRC16(frame); frame.append(crc & 0xFF); frame.append((crc >> 8) & 0xFF);
    sock->write(frame);
}

void ShelfDisplayDialog::onReadyReadLeft() { processReadyRead(shelfSocketLeft, m_bufferLeft, false); }
void ShelfDisplayDialog::onReadyReadRight() { processReadyRead(shelfSocketRight, m_bufferRight, true); }

void ShelfDisplayDialog::processReadyRead(QTcpSocket* socket, QByteArray& buffer, bool isRight) {
    if (!socket) return;
    buffer.append(socket->readAll());

    while (buffer.size() >= 7) {
        int startIndex = -1;
        for (int i = 0; i <= buffer.size() - 7; ++i) {
            if (buffer.at(i+1) == (char)0x03) { startIndex = i; break; }
        }
        if (startIndex == -1) { if (buffer.size() > 1024) buffer.clear(); break; }
        if (startIndex > 0) buffer.remove(0, startIndex);

        quint8 currentDeviceAddr = buffer.at(0);
        int combinedAddr = isRight ? (currentDeviceAddr + 256) : currentDeviceAddr;

        quint8 byteCount = buffer.at(2);
        int expectedLength = 3 + byteCount + 2;
        if (buffer.size() < expectedLength) break;

        QByteArray frame = buffer.left(expectedLength);
        quint16 calcCrc = calculateCRC16(frame.left(expectedLength - 2));
        quint8 crcL = frame.at(expectedLength - 2); quint8 crcH = frame.at(expectedLength - 1);

        if (crcL == (calcCrc & 0xFF) && crcH == ((calcCrc >> 8) & 0xFF)) {
            if (byteCount == 0x04) {
                quint32 rawPayload = (static_cast<quint8>(frame.at(3)) << 24) | (static_cast<quint8>(frame.at(4)) << 16) | (static_cast<quint8>(frame.at(5)) << 8)  | static_cast<quint8>(frame.at(6));
                qint32 currentQty = rawPayload & 0x00FFFFFF;
                if (currentQty & 0x00800000) currentQty |= 0xFF000000;

                emit realTimeQuantityReceived(combinedAddr, currentQty);

                QString cellId = getCellIdByAddress(combinedAddr);
                if (!cellId.isEmpty()) {
                    ShelfData data = ShelfDatabase::getInstance().getItem(cellId);
                    if (data.count != currentQty) ShelfDatabase::getInstance().updateItem(cellId, data.name, data.spec, data.code, currentQty);

                    QPushButton* btn = this->findChild<QPushButton*>("btn_" + cellId);
                    if (btn) { bool isBig = cellId.startsWith("A-4"); btn->setStyleSheet(getShelfStyle(currentQty, isBig)); }
                }
            }
            buffer.remove(0, expectedLength);
        } else {
            buffer.remove(0, 2);
        }
    }
}

quint16 ShelfDisplayDialog::calculateCRC16(const QByteArray &data) {
    quint16 crc = 0xFFFF;
    for (int pos = 0; pos < data.length(); pos++) {
        crc ^= (quint8)data.at(pos);
        for (int i = 8; i != 0; i--) {
            if ((crc & 0x0001) != 0) { crc >>= 1; crc ^= 0xA001; } else { crc >>= 1; }
        }
    }
    return crc;
}

void ShelfDisplayDialog::showEvent(QShowEvent *event) {
    QDialog::showEvent(event);
    if (!tcpServerLeft->isListening()) tcpServerLeft->listen(QHostAddress::Any, 1080);
    if (!tcpServerRight->isListening()) tcpServerRight->listen(QHostAddress::Any, 1081);
}

void ShelfDisplayDialog::hideEvent(QHideEvent *event) {
    QDialog::hideEvent(event);
    if (pollTimer && pollTimer->isActive()) pollTimer->stop();
    if (tcpServerLeft && tcpServerLeft->isListening()) tcpServerLeft->close();
    if (tcpServerRight && tcpServerRight->isListening()) tcpServerRight->close();

    if (shelfSocketLeft) { disconnect(shelfSocketLeft, &QTcpSocket::disconnected, this, &ShelfDisplayDialog::onClientDisconnectedLeft); shelfSocketLeft->abort(); shelfSocketLeft->deleteLater(); shelfSocketLeft = nullptr; }
    if (shelfSocketRight) { disconnect(shelfSocketRight, &QTcpSocket::disconnected, this, &ShelfDisplayDialog::onClientDisconnectedRight); shelfSocketRight->abort(); shelfSocketRight->deleteLater(); shelfSocketRight = nullptr; }
}

void ShelfDisplayDialog::on_btn_import_parts_clicked() {
    QString fileName = QFileDialog::getOpenFileName(this, "选择零件表", "", "CSV 文件 (*.csv)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) { QMessageBox::critical(this, "错误", "无法打开文件！"); return; }

    m_importQueue.clear();
    QTextStream in(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    in.setEncoding(QStringConverter::Utf8);
#else
    in.setCodec("UTF-8");
#endif
    in.readLine();
    while (!in.atEnd()) {
        QString line = in.readLine(); QStringList fields = line.split(",");
        if (fields.size() >= 5) {
            PartImportData data; data.cellId = fields[0].trimmed(); data.name = fields[1].trimmed();
            data.spec = fields[2].trimmed(); data.code = fields[3].trimmed(); data.quantity = fields[4].toInt();
            m_importQueue.append(data);
        }
    }
    file.close();
    if (m_importQueue.isEmpty()) { QMessageBox::warning(this, "提示", "未读取到有效数据！"); return; }
    if (pollTimer && pollTimer->isActive()) pollTimer->stop();

    ui->btn_import_parts->setEnabled(false); ui->btn_import_parts->setText("正在烧录硬件...");
    m_currentImportStep = 0; m_importTimer->start(80);
}

void ShelfDisplayDialog::onImportTimerTimeout() {
    if (m_importQueue.isEmpty()) {
        m_importTimer->stop();
        if (pollTimer) pollTimer->start(500);
        ui->btn_import_parts->setEnabled(true); ui->btn_import_parts->setText("导入零件表");
        QMessageBox::information(this, "完成", "零件表已全部烧录至硬件并同步至本地！");
        InitUI();
        return;
    }

    PartImportData currentPart = m_importQueue.first();
    int combinedAddr = getCombinedAddress(currentPart.cellId);

    if (m_currentImportStep == 0) { sendStringCommand(combinedAddr, 0x0100, currentPart.name, 20); m_currentImportStep++; }
    else if (m_currentImportStep == 1) { sendStringCommand(combinedAddr, 0x0101, currentPart.spec, 20); m_currentImportStep++; }
    else if (m_currentImportStep == 2) { sendStringCommand(combinedAddr, 0x0102, currentPart.code, 91); m_currentImportStep++; }
    else if (m_currentImportStep == 3) {
        sendInt32Command(combinedAddr, 0x0106, currentPart.quantity);
        ShelfDatabase::getInstance().updateItem(currentPart.cellId, currentPart.name, currentPart.spec, currentPart.code, currentPart.quantity);
        m_importQueue.removeFirst(); m_currentImportStep = 0;
    }
}

void ShelfDisplayDialog::sendStringCommand(int combinedAddr, quint16 reg, const QString &text, int maxBytes) {
    bool isRight = combinedAddr >= 256; QTcpSocket *sock = isRight ? shelfSocketRight : shelfSocketLeft;
    quint8 addr = combinedAddr % 256;

    QByteArray gbkData = text.toLocal8Bit();
    if (gbkData.size() > maxBytes) gbkData.resize(maxBytes);
    QByteArray payload = gbkData; if (payload.size() % 2 != 0) payload.append(char(0x00));

    QByteArray frame; frame.append(addr); frame.append(char(0x10));
    frame.append(char((reg >> 8) & 0xFF)); frame.append(char(reg & 0xFF));
    quint16 regCount = payload.size() / 2; frame.append(char((regCount >> 8) & 0xFF)); frame.append(char(regCount & 0xFF));
    frame.append(char(payload.size() & 0xFF)); frame.append(payload);
    quint16 crc = calculateCRC16(frame); frame.append(char(crc & 0xFF)); frame.append(char((crc >> 8) & 0xFF));

    if (sock && sock->state() == QAbstractSocket::ConnectedState) { sock->write(frame); sock->flush(); }
}

void ShelfDisplayDialog::sendInt32Command(int combinedAddr, quint16 reg, int value) {
    bool isRight = combinedAddr >= 256; QTcpSocket *sock = isRight ? shelfSocketRight : shelfSocketLeft;
    quint8 addr = combinedAddr % 256;

    QByteArray data; data.append(char((value >> 24) & 0xFF)); data.append(char((value >> 16) & 0xFF)); data.append(char((value >> 8) & 0xFF));  data.append(char(value & 0xFF));
    QByteArray frame; frame.append(addr); frame.append(char(0x10)); frame.append(char((reg >> 8) & 0xFF)); frame.append(char(reg & 0xFF));
    frame.append(char(0x00)); frame.append(char(0x02)); frame.append(char(0x04)); frame.append(data);
    quint16 crc = calculateCRC16(frame); frame.append(char(crc & 0xFF)); frame.append(char((crc >> 8) & 0xFF));

    if (sock && sock->state() == QAbstractSocket::ConnectedState) { sock->write(frame); sock->flush(); }
}