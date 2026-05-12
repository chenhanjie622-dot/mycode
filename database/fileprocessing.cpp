#include "fileprocessing.h"

#include <QFileDialog>
#include <QDir>
#include <QDebug>
#include <QImageReader>
#include <QSettings>
#include <QFileDialog>
#include <QStandardPaths>

QMap<int, QStringList> FileProcessing::m_stepPath;

FileProcessing::FileProcessing(QObject *parent)
    : QObject{parent}
{

}

QString FileProcessing::GetFilePath()
{
    m_stepPath.clear();
    QSettings settings("MyCompany", "MyApp");
    QString lastPath = settings.value(
                                   "lastPath",
                                   QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)
                                   ).toString();
    QString filePath = QFileDialog::getExistingDirectory(
        nullptr,
        "选择文件夹",
        lastPath,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
        );

    if (filePath.isEmpty()) return QString();

    settings.setValue("lastPath", filePath);

    int key = 0;
    FindLeafFolders(filePath, key);

    return filePath;
}

void FileProcessing::FindLeafFolders(const QString &dirPath, int &key)
{
    QDir dir(dirPath);
    if (!dir.exists()) return;

    // 获取当前目录下的所有条目
    QFileInfoList entries = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries, QDir::DirsFirst);

    for (const QFileInfo &entry : std::as_const(entries)) {
        if (entry.isDir()) {
            // 如果是目录，递归调用
            FindLeafFolders(entry.absoluteFilePath(), key);
        } else {
            // 如果是最底层的文件夹，处理文件
            if (isBottomLevelDirectory(dirPath)) {
                processFilesInBottomLevelDirectory(dirPath, key);
                break; // 处理完最底层文件夹后退出
            }
        }
    }
}

bool FileProcessing::isBottomLevelDirectory(const QString &dirPath)
{
    QDir dir(dirPath);
    QFileInfoList entries = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries, QDir::DirsFirst);
    for (const QFileInfo &entry : std::as_const(entries)) {
        if (entry.isDir()) {
            return false; // 如果有子目录，则不是最底层
        }
    }
    return true;
}

void FileProcessing::processFilesInBottomLevelDirectory(const QString &dirPath, int &key)
{
    QDir dir(dirPath);
    QStringList filters;
    filters << "*.jpg" << "*.png" << "*.jpeg" << "*.mp4" << "*.avi"; // 添加更多文件类型
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);

    m_stepPath[key] << dir.dirName();
    // qDebug() << "Path:" << dir.dirName();

    QString imagePath = "";
    QString videoPath = "";

    // 先查找图片
    for (const QFileInfo &file : std::as_const(files)) {
        if (file.isFile()) {
            QString suffix = file.suffix().toLower();
            if (suffix == "jpg" || suffix == "png" || suffix == "jpeg") {
                imagePath = file.absoluteFilePath();
                // qDebug() << "Image: " << imagePath;
            }
        }
    }
    m_stepPath[key] << imagePath;

    // 再查找视频
    for (const QFileInfo &file : std::as_const(files)) {
        if (file.isFile()) {
            QString suffix = file.suffix().toLower();
            if (suffix == "mp4" || suffix == "avi") {
                videoPath = file.absoluteFilePath();
                // qDebug() << "Video: " << videoPath;
            }
        }
    }
    m_stepPath[key] << videoPath;

    key++;
}

QMap<int, QStringList> FileProcessing::GetStepPath()
{
    return m_stepPath;
}

void FileProcessing::StepPathClear()
{
    m_stepPath.clear();
}
