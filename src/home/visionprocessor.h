#ifndef VISIONPROCESSOR_H
#define VISIONPROCESSOR_H

#include <QObject>
#include <QVideoFrame>
#include <QImage>
#include <opencv2/opencv.hpp>

class VisionProcessor : public QObject
{
    Q_OBJECT
public:
    explicit VisionProcessor(QObject *parent = nullptr);

    // 设置检测区域 (x, y, width, height)
    void setROI(int x, int y, int w, int h);

public slots:
    // 接收来自 QVideoSink 的原始帧并处理
    void processFrame(const QVideoFrame &frame);

signals:
    // 当检测到手部进入区域时发出此信号
    void handDetected();

private:
    cv::Rect m_roi;
    cv::Mat m_background; // 背景模型，用于对比
    bool m_isTriggered = false;
    int m_frameCount = 0;

    // 将 Qt 视频帧转为 OpenCV 矩阵
    cv::Mat videoFrameToMat(const QVideoFrame &frame);
};

#endif // VISIONPROCESSOR_H
