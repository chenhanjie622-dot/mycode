#ifndef SHELFDISPLAYDIALOG_H
#define SHELFDISPLAYDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QFile>
#include <QFileDialog>


struct PartImportData {
    QString cellId;     // 货位号 A-1-1
    QString name;       // 名称
    QString spec;       // 规格
    QString code;       // 编码
    int quantity;       // 数量
};

namespace Ui {
class ShelfDisplayDialog;
}

class ShelfDisplayDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ShelfDisplayDialog(QWidget *parent = nullptr);
    ~ShelfDisplayDialog();

    void InitUI();

signals:
    void signalBackHome();
    void realTimeQuantityReceived(int combinedAddr, int qty);

private slots:
    void on_btn_shelfdlg_back_clicked();
    void on_btn_shelfdlg_refresh_clicked();
    void onShelfClicked(QString id);
    void on_btn_shelfdlg_configure_clicked();

    // 【修改】双通道网络槽函数
    void onNewConnectionLeft();
    void onNewConnectionRight();
    void onClientDisconnectedLeft();
    void onClientDisconnectedRight();

    void onReadyReadLeft();
    void onReadyReadRight();
    void processReadyRead(QTcpSocket* socket, QByteArray& buffer, bool isRight);

    void onPollTimeout();

    void on_btn_import_parts_clicked();
    void onImportTimerTimeout();

protected:
    void hideEvent(QHideEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void clearLayout();

    quint16 calculateCRC16(const QByteArray &data);

    QList<int> m_scanQueue;
    QString getCellIdByAddress(int combinedAddr);

    Ui::ShelfDisplayDialog *ui;
    bool m_isBackSide = false;

    // ================= 网络通讯变量(双通道) =================
    QTcpServer *tcpServerLeft;
    QTcpServer *tcpServerRight;
    QTcpSocket *shelfSocketLeft = nullptr;
    QTcpSocket *shelfSocketRight = nullptr;

    QTimer *pollTimer;
    QByteArray m_bufferLeft;
    QByteArray m_bufferRight;

    int m_currentPollingAddress = 0;

    void sendStringCommand(int combinedAddr, quint16 reg, const QString &text, int maxBytes);
    void sendInt32Command(int combinedAddr, quint16 reg, int value);

    QList<PartImportData> m_importQueue;
    QTimer* m_importTimer;
    int m_currentImportStep = 0;
};

#endif // SHELFDISPLAYDIALOG_H