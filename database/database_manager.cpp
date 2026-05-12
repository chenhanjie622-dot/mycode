#include "database_manager.h"
#include "qsqlquery.h"

DatabaseManager* DatabaseManager::m_instance = nullptr;
QMutex DatabaseManager::m_mutex;

DatabaseManager::DatabaseManager(QObject *parent) : QObject(parent)
{
    // 私有构造，禁止外部实例化
}

DatabaseManager::~DatabaseManager()
{
    if (m_database.isOpen()) {
        m_database.close();
    }
}

DatabaseManager& DatabaseManager::instance()
{
    if (!m_instance) {
        m_instance = new DatabaseManager();
    }
    return *m_instance;
}

void DatabaseManager::initialize()
{
    // 1. 创建数据库目录
    if (!createDatabaseDirectory()) {
        qWarning() << "Failed to create database directory!";
        emit databaseInitialized(false);
        return ;
    }

    // 2. 设置数据库路径（进程目录下的 database/database.db）
    QString dbPath = QDir::currentPath() + "/database/database.db";
    m_database = QSqlDatabase::addDatabase("QSQLITE", "AppDatabaseConnection");
    m_database.setDatabaseName(dbPath);

    // 3. 检查数据库是否存在（若不存在则自动创建）
    bool dbExists = checkDatabaseExists();
    if (!dbExists) {
        qDebug() << "Database not found, creating new one at:" << dbPath;
    }

    // 4. 打开数据库
    if (!m_database.open()) {
        qCritical() << "Database open error:" << m_database.lastError().text();
        emit databaseInitialized(false);
        return ;
    }

    emit databaseInitialized(true);
}

void DatabaseManager::initializetable()
{
    bool ret = false;
    createSubCategoryTable(); // <--- 新增这一行：确保启动时创建好子目录表
    if(!createDataTable("user") && createUserdataTable())
    {
        UserInfo info;
        info.userId= "null";
        info.username = "admin";
        info.name = "test";
        info.password = "admin";
        info.roleType = "0";
        info.telephone = "110";
        info.createTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        ret = AddUserInfo(info);
    }

    if(!createDataTable("step") && createStepdataTable())
    {
        QStringList items = {"PCR安装", "大型一体机安装", "桌面一体机安装", "手持设备安装", "其他设备安装"};
        foreach (QString var, items) {
            StepInfo info;
            info.stepname = var;
            m_database.transaction();
            ret = AddStepInfo(info);
            if(!ret)
                m_database.rollback();
        }

        if(ret)
            m_database.commit();
    }

    emit databaseInitializedTable(ret);
}

bool DatabaseManager::createDatabaseDirectory()
{
    QDir dir(QDir::currentPath());
    if (!dir.exists("database")) {
        return dir.mkdir("database");
    }
    return true;
}

bool DatabaseManager::checkDatabaseExists()
{
    QString dbPath = QDir::currentPath() + "/database/database.db";
    return QFile::exists(dbPath);
}

QSqlDatabase DatabaseManager::database() const
{
    return m_database;
}

bool DatabaseManager::queryUserInfo(UserInfo &info)
{
    QMutexLocker locker(&m_mutex);
    bool ret = false;
    QSqlQuery sqlQuery(m_database);
    QString  sql = QString("SELECT * FROM user where username='%1' ").arg(info.username);
    sqlQuery.prepare(sql);
    if (sqlQuery.exec() && sqlQuery.next())
    {
        info.id = sqlQuery.value("id").toInt();
        info.userId= sqlQuery.value("userId").toString();
        info.username = sqlQuery.value("username").toString();
        info.name = sqlQuery.value("name").toString();
        info.password = sqlQuery.value("password").toString();
        info.roleType = sqlQuery.value("roleType").toString();
        info.telephone = sqlQuery.value("telephone").toString();
        info.createTime = sqlQuery.value("createTime").toString();
        info.modifyTime = sqlQuery.value("modifyTime").toString();
        info.reserve1 = sqlQuery.value("reserve1").toString();
        info.reserve2 = sqlQuery.value("reserve2").toString();
        info.reserve3 = sqlQuery.value("reserve3").toString();
        ret = true;
    }
    return ret;
}

bool DatabaseManager::queryUserInfoAll(QVector<UserInfo> &infos)
{
    QMutexLocker locker(&m_mutex);
    QString  sql = "SELECT * FROM user ORDER BY createTime";
    QSqlQuery sqlQuery(m_database);
    sqlQuery.prepare(sql);
    infos.clear();
    if (!sqlQuery.exec()) {
        qDebug() << sqlQuery.lastError();
        return false;
    }
    while (sqlQuery.next())
    {
        UserInfo info;
        info.id = sqlQuery.value("id").toInt();
        info.userId= sqlQuery.value("userId").toString();
        info.username = sqlQuery.value("username").toString();
        info.name = sqlQuery.value("name").toString();
        info.password = sqlQuery.value("password").toString();
        info.roleType = sqlQuery.value("roleType").toString();
        info.telephone = sqlQuery.value("telephone").toString();
        info.createTime = sqlQuery.value("createTime").toString();
        info.modifyTime = sqlQuery.value("modifyTime").toString();
        info.reserve1 = sqlQuery.value("reserve1").toString();
        info.reserve2 = sqlQuery.value("reserve2").toString();
        info.reserve3 = sqlQuery.value("reserve3").toString();
        infos.push_back(info);
    }
    return infos.size() > 0;
}

bool DatabaseManager::AddUserInfo(UserInfo &info)
{
    QMutexLocker locker(&m_mutex);
    QSqlQuery query(m_database);
    query.prepare("INSERT INTO user (userId,username,name,password,"
                  "roleType,telephone,createTime,modifyTime,reserve1,reserve2,reserve3)"
                  " VALUES (:userId,:username,:name,:password,:roleType,:telephone,:createTime,:modifyTime,:reserve1,:reserve2,:reserve3)");
    query.bindValue(":userId",info.userId);
    query.bindValue(":username",info.username);
    query.bindValue(":name",info.name);
    query.bindValue(":password",info.password);
    query.bindValue(":roleType",info.roleType);
    query.bindValue(":telephone",info.telephone);
    query.bindValue(":createTime",info.createTime);
    query.bindValue(":modifyTime",info.modifyTime);
    query.bindValue(":reserve1",info.reserve1);
    query.bindValue(":reserve2",info.reserve2);
    query.bindValue(":reserve3",info.reserve3);

    if (!query.exec()) {
        qDebug() << "Error inserting user:" << query.lastError().text();
    }

    return query.lastError().type() == QSqlError::NoError;
}

bool DatabaseManager::DelUserInfo(int id)
{
    QMutexLocker locker(&m_mutex);
    QSqlQuery query(m_database);
    QString  sql = QString("DELETE FROM user where id=:id");
    query.prepare(sql);
    query.bindValue(":id",id);
    if (!query.exec()) {
        qDebug() << "Error delete user:" << query.lastError().text();
    }
    return query.lastError().type() == QSqlError::NoError;
}

bool DatabaseManager::UpdateUserInfo(const UserInfo &user)
{
    // 1. 检查数据库是否打开
    if(!m_database.isOpen()) {
        qDebug() << "[数据库错误] 更新用户失败：数据库未打开";
        return false;
    }

    // 2. 准备更新SQL
    QSqlQuery query(m_database);

    // 根据是否需要更新密码来构建不同的SQL语句
    QString sql;
    if (user.password != "") {  // 使用 != "" 而不是 isEmpty() 检查
        // 如果需要更新密码
        sql = "UPDATE user SET "
              "username = ?, "
              "name = ?, "
              "password = ?, "
              "roleType = ?, "
              "telephone = ?, "
              "modifyTime = ? "
              "WHERE id = ?";

        query.prepare(sql);
        query.addBindValue(user.username);
        query.addBindValue(user.name);
        query.addBindValue(user.password);  // 包含密码更新
        query.addBindValue(user.roleType);
        query.addBindValue(user.telephone);
        query.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")); // 最新修改时间
        query.addBindValue(user.id); // 主键，确定要更新的用户
    } else {
        // 如果不需要更新密码
        sql = "UPDATE user SET "
              "username = ?, "
              "name = ?, "
              "roleType = ?, "
              "telephone = ?, "
              "modifyTime = ? "
              "WHERE id = ?";

        query.prepare(sql);
        query.addBindValue(user.username);
        query.addBindValue(user.name);
        query.addBindValue(user.roleType);
        query.addBindValue(user.telephone);
        query.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")); // 最新修改时间
        query.addBindValue(user.id); // 主键，确定要更新的用户
    }

    // 4. 执行并检查结果
    if(!query.exec()) {
        qDebug() << "[数据库错误] 更新用户失败：" << query.lastError().text();
        return false;
    }

    // 5. 检查是否真的更新了数据（防止ID不存在）
    if(query.numRowsAffected() == 0) {
        qDebug() << "[数据库提示] 无用户被更新：ID=" << user.id << "的用户不存在";
        return false;
    }

    qDebug() << "[数据库成功] 更新用户ID=" << user.id << "成功";
    return true;
}

bool DatabaseManager::IsUsernameExists(const QString &username)
{
    QMutexLocker locker(&m_mutex);
    QSqlQuery query(m_database);
    QString sql = QString("SELECT COUNT(*) FROM user WHERE username = ?");
    query.prepare(sql);
    query.addBindValue(username);

    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }

    return false;
}

bool DatabaseManager::AddStepInfo(StepInfo &info)
{
    QMutexLocker locker(&m_mutex);
    QSqlQuery query(m_database);
    query.prepare("INSERT INTO step (stepname,steppath,rackdata,itemname,reserve1,reserve2,reserve3)"
                  " VALUES (:stepname,:steppath,:rackdata,:itemname,:reserve1,:reserve2,:reserve3)");
    query.bindValue(":stepname",info.stepname);
    query.bindValue(":steppath",info.steppath);
    query.bindValue(":itemname",info.itemname);
    query.bindValue(":rackdata",info.rackdata);
    query.bindValue(":reserve1",info.reserve1);
    query.bindValue(":reserve2",info.reserve2);
    query.bindValue(":reserve3",info.reserve3);

    if (!query.exec()) {
        qDebug() << "Error inserting step:" << query.lastError().text();
    }

    return query.lastError().type() == QSqlError::NoError;
}

bool DatabaseManager::UpdateStepInfo(const StepInfo &step)
{
    QMutexLocker locker(&m_mutex);
    QSqlQuery query(m_database);
    query.prepare("UPDATE step SET steppath=:steppath,rackdata=:rackdata,itemname=:itemname,"
                  "reserve1=:reserve1,reserve2=:reserve2,reserve3=:reserve3 where stepname=:stepname");
    query.bindValue(":stepname",step.stepname);
    query.bindValue(":steppath",step.steppath);
    query.bindValue(":itemname",step.itemname);
    query.bindValue(":rackdata",step.rackdata);
    query.bindValue(":reserve1",step.reserve1);
    query.bindValue(":reserve2",step.reserve2);
    query.bindValue(":reserve3",step.reserve3);

    if (!query.exec()) {
        qDebug() << "Error inserting step:" << query.lastError().text();
    }

    return query.lastError().type() == QSqlError::NoError;
}

bool DatabaseManager::queryStepInfo(StepInfo &info)
{
    QMutexLocker locker(&m_mutex);
    bool ret = false;
    QSqlQuery sqlQuery(m_database);
    QString  sql = QString("SELECT * FROM step where stepname='%1' ").arg(info.stepname);
    sqlQuery.prepare(sql);
    if (sqlQuery.exec() && sqlQuery.next())
    {
        info.id = sqlQuery.value("id").toInt();
        info.stepname = sqlQuery.value("stepname").toString();
        info.steppath = sqlQuery.value("steppath").toString();
        info.rackdata = sqlQuery.value("rackdata").toString();
        info.itemname = sqlQuery.value("itemname").toString();
        info.reserve1 = sqlQuery.value("reserve1").toString();
        info.reserve2 = sqlQuery.value("reserve2").toString();
        info.reserve3 = sqlQuery.value("reserve3").toString();
        ret = true;
    }
    return ret;
}

bool DatabaseManager::createDataTable(QString table)
{
    QMutexLocker locker(&m_mutex);
    QSqlQuery sqlQuery(m_database);
    sqlQuery.prepare("PRAGMA table_info('" + table +"')");
    sqlQuery.exec();
    if (sqlQuery.next()) {
        return true;
    }
    else {
        return false;
    }
}

bool DatabaseManager::createUserdataTable()
{
    QMutexLocker locker(&m_mutex);
    QSqlQuery sqlQuery(m_database);
    sqlQuery.prepare("CREATE TABLE user ("
                     "id         INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
                     "userId     TEXT,"
                     "username   TEXT NOT NULL,"
                     "name       TEXT NOT NULL,"
                     "password   TEXT NOT NULL,"
                     "roleType   TEXT NOT NULL,"
                     "telephone  TEXT,"
                     "createTime TEXT,"
                     "modifyTime TEXT,"
                     "reserve1   TEXT,"
                     "reserve2   TEXT,"
                     "reserve3   TEXT);");
    sqlQuery.exec();
    qDebug() << "user" << sqlQuery.lastError();
    return sqlQuery.lastError().type() == QSqlError::NoError;
}

bool DatabaseManager::createStepdataTable()
{
    QMutexLocker locker(&m_mutex);
    QSqlQuery sqlQuery(m_database);
    sqlQuery.prepare("CREATE TABLE step ("
                     "id         INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
                     "stepname   TEXT,"
                     "steppath   TEXT,"
                     "rackdata   TEXT,"
                     "itemname   TEXT,"
                     "reserve1   TEXT,"
                     "reserve2   TEXT,"
                     "reserve3   TEXT);");
    sqlQuery.exec();
    qDebug() << "step" << sqlQuery.lastError();
    return sqlQuery.lastError().type() == QSqlError::NoError;
}

// ======== 新增的子目录数据库操作实现 ========
bool DatabaseManager::createSubCategoryTable()
{
    QMutexLocker locker(&m_mutex);
    QSqlQuery sqlQuery(m_database);
    sqlQuery.prepare("CREATE TABLE IF NOT EXISTS sub_category ("
                     "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
                     "main_category TEXT NOT NULL,"
                     "sub_category TEXT NOT NULL);");
    bool ret = sqlQuery.exec();
    if (!ret) qDebug() << "create sub_category error:" << sqlQuery.lastError();
    return ret;
}

bool DatabaseManager::AddSubCategory(const QString& mainCat, const QString& subCat)
{
    QMutexLocker locker(&m_mutex);
    QSqlQuery query(m_database);
    query.prepare("INSERT INTO sub_category (main_category, sub_category) VALUES (?, ?)");
    query.addBindValue(mainCat);
    query.addBindValue(subCat);
    return query.exec();
}

bool DatabaseManager::DelSubCategory(const QString& mainCat, const QString& subCat)
{
    QMutexLocker locker(&m_mutex);
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM sub_category WHERE main_category = ? AND sub_category = ?");
    query.addBindValue(mainCat);
    query.addBindValue(subCat);
    return query.exec();
}

bool DatabaseManager::GetSubCategories(const QString& mainCat, QStringList& subCats)
{
    QMutexLocker locker(&m_mutex);
    subCats.clear();
    QSqlQuery query(m_database);
    query.prepare("SELECT sub_category FROM sub_category WHERE main_category = ?");
    query.addBindValue(mainCat);
    if (query.exec()) {
        while (query.next()) {
            subCats.append(query.value(0).toString());
        }
        return true;
    }
    return false;
}