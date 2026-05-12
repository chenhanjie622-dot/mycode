#ifndef PROJECTORWINDOW_H
#define PROJECTORWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QMediaPlayer> // 新增
#include <QVideoWidget> // 新增

class ProjectorWindow : public QWidget
{
    Q_OBJECT
public:
    explicit ProjectorWindow(QWidget *parent = nullptr);
    ~ProjectorWindow();

    void showImage(const QString &path, QRect screenGeometry);

    // ======== 新增的视频播放控制 ========
    void playAnimationVideo(const QString &videoPath, QRect screenGeometry);
    void stopVideo();
    void pauseVideo();
    void resumeVideo();

private:
    QLabel *m_label;
    QMediaPlayer *m_projPlayer;      // 新增：投影仪独立播放器
    QVideoWidget *m_projVideoWidget; // 新增：投影仪独立视频画布
};

#endif // PROJECTORWINDOW_H