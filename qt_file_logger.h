#ifndef QT_FILE_LOGGER_H
#define QT_FILE_LOGGER_H

#include <QMessageLogContext>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QThread>
#include <QDir>
#include <QCoreApplication>
#include <QMutex>
#include <QMutexLocker>

#ifdef Q_OS_UNIX
#include <execinfo.h>
#include <unistd.h>
#include <signal.h>
#endif

static constexpr qint64 QT_CRASH_LOG_MAX_SIZE =30LL * 1024 * 1024; // 30MB
static QMutex mutex;

inline QFile& qtLogFile()
{
    static QFile file;
    static bool inited = false;

    if (!inited) {
        QString path = QCoreApplication::applicationDirPath()+ QDir::separator()+ "qt_msg.log";
        file.setFileName(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            qWarning() << "无法打开日志文件进行写入:" << file.errorString();
            // 可以根据情况决定是 return 还是做其他错误处理
        }
        inited = true;
    }

    return file;
}

inline void qtCheckLogSize(QFile& f)
{
    if (f.size() < QT_CRASH_LOG_MAX_SIZE)
        return;

    f.resize(0);
    f.seek(0);
}

inline void qtWriteLogLine(const QString& line)
{
    QMutexLocker locker(&mutex);
    QFile& f = qtLogFile();
    if (!f.isOpen())
        return;
    qtCheckLogSize(f);
    QTextStream ts(&f);
    ts << line << Qt::endl;
    ts.flush();
}


inline void qtWriteBacktrace()
{
#ifdef Q_OS_UNIX
    void* stack[64];
    int count = ::backtrace(stack, 64);
    char** symbols = ::backtrace_symbols(stack, count);

    if (!symbols)
        return;

    qtWriteLogLine("==== BACKTRACE BEGIN ====");
    for (int i = 0; i < count; ++i) {
        qtWriteLogLine(QString::fromLocal8Bit(symbols[i]));
    }
    qtWriteLogLine("==== BACKTRACE END ====");

    free(symbols);
#endif
}

inline void qtCloseLogFile()
{
    QFile& f = qtLogFile();
    f.close();
}

inline void qtFileMessageHandler(QtMsgType type, const QMessageLogContext& ctx, const QString& msg)
{
    const char* level = "UNKNOWN";
    switch (type) {
    case QtDebugMsg:    level = "DEBUG"; break;
    case QtWarningMsg:  level = "WARNING"; break;
    case QtCriticalMsg: level = "CRITICAL"; break;
    case QtFatalMsg:    level = "FATAL"; break;
    case QtInfoMsg:
        break;
    }

    QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString line = QString("[%1] [%2] %3 (%4:%5)").arg(time, level, msg, ctx.file).arg(ctx.line);

    // 同时输出到文件和控制台
    qtWriteLogLine(line);

    // 输出到控制台，这样可以在IDE中看到调试信息
    fprintf(stderr, "%s\n", line.toLocal8Bit().constData());
    fflush(stderr);

    if (type == QtFatalMsg) {
        qtWriteLogLine(QString("ThreadId=%1").arg(reinterpret_cast<quintptr>(QThread::currentThreadId())));
        qtWriteBacktrace();
        qtCloseLogFile();
#ifdef Q_OS_UNIX
        ::_exit(128 + SIGABRT);
#endif
    }
}


inline void installQtFileLogger()
{
    qInstallMessageHandler(qtFileMessageHandler);
}

inline void unInstallQtFileLogger()
{
    qInstallMessageHandler(nullptr);
}

#endif // QT_FILE_LOGGER_H
