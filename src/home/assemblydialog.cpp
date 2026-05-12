#include "assemblydialog.h"
#include "qmessagebox.h"
#include "stepshowform.h"
#include "rackcfgform.h"
#include <QListWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QUrl>
#include <QDebug>
#include <QGuiApplication>
#include <QScreen>
#include <QTimer>
#include <QPair>
#include "database/shelfdatabase.h"
#include <QSet>
#include <QScroller>
#include <QEventLoop>
#include <QMediaDevices>
#include <QCameraDevice>
#include <opencv2/opencv.hpp>
#include <QVideoSink>
#include <QQueue> // 必须包含队列头文件

static QMap<int, int> s_debounceMap;
static QMap<int, int> s_initialQuantities;

AssemblyDialog::AssemblyDialog(QWidget *parent)
    : QDialog(parent), m_shelfSocketLeft(nullptr), m_shelfSocketRight(nullptr)
{
    ShelfDatabase::getInstance().initDB();
    initUI();

    // ================= 启动双路智能货架通讯服务 =================
    m_tcpServerLeft = new QTcpServer(this);
    m_tcpServerRight = new QTcpServer(this);

    connect(m_tcpServerLeft, &QTcpServer::newConnection, this, &AssemblyDialog::onNewConnectionLeft);
    connect(m_tcpServerRight, &QTcpServer::newConnection, this, &AssemblyDialog::onNewConnectionRight);

    m_pollTimer = new QTimer(this);
    connect(m_pollTimer, &QTimer::timeout, this, &AssemblyDialog::onPollTimeout);

    // 【新增】串行队列超时计时器：300ms 没收到回复则强制发送下一条，防止队列死锁
    m_ackTimeoutTimer = new QTimer(this);
    m_ackTimeoutTimer->setSingleShot(true);
    connect(m_ackTimeoutTimer, &QTimer::timeout, this, [this](){
        qDebug() << "警告：Modbus 指令响应超时，强制执行下一项任务";
        processNextTask();
    });
}

AssemblyDialog::~AssemblyDialog()
{
    if (m_shelfSocketLeft) m_shelfSocketLeft->disconnectFromHost();
    if (m_shelfSocketRight) m_shelfSocketRight->disconnectFromHost();
}

// ========================= 串行队列核心实现 (同步等待方案) =========================

void AssemblyDialog::addModbusTask(int addr, quint8 func, quint16 reg, const QByteArray &data) {
    m_taskQueue.enqueue({addr, func, reg, data});
    // 如果当前没有正在处理的任务，则立即启动
    if (!m_isProcessingQueue) {
        processNextTask();
    }
}

void AssemblyDialog::processNextTask() {
    if (m_taskQueue.isEmpty()) {
        m_isProcessingQueue = false;
        // 队列全部处理完毕，恢复轮询定时器
        if (!m_activeRacks.isEmpty() && m_pollTimer && !m_pollTimer->isActive()) {
            m_pollTimer->start(200);
        }
        return;
    }

    m_isProcessingQueue = true;
    ModbusTask task = m_taskQueue.dequeue();

    // 物理发送指令
    sendModbusCommand(task.addr, task.func, task.reg, task.data);

    // 启动超时监测 (等待硬件 ACK 回复)
    m_ackTimeoutTimer->start(350);
}

void AssemblyDialog::sendModbusCommand(int combinedAddr, quint8 funcCode, quint16 regAddress, const QByteArray &dataParams) {
    bool isRight = combinedAddr >= 256;
    QTcpSocket *sock = isRight ? m_shelfSocketRight : m_shelfSocketLeft;
    quint8 address = combinedAddr % 256;

    if (!sock || sock->state() != QAbstractSocket::ConnectedState) {
        // 如果 Socket 不可用，跳过该任务尝试下一条
        QTimer::singleShot(10, this, &AssemblyDialog::processNextTask);
        return;
    }

    QByteArray payload = dataParams;
    while (payload.size() < 4) payload.append(char(0x00));

    QByteArray frame;
    frame.append(address);
    frame.append(funcCode);
    frame.append(char((regAddress >> 8) & 0xFF));
    frame.append(char(regAddress & 0xFF));
    frame.append(char(0x00));
    frame.append(char(0x01)); // 寄存器数量固定 0x0001
    frame.append(char(0x04)); // 字节数 0x04
    frame.append(payload.left(4));

    quint16 crc = calculateCRC16(frame);
    frame.append(char(crc & 0xFF));
    frame.append(char((crc >> 8) & 0xFF));

    sock->write(frame);
    sock->flush();
}

void AssemblyDialog::initUI()
{
    this->resize(900, 600);
    this->setObjectName("AssemblyDialog");

    m_listWidget = new QListWidget(this);
    m_listWidget->setStyleSheet("font-size:15px;");
    m_listWidget->setFixedWidth(180);

    m_cameraWidget = new QVideoWidget(this);
    m_cameraWidget->setFixedSize(180, 135);
    m_cameraWidget->hide();

    m_roiOverlay = new QLabel(m_cameraWidget);
    m_roiOverlay->setStyleSheet("border: 5px solid #00FF00; background: transparent;");
    m_roiOverlay->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_roiOverlay->hide();

    QVBoxLayout *leftLayout = new QVBoxLayout;
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(5);
    leftLayout->addWidget(m_listWidget, 1);

    QLabel *camLabel = new QLabel("选择摄像头设备:", this);
    camLabel->setStyleSheet("font-size:12px; color:#666; padding-left:5px;");
    leftLayout->addWidget(camLabel, 0);

    m_comboCamera = new QComboBox(this);
    m_comboCamera->setFixedWidth(180);
    m_comboCamera->setFixedHeight(30);
    leftLayout->addWidget(m_comboCamera, 0);

    m_cameraWidget->setFixedSize(180, 135);
    leftLayout->addWidget(m_cameraWidget, 0);

    connect(m_comboCamera, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AssemblyDialog::onCameraChanged);

    m_labelProgress = new QLabel("步骤 0 / 0", this);
    m_labelProgress->setAlignment(Qt::AlignCenter);
    m_labelProgress->setStyleSheet("font-size:16px;font-weight:bold;");

    m_videoWidget = new QVideoWidget(this);
    QFrame *videoFrame = new QFrame(this);
    videoFrame->setObjectName("frameVideo");
    QVBoxLayout *videoLayout = new QVBoxLayout(videoFrame);
    videoLayout->setContentsMargins(5,5,5,5);
    videoLayout->addWidget(m_videoWidget);

    m_player = new QMediaPlayer(this);
    m_player->setVideoOutput(m_videoWidget);

    m_labelStep = new QLabel("步骤说明", this);
    m_labelStep->setObjectName("labelStep");
    m_labelStep->setMinimumHeight(60);
    m_labelStep->setWordWrap(true);
    m_labelStep->setStyleSheet("font-size:14px; color:#333; line-height:1.5; padding:5px;");

    m_btnPlayPause = new QPushButton("暂停", this);
    m_btnPlayPause->setObjectName("btnPlay");
    m_btnNext = new QPushButton("下一步", this);
    m_btnNext->setObjectName("btnNext");
    m_btnPrev = new QPushButton("上一步", this);
    m_btnPrev->setObjectName("btnPrev");
    QPushButton *btnBack = new QPushButton("返回首页", this);
    btnBack->setObjectName("btnBack");

    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnPrev);
    btnLayout->addWidget(m_btnPlayPause);
    btnLayout->addWidget(m_btnNext);
    btnLayout->addWidget(btnBack);
    btnLayout->addStretch();

    QVBoxLayout *rightLayout = new QVBoxLayout;
    rightLayout->setSpacing(15);
    rightLayout->addWidget(m_labelProgress);
    rightLayout->addWidget(videoFrame, 8);
    rightLayout->addWidget(m_labelStep, 1);
    rightLayout->addLayout(btnLayout);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(15,15,15,15);
    mainLayout->setSpacing(15);
    mainLayout->addLayout(leftLayout);
    mainLayout->addLayout(rightLayout, 1);

    connect(m_listWidget, &QListWidget::itemClicked, this, &AssemblyDialog::onDeviceSelected);
    connect(m_listWidget, &QListWidget::currentRowChanged, this, &AssemblyDialog::playStep);
    connect(m_btnNext, &QPushButton::clicked, this, &AssemblyDialog::onNextStep);
    connect(m_btnPrev, &QPushButton::clicked, this, &AssemblyDialog::onPrevStep);
    connect(m_btnPlayPause, &QPushButton::clicked, this, &AssemblyDialog::onPlayPause);

    connect(btnBack, &QPushButton::clicked, this, [this]() {
        if (m_camera && m_camera->isActive()) {
            m_camera->stop();
            m_cameraWidget->hide();
        }
        m_currentDevice.clear();
        emit signalBackHome();
    });

    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &AssemblyDialog::onMediaStatusChanged);
    QScroller::grabGesture(m_listWidget->viewport(), QScroller::LeftMouseButtonGesture);
}

void AssemblyDialog::loadDeviceList() {
    for (auto it = m_data.begin(); it != m_data.end(); ++it) {
        m_listWidget->addItem(it.key());
    }
}

void AssemblyDialog::onDeviceSelected() {
    QString text = m_listWidget->currentItem()->text();
    if (!m_data.contains(text)) return;
    m_currentDevice = text;
    m_currentSteps = m_data[m_currentDevice];

    m_listWidget->blockSignals(true);
    m_listWidget->clear();
    for (int i = 0; i < m_currentSteps.size(); ++i) {
        m_listWidget->addItem(QString("步骤 %1").arg(i + 1));
    }
    m_listWidget->blockSignals(false);
    m_currentIndex = 0;
    m_listWidget->setCurrentRow(0);

    QTimer::singleShot(100, this, [this]() {
        const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
        m_comboCamera->blockSignals(true);
        m_comboCamera->clear();
        int defaultIdx = 0;
        for (int i = 0; i < cameras.size(); ++i) {
            m_comboCamera->addItem(cameras[i].description(), QVariant::fromValue(cameras[i]));
            if (cameras[i].description().contains("UGREEN", Qt::CaseInsensitive)) {
                defaultIdx = i;
            }
        }
        m_comboCamera->setCurrentIndex(defaultIdx);
        m_comboCamera->blockSignals(false);
        onCameraChanged(m_comboCamera->currentIndex());
    });
}

void AssemblyDialog::playStep(int index) {
    if (m_currentSteps.isEmpty() || index < 0 || index >= m_currentSteps.size()) return;
    m_currentIndex = index;
    StepShowForm* form = m_currentSteps[index];

    QString stepDesc = form->getStepText();
    QVector<QSharedPointer<RackCFGForm>> rackForm = form->getRackForm();
    QString rackTips;
    for (auto rackItem : rackForm) {
        rackTips += QString(" [👉请前往 %1: 拿 %2 个]").arg(rackItem->getRackNumber()).arg(rackItem->getQuantityGoods());
    }
    if (!rackTips.isEmpty()) stepDesc += "   " + rackTips;

    m_labelProgress->setText(QString("步骤 %1 / %2").arg(index + 1).arg(m_currentSteps.size()));
    QString sopText = form->getSopText();
    if (!sopText.isEmpty()) {
        stepDesc = QString("<b>%1</b><br><br><span style='color:#0055ff;'>%2</span>")
        .arg(stepDesc).arg(sopText.replace("\n", "<br>"));
    } else {
        stepDesc = QString("<b>%1</b>").arg(stepDesc);
    }
    m_labelStep->setText(stepDesc);

    updateRackLights();

    QString vPath = form->getVideoPath();
    if (!vPath.isEmpty()) {
        m_player->stop();
        m_player->setSource(QUrl::fromLocalFile(vPath));
        m_btnPlayPause->setText("播放");
    }

    QString pPath = form->getProjectorImagePath();
    QString projVideoPath = form->getProjectorVideoPath();
    QList<QScreen *> screens = QGuiApplication::screens();
    QScreen *projectorScreen = nullptr;
    if (screens.size() >= 3) projectorScreen = screens.at(1);

    if (projectorScreen) {
        if (!m_projWin) m_projWin = new ProjectorWindow();
        if (!projVideoPath.isEmpty()) m_projWin->playAnimationVideo(projVideoPath, projectorScreen->geometry());
        else if (!pPath.isEmpty()) m_projWin->showImage(pPath, projectorScreen->geometry());
        else m_projWin->hide();
    } else if (m_projWin) {
        m_projWin->hide();
    }
}

void AssemblyDialog::onNextStep() {
    if (m_currentIndex < m_currentSteps.size() - 1) {
        m_currentIndex++;
        m_listWidget->setCurrentRow(m_currentIndex);
    }
}

void AssemblyDialog::onPrevStep() {
    if (m_currentIndex > 0) {
        m_currentIndex--;
        m_listWidget->setCurrentRow(m_currentIndex);
    }
}

void AssemblyDialog::onPlayPause() {
    if (m_player->playbackState() == QMediaPlayer::PlayingState) {
        m_player->pause();
        m_btnPlayPause->setText("播放");
    } else {
        m_player->play();
        m_btnPlayPause->setText("暂停");
    }
}

void AssemblyDialog::onMediaStatusChanged(QMediaPlayer::MediaStatus status) {
    if (status == QMediaPlayer::EndOfMedia) {
        QListWidgetItem *item = m_listWidget->item(m_currentIndex);
        if (item && !item->text().contains("✔")) item->setText(item->text() + " ✔");
        onNextStep();
    }
}

void AssemblyDialog::updateData(QMap<QString, QList<StepShowForm*>> data) {
    m_data = data;
    m_currentDevice.clear();
    m_currentSteps.clear();
    m_currentIndex = -1;
    m_listWidget->blockSignals(true);
    m_listWidget->clear();
    m_listWidget->blockSignals(false);
    m_player->stop();
    if (m_projWin) m_projWin->stopVideo();
    m_labelStep->setText("步骤说明");
    m_labelProgress->setText("步骤 0 / 0");
    loadDeviceList();
}

// ========================= 智能货架通讯逻辑 =========================

void AssemblyDialog::onNewConnectionLeft() {
    m_shelfSocketLeft = m_tcpServerLeft->nextPendingConnection();
    connect(m_shelfSocketLeft, &QTcpSocket::disconnected, this, &AssemblyDialog::onClientDisconnectedLeft);
    connect(m_shelfSocketLeft, &QTcpSocket::readyRead, this, &AssemblyDialog::onReadyReadLeft);
    qDebug() << "【左侧货架】TCP 已握手，等待协议栈初始化...";
    QTimer::singleShot(800, this, [this]() {
        if (m_shelfSocketLeft && m_shelfSocketLeft->state() == QAbstractSocket::ConnectedState) {
            updateRackLights();
        }
    });
}

void AssemblyDialog::onNewConnectionRight() {
    m_shelfSocketRight = m_tcpServerRight->nextPendingConnection();
    connect(m_shelfSocketRight, &QTcpSocket::disconnected, this, &AssemblyDialog::onClientDisconnectedRight);
    connect(m_shelfSocketRight, &QTcpSocket::readyRead, this, &AssemblyDialog::onReadyReadRight);
    qDebug() << "【右侧货架】TCP 已握手，等待协议栈初始化...";
    QTimer::singleShot(800, this, [this]() {
        if (m_shelfSocketRight && m_shelfSocketRight->state() == QAbstractSocket::ConnectedState) {
            updateRackLights();
        }
    });
}

void AssemblyDialog::onClientDisconnectedLeft() {
    m_bufferLeft.clear();
    if (m_shelfSocketLeft) { m_shelfSocketLeft->deleteLater(); m_shelfSocketLeft = nullptr; }
    if (!m_shelfSocketRight) m_pollTimer->stop();
}

void AssemblyDialog::onClientDisconnectedRight() {
    m_bufferRight.clear();
    if (m_shelfSocketRight) { m_shelfSocketRight->deleteLater(); m_shelfSocketRight = nullptr; }
    if (!m_shelfSocketLeft) m_pollTimer->stop();
}

int AssemblyDialog::calculateModbusAddress(QString cellId) {
    QStringList parts = cellId.split("-");
    if(parts.size() != 3) return 0;
    QString side = parts[0];
    int row = parts[1].toInt();
    int col = parts[2].toInt();
    bool isRight = false;
    int localCol = col;
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

QString AssemblyDialog::getCellIdByAddress(int combinedAddr) {
    bool isRight = combinedAddr >= 256;
    quint8 addr = combinedAddr % 256;
    int colOffset = isRight ? 5 : 0;
    int colOffsetRow4 = isRight ? 3 : 0;
    if (addr >= 0x43 && addr <= 0x47) return QString("A-1-%1").arg(addr - 0x43 + 1 + colOffset);
    if (addr >= 0x39 && addr <= 0x3D) return QString("A-2-%1").arg(addr - 0x39 + 1 + colOffset);
    if (addr >= 0x34 && addr <= 0x38) return QString("A-3-%1").arg(addr - 0x34 + 1 + colOffset);
    if (addr >= 0x31 && addr <= 0x33) return QString("A-4-%1").arg(addr - 0x31 + 1 + colOffsetRow4);
    if (addr <= 0x4C && addr >= 0x48) return QString("B-1-%1").arg(0x4C - addr + 1 + colOffset);
    if (addr <= 0x42 && addr >= 0x3E) return QString("B-2-%1").arg(0x42 - addr + 1 + colOffset);
    return "";
}

quint16 AssemblyDialog::calculateCRC16(const QByteArray &data) {
    quint16 crc = 0xFFFF;
    for (int pos = 0; pos < data.length(); pos++) {
        crc ^= (quint8)data.at(pos);
        for (int i = 8; i != 0; i--) {
            if ((crc & 0x0001) != 0) { crc >>= 1; crc ^= 0xA001; } else { crc >>= 1; }
        }
    }
    return crc;
}

void AssemblyDialog::sendPlanQuantity(int addr, int qty) {
    QByteArray data;
    data.append(char((qty >> 24) & 0xFF)); data.append(char((qty >> 16) & 0xFF));
    data.append(char((qty >> 8) & 0xFF));  data.append(char(qty & 0xFF));
    addModbusTask(addr, 0x10, 0x010B, data);
}

void AssemblyDialog::sendClearOperation(int addr) {
    QByteArray data(4, 0x00);
    addModbusTask(addr, 0x10, 0x010C, data);
}

void AssemblyDialog::showEvent(QShowEvent *event) {
    QDialog::showEvent(event);
    if (!m_tcpServerLeft->isListening()) m_tcpServerLeft->listen(QHostAddress::Any, 1080);
    if (!m_tcpServerRight->isListening()) m_tcpServerRight->listen(QHostAddress::Any, 1081);
}

void AssemblyDialog::hideEvent(QHideEvent *event) {
    QDialog::hideEvent(event);
    m_pollTimer->stop();
    m_taskQueue.clear(); // 退出时清空队列
    m_isProcessingQueue = false;

    QSet<int> allPossibleLamps = m_physicalOnSet;
    for(int a : m_activeRacks) allPossibleLamps.insert(a);
    for (int addr : allPossibleLamps) {
        addModbusTask(addr, 0x10, 0x010C, QByteArray(4, 0x00));
    }

    m_physicalOnSet.clear();
    m_activeRacks.clear();
    m_targetQuantities.clear();
    s_debounceMap.clear();
    s_initialQuantities.clear();
}

// ================== 亮灯函数 (重构为队列版) ==================
void AssemblyDialog::updateRackLights()
{
    if (m_currentSteps.isEmpty() || m_currentIndex < 0) return;

    // 1. 立即清空当前队列，防止旧指令干扰新步骤
    m_taskQueue.clear();
    m_isProcessingQueue = false;
    if (m_pollTimer) m_pollTimer->stop();

    StepShowForm* form = m_currentSteps[m_currentIndex];
    QVector<QSharedPointer<RackCFGForm>> rackForm = form->getRackForm();

    QSet<int> currentStepAddrs;
    for (auto rackItem : rackForm) {
        if (rackItem->getQuantityGoods().toInt() > 0)
            currentStepAddrs.insert(calculateModbusAddress(rackItem->getRackNumber()));
    }

    // --- 阶段 A：灭灯任务 (强制进队) ---
    QList<int> toRemove;
    for (int addr : std::as_const(m_physicalOnSet)) {
        if (!currentStepAddrs.contains(addr)) {
            toRemove.append(addr);
            addModbusTask(addr, 0x10, 0x010C, QByteArray(4, 0x00));
        }
    }
    for(int addr : toRemove) m_physicalOnSet.remove(addr);

    // --- 阶段 B：开启新灯任务 (强制进队) ---
    m_activeRacks.clear();
    m_targetQuantities.clear();

    for (auto rackItem : rackForm) {
        int qty = rackItem->getQuantityGoods().toInt();
        if (qty <= 0) continue;
        int addr = calculateModbusAddress(rackItem->getRackNumber());

        m_activeRacks.append(addr);
        m_physicalOnSet.insert(addr);

        // 1. 先发 Clear 指令重置硬件
        addModbusTask(addr, 0x10, 0x010C, QByteArray(4, 0x00));

        // 2. 再发 PlanQuantity 指令开启拣选
        QByteArray data;
        data.append(char((qty >> 24) & 0xFF)); data.append(char((qty >> 16) & 0xFF));
        data.append(char((qty >> 8) & 0xFF));  data.append(char(qty & 0xFF));
        addModbusTask(addr, 0x10, 0x010B, data);

        int initialStock = ShelfDatabase::getInstance().getItem(rackItem->getRackNumber()).count;
        m_targetQuantities.insert(addr, initialStock - qty);
        s_initialQuantities.insert(addr, initialStock);
    }
}

void AssemblyDialog::onPollTimeout()
{
    if (m_activeRacks.isEmpty()) return;
    if (m_isProcessingQueue) return; // 如果正在执行写操作，跳过轮询

    m_pollIndex++;
    if (m_pollIndex >= m_activeRacks.size()) m_pollIndex = 0;

    int combinedAddr = m_activeRacks[m_pollIndex];
    bool isRight = combinedAddr >= 256;
    QTcpSocket *sock = isRight ? m_shelfSocketRight : m_shelfSocketLeft;
    quint8 targetAddr = combinedAddr % 256;

    if (!sock || sock->state() != QAbstractSocket::ConnectedState) return;

    QByteArray frame;
    frame.append(targetAddr);
    frame.append(0x03);
    frame.append(char(0x01)); frame.append(char(0x0F));
    frame.append(char(0x00)); frame.append(char(0x02));

    quint16 crc = calculateCRC16(frame);
    frame.append(char(crc & 0xFF));
    frame.append(char((crc >> 8) & 0xFF));

    sock->write(frame);
    sock->flush();
}

void AssemblyDialog::onReadyReadLeft() { processReadyRead(m_shelfSocketLeft, m_bufferLeft, false); }
void AssemblyDialog::onReadyReadRight() { processReadyRead(m_shelfSocketRight, m_bufferRight, true); }

void AssemblyDialog::processReadyRead(QTcpSocket* socket, QByteArray& buffer, bool isRight)
{
    if (!socket) return;
    buffer.append(socket->readAll());

    while (buffer.size() >= 7) {
        int startIndex = -1;
        for (int i = 0; i <= buffer.size() - 7; ++i) {
            // 兼容查询回复(03)和设置回复(10)
            if (buffer.at(i+1) == (char)0x03 || buffer.at(i+1) == (char)0x10) {
                startIndex = i; break;
            }
        }
        if (startIndex == -1) { buffer.clear(); break; }
        if (startIndex > 0) buffer.remove(0, startIndex);

        quint8 funcCode = buffer.at(1);

        // --- 核心修改：如果收到 0x10 写指令的确认包 ---
        if (funcCode == 0x10) {
            if (buffer.size() < 8) break; // ACK 包通常为 8 字节
            m_ackTimeoutTimer->stop(); // 停止超时监测
            buffer.remove(0, 8);
            processNextTask(); // 发送队列中下一条指令
            continue;
        }

        // --- 原有逻辑：处理 0x03 轮询回复 ---
        if (funcCode == 0x03) {
            quint8 byteCount = buffer.at(2);
            int expectedLength = 3 + byteCount + 2;
            if (buffer.size() < expectedLength) break;

            QByteArray frame = buffer.left(expectedLength);
            quint16 calcCrc = calculateCRC16(frame.left(expectedLength - 2));
            if ((quint8)frame.at(expectedLength-2) == (calcCrc & 0xFF) &&
                (quint8)frame.at(expectedLength-1) == ((calcCrc >> 8) & 0xFF)) {

                quint32 rawPayload = (static_cast<quint8>(frame.at(3)) << 24) |
                                     (static_cast<quint8>(frame.at(4)) << 16) |
                                     (static_cast<quint8>(frame.at(5)) << 8)  |
                                     static_cast<quint8>(frame.at(6));

                qint32 currentRemainingQty = rawPayload & 0x00FFFFFF;
                if (currentRemainingQty & 0x00800000) currentRemainingQty |= 0xFF000000;

                int currentAddr = (quint8)frame.at(0) + (isRight ? 256 : 0);

                if (m_targetQuantities.contains(currentAddr)) {
                    int expectedQty = m_targetQuantities[currentAddr];
                    int initialQty = s_initialQuantities[currentAddr];

                    // 【核心修改点】：灵敏度优化
                    // 只要实时读取到一次目标剩余数量，立即触发灭灯指令，无需多次确认
                    if (currentRemainingQty == expectedQty && currentRemainingQty < initialQty) {
                        sendClearOperation(currentAddr);
                        m_activeRacks.removeOne(currentAddr);
                        m_targetQuantities.remove(currentAddr);
                        s_debounceMap.remove(currentAddr);
                        s_initialQuantities.remove(currentAddr);
                    } else {
                        s_debounceMap[currentAddr] = 0;
                    }
                }
            }
            buffer.remove(0, expectedLength);
        }
    }
}

// 实现摄像头切换核心代码 (保持不动)
void AssemblyDialog::onCameraChanged(int index) {
    if (index < 0 || index >= m_comboCamera->count()) return;
    QCameraDevice targetDevice = m_comboCamera->itemData(index).value<QCameraDevice>();
    if (m_camera) {
        m_camera->stop();
        delete m_camera;
        m_camera = nullptr;
    }
    m_camera = new QCamera(targetDevice, this);
    if (!m_captureSession) {
        m_captureSession = new QMediaCaptureSession(this);
        m_captureSession->setVideoOutput(m_cameraWidget);
        m_visionProcessor = new VisionProcessor(this);
        connect(m_captureSession->videoSink(), &QVideoSink::videoFrameChanged,
                m_visionProcessor, &VisionProcessor::processFrame);
        connect(m_visionProcessor, &VisionProcessor::handDetected, this, [this](){
            if (m_player->playbackState() != QMediaPlayer::PlayingState) {
                m_player->play();
                m_btnPlayPause->setText("暂停");
            }
        });
    }
    m_cameraWidget->show();
    m_captureSession->setCamera(m_camera);
    m_camera->start();
    if (m_camera) {
        m_cameraWidget->show();
        m_camera->start();
        double scale = 0.28;
        int uiX = 50 * scale;
        int uiY = 30 * scale;
        int uiW = 260 * scale;
        int uiH = 420 * scale;
        m_roiOverlay->setGeometry(uiX, uiY, uiW, uiH);
        m_roiOverlay->show();
        m_roiOverlay->raise();
    }
}