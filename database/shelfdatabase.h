#ifndef SHELFDATABASE_H
#define SHELFDATABASE_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

// 定义一个结构体来方便传递数据
struct ShelfData {
    QString id;
    QString name;
    int count;
    QString spec;  // 新增：规格
    QString code;  // 新增：编码
};

class ShelfDatabase {
public:
    // 单例模式或静态方法，方便全局调用
    static ShelfDatabase& getInstance() {
        static ShelfDatabase instance;
        return instance;
    }

    bool initDB();                                      // 初始化数据库和表
    bool updateItem(QString id, QString name, QString spec, QString code, int count); // 更新/插入数据
    ShelfData getItem(QString id);                     // 获取单个格口数据

private:
    ShelfDatabase() {} // 构造函数私有化
    QSqlDatabase db;
};

#endif // SHELFDATABASE_H
