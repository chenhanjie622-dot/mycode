#ifndef ASSEMBLYDIALOG_H
#define ASSEMBLYDIALOG_H

#include <QDialog>
#include <QMap>
#include <QList>
#include <QMediaPlayer>
#include <QTcpServer>
#include <QTcpSocket>
#include "ProjectorWindow.h"
#include <QTimer>
#include <QCamera>
#include <QMediaCaptureSession>
#include <QVideoWidget>
#include "visionprocessor.h"
#include <QComboBox>
#include <QQueue>

QT_BEGIN_NAMESPACE
class QListWidget;
class QLabel;
class QPushButton;
class QVideoWidget;
class QMediaPlayer;
QT_END_NAMESPACE

class StepShowForm;

class AssemblyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AssemblyDialog(QWidget *parent = nullptr);
    ~AssemblyDialog();

    void updateData(QMap<QString, QList<StepShowForm*>> data);

private:
    void initUI();
    void loadDeviceList();
    void playStep(int index);

    // ================= 智能货架控制核心 =================
    void updateRackLights();
    int calculateModbusAddress(QString cellId);               // 【修改】返回携带左右区分的混合地址
    quint16 calculateCRC16(const QByteArray &data);
    QString getCellIdByAddress(int combinedAddr);             // 【修改】将混合地址转回 UI 格式
    void sendModbusCommand(int combinedAddr, quint8 funcCode, quint16 regAddress, const QByteArray &dataParams);
    void sendPlanQuantity(int combinedAddr, int qty);
    void sendClearOperation(int combinedAddr);
    int m_commandSequence = 0;      // 指令序列号
    QList<int> m_lastActiveRacks;   // 记录【真正】发过点灯指令的地址
    QSet<int> m_physicalOnSet;      // 【新增】记录当前所有“物理上可能亮着”的货位地址
    struct ModbusTask {
        int addr;
        quint8 func;
        quint16 reg;
        QByteArray data;
    };
    QQueue<ModbusTask> m_taskQueue; // 指令队列
    bool m_isProcessingQueue = false; // 队列是否正在执行
    QTimer *m_ackTimeoutTimer; // 防止硬件死机导致队列卡死

    void addModbusTask(int addr, quint8 func, quint16 reg, const QByteArray &data);
    void processNextTask();

private slots:
    void onDeviceSelected();
    void onNextStep();
    void onPrevStep();
    void onPlayPause();
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);

    // 【修改】双路网络连接槽函数
    void onNewConnectionLeft();
    void onNewConnectionRight();
    void onClientDisconnectedLeft();
    void onClientDisconnectedRight();
    void onPollTimeout();

    void onReadyReadLeft();
    void onReadyReadRight();
    void processReadyRead(QTcpSocket* socket, QByteArray& buffer, bool isRight);
    void onCameraChanged(int index); // 新增切换槽函数

private:
    QMap<QString, QList<StepShowForm*>> m_data;

    QString m_currentDevice;
    QList<StepShowForm*> m_currentSteps;
    int m_currentIndex = 0;

    // ================= UI =================
    QListWidget *m_listWidget;

    QLabel *m_labelStep;
    QLabel *m_labelProgress;

    QPushButton *m_btnNext;
    QPushButton *m_btnPrev;
    QPushButton *m_btnPlayPause;

    QMediaPlayer *m_player;
    QVideoWidget *m_videoWidget;

    QComboBox *m_comboCamera; // 新增下拉框指针

    ProjectorWindow *m_projWin = nullptr;

    QLabel *m_roiOverlay = nullptr; // 用于显示红框的透明层

    // ================= 网络通讯变量(双通道) =================
    QTcpServer *m_tcpServerLeft;
    QTcpServer *m_tcpServerRight;
    QTcpSocket *m_shelfSocketLeft;
    QTcpSocket *m_shelfSocketRight;

    QList<int> m_activeRacks;             // 【修改】记录当前亮起的货位混合地址

    QTimer *m_pollTimer;
    QByteArray m_bufferLeft;
    QByteArray m_bufferRight;
    QMap<int, int> m_targetQuantities;    // 【修改】记录每个混合地址预期剩余的数量
    int m_pollIndex = 0;

    //================= 摄像头相关变量 =================
    QCamera *m_camera = nullptr;
    QMediaCaptureSession *m_captureSession = nullptr;
    QVideoWidget *m_cameraWidget = nullptr;

    VisionProcessor *m_visionProcessor = nullptr;

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

signals:
    void signalBackHome();
};

#endif // ASSEMBLYDIALOG_H