#include "shelfdatabase.h"

bool ShelfDatabase::initDB() {
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("shelf_data.db");

    if (!db.open()) {
        qDebug() << "Error: connection with database failed";
        return false;
    }

    QSqlQuery query;
    return query.exec("CREATE TABLE IF NOT EXISTS ShelfTable ("
                      "id TEXT PRIMARY KEY, "
                      "name TEXT, "
                      "spec TEXT, "
                      "code TEXT, "
                      "count INTEGER)");
}

bool ShelfDatabase::updateItem(QString id, QString name, QString spec, QString code, int count) {
    QSqlQuery query;
    query.prepare("INSERT OR REPLACE INTO ShelfTable (id, name, spec, code, count) "
                  "VALUES (:id, :name, :spec, :code, :count)");
    query.bindValue(":id", id);
    query.bindValue(":name", name);
    query.bindValue(":spec", spec);
    query.bindValue(":code", code);
    query.bindValue(":count", count);
    return query.exec();
}

ShelfData ShelfDatabase::getItem(QString id) {
    QSqlQuery query;
    query.prepare("SELECT name, count, spec, code FROM ShelfTable WHERE id = :id");
    query.bindValue(":id", id);

    ShelfData data;
    data.id = id;
    if (query.exec() && query.next()) {
        data.name = query.value(0).toString();
        data.count = query.value(1).toInt();
        data.spec = query.value(2).toString(); // 取出规格
        data.code = query.value(3).toString(); // 取出编码
    } else {
        data.name = "空";
        data.count = 0;
        data.spec = "";
        data.code = "";
    }
    return data;
}
