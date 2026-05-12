#include "mainwindow.h"
#include "qdir.h"
#include "qt_file_logger.h"

#include <QApplication>
#include <QTextStream>

#include <QScreen>
#include <QDebug>
#include <QGuiApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    qDebug() << "---- 1. 程序启动，QApplication 初始化完成 ----";

    installQtFileLogger();
    QFile qssFile(":/qss/default.qss");
    if (qssFile.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream in(&qssFile);
        qApp->setStyleSheet(in.readAll());
        qssFile.close();
    }

    qDebug() << "---- 2. QSS 样式表加载完成，准备创建 MainWindow ----";

    MainWindow w;

    qDebug() << "---- 3. MainWindow 创建完成，准备调用 show() ----";

    // 强制给一个初始大小，防止 ui 文件里的布局坍塌导致尺寸为0
    w.resize(1280, 720);
    w.show();

    qDebug() << "---- 4. 窗口已 show()，进入事件循环 ----";

    return a.exec();
}
