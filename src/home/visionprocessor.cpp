#include "visionprocessor.h"
#include <QDebug>

VisionProcessor::VisionProcessor(QObject *parent)
    : QObject(parent), m_roi(50, 30, 260, 420) // 修改后的坐标，覆盖左侧区域
{
    // 这里的坐标基于 640x480 的标准分辨率：
    // x=50, y=30: 留出一点边距
    // width=260: 约占横向的一半
    // height=420: 覆盖大部分高度
}

void VisionProcessor::setROI(int x, int y, int w, int h) {
    m_roi = cv::Rect(x, y, w, h);
}

void VisionProcessor::processFrame(const QVideoFrame &frame) {
    if (!frame.isValid()) return;

    cv::Mat currentFrame = videoFrameToMat(frame);
    if (currentFrame.empty()) return;

    // 1. 裁剪 ROI 区域并转为灰度图
    cv::Mat roiFrame = currentFrame(m_roi);
    cv::Mat gray;
    cv::cvtColor(roiFrame, gray, cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(gray, gray, cv::Size(21, 21), 0);

    // 2. 初始化背景（前 10 帧作为基准）
    if (m_frameCount < 10) {
        m_background = gray.clone();
        m_frameCount++;
        return;
    }

    // 3. 计算当前帧与背景的差异
    cv::Mat frameDelta, thresh;
    cv::absdiff(m_background, gray, frameDelta);
    cv::threshold(frameDelta, thresh, 25, 255, cv::THRESH_BINARY);

    // 4. 计算差异像素占比
    // 提高误触发门槛：将 0.20 改为 0.40
    double changeRatio = (double)cv::countNonZero(thresh) / (m_roi.width * m_roi.height);
    if (changeRatio > 0.40) {
        if (!m_isTriggered) {
            m_isTriggered = true;
            emit handDetected();
        }
    } else {
        m_isTriggered = false;
        // 加快背景学习速度，适应光线变化
        cv::addWeighted(m_background, 0.85, gray, 0.15, 0, m_background);
    }
}

cv::Mat VisionProcessor::videoFrameToMat(const QVideoFrame &frame) {
    QVideoFrame cloneFrame(frame);
    if (!cloneFrame.map(QVideoFrame::ReadOnly)) return cv::Mat();

    // 根据格式转换，这里假设是标准的 RGB/RGBA
    QImage img = cloneFrame.toImage().convertToFormat(QImage::Format_RGB888);
    cv::Mat mat(img.height(), img.width(), CV_8UC3, (void*)img.bits(), img.bytesPerLine());

    cv::Mat result;
    cv::cvtColor(mat, result, cv::COLOR_RGB2BGR);
    cloneFrame.unmap();
    return result.clone();
}