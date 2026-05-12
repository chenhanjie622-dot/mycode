#ifndef FILECONFIGURE_H
#define FILECONFIGURE_H

#include <QFile>
#include <QMutex>
#include <QDebug>
#include <QSettings>
#include <QCoreApplication>

#include <QDir>

class fileConfigure : public QObject
{
    Q_OBJECT
public:
    static fileConfigure& instance() {
        static QMutex mutex;
        QMutexLocker locker(&mutex);
        static fileConfigure singleton;
        return singleton;
    }

    QString getConfigPath()
    {
        return QDir::currentPath() + "/database/config.ini";
    }

    // 写入配置（模板方法支持任意类型）
    template<typename T>
    void write(const QString& key, const T& value) {
        QMutexLocker lock(&m_mutex);
        m_settings.setValue(key, QVariant::fromValue(value));
    }

    // 读取配置（带默认值）
    template<typename T>
    T read(const QString& key, const T& defaultValue = T()) {
        QMutexLocker lock(&m_mutex);
        return m_settings.value(key, defaultValue).template value<T>();
    }

    fileConfigure() :
        m_settings(getConfigPath(),
                   QSettings::IniFormat)
    {
        // 如果文件不存在，写入默认值
        if (!QFile::exists(getConfigPath()))
        {
            m_settings.setValue("user_use", "");
        }
        m_settings.sync();
    }

    ~fileConfigure() = default;

    QSettings m_settings;
    QMutex m_mutex;

};
#define g_fileConfigure fileConfigure::instance()

#endif // FILECONFIGURE_H
