#include "ProjectorWindow.h"
#include <QPixmap>
#include <QGuiApplication>
#include <QScreen>
#include <QUrl>

ProjectorWindow::ProjectorWindow(QWidget *parent)
    : QWidget(parent)
{
    this->setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    this->setStyleSheet("background-color: black;");

    m_label = new QLabel(this);
    m_label->setAlignment(Qt::AlignCenter);

    // ======== 新增视频播放器初始化 ========
    m_projVideoWidget = new QVideoWidget(this);
    m_projPlayer = new QMediaPlayer(this);
    m_projPlayer->setVideoOutput(m_projVideoWidget);
    // 在 Qt6 中，如果不绑定 QAudioOutput，播放视频将天然静音，完美契合投影仪只要画面的需求！

    m_projVideoWidget->hide(); // 默认隐藏
}

ProjectorWindow::~ProjectorWindow() { }

void ProjectorWindow::showImage(const QString &path, QRect screenGeometry)
{
    stopVideo();
    if (path.isEmpty()) { this->hide(); return; }

    // 1. 移动并改变窗口大小（因为是无边框，这就等于完美全屏）
    this->setGeometry(screenGeometry);

    QPixmap pix(path);
    if (!pix.isNull()) {
        m_label->setPixmap(pix.scaled(screenGeometry.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        // 确保图片控件也铺满整个窗口
        m_label->setGeometry(0, 0, screenGeometry.width(), screenGeometry.height());

        m_projVideoWidget->hide();
        m_label->show();

        // 2. 正常显示即可，【千万不要调用 this->showFullScreen();】
        this->show();
    } else {
        this->hide();
    }
}

// ======== 新增视频控制具体实现 ========
void ProjectorWindow::playAnimationVideo(const QString &videoPath, QRect screenGeometry)
{
    stopVideo();
    if (videoPath.isEmpty()) { this->hide(); return; }

    // 1. 移动并撑满投影仪屏幕
    this->setGeometry(screenGeometry);
    m_label->hide();

    // 2. 确保视频播放组件也铺满整个窗口
    m_projVideoWidget->setGeometry(0, 0, screenGeometry.width(), screenGeometry.height());
    m_projVideoWidget->show();

    // 3. 正常显示即可，【千万不要调用 this->showFullScreen();】
    this->show();

    m_projPlayer->setSource(QUrl::fromLocalFile(videoPath));
    m_projPlayer->setLoops(-1); // Qt6 中设置 -1 表示无限循环播放
    m_projPlayer->play();
}

void ProjectorWindow::stopVideo() {
    if (m_projPlayer) m_projPlayer->stop();
}
void ProjectorWindow::pauseVideo() {
    if (m_projPlayer && m_projPlayer->playbackState() == QMediaPlayer::PlayingState)
        m_projPlayer->pause();
}
void ProjectorWindow::resumeVideo() {
    if (m_projPlayer && m_projPlayer->playbackState() == QMediaPlayer::PausedState)
        m_projPlayer->play();
}