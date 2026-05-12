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
    // ======== 直接在这里打印，绕开显卡渲染崩溃 ========
    qDebug() << "===== 屏幕真实排序检测 =====";
    QList<QScreen *> screens = QGuiApplication::screens();
    for(int i = 0; i < screens.size(); i++) {
        qDebug() << "索引" << i << " -> 名字:" << screens[i]->name() << " 分辨率:" << screens[i]->geometry();
    }
    qDebug() << "============================";
    // ================================================
    installQtFileLogger();
    QFile qssFile(":/qss/default.qss");
    if (qssFile.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream in(&qssFile);
        QString qss = in.readAll();
        qApp->setStyleSheet(qss);
        qssFile.close();
    }

    MainWindow w;
    w.show();
    return a.exec();
}
