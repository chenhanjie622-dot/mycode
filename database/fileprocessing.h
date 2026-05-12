#ifndef FILEPROCESSING_H
#define FILEPROCESSING_H

#include <QObject>
#include <QMap>

class FileProcessing : public QObject
{
    Q_OBJECT
public:
    explicit FileProcessing(QObject *parent = nullptr);

    static QString GetFilePath();

    static void FindLeafFolders(const QString &dirPath, int &key);

    static bool isBottomLevelDirectory(const QString &dirPath);

    static void processFilesInBottomLevelDirectory(const QString &dirPath, int &key);

    static QMap<int, QStringList> GetStepPath();

    static void StepPathClear();

private:
    static QMap<int, QStringList> m_stepPath;
};

#endif // FILEPROCESSING_H
