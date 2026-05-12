#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H
#define g_DatabaseManager DatabaseManager::instance()

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QMutex>
#include <QDir>
#include <QVector>
#include <QDateTime>

#include "structs/datatypes.h"

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    static DatabaseManager& instance();  // 单例模式入口
    void initialize();                   // 初始化数据库连接
    void initializetable();              // 初始化数据库表
    QSqlDatabase database() const;       // 获取数据库对象

    // 事务批量操作.
    void transaction(){ m_database.transaction(); }
    void rollback(){ m_database.rollback(); }
    void commit(){ m_database.commit(); }

public:
    // 用户表
    bool queryUserInfo(UserInfo& info);               // 查询当前账号（按用户名）
    bool queryUserInfoAll(QVector<UserInfo>& infos);  // 查询所有
    bool AddUserInfo(UserInfo& info);                 // 新增
    bool DelUserInfo(int id);                         // 根据ID删除
    bool UpdateUserInfo(const UserInfo &user);        // 根据传入结构体更新
    bool IsUsernameExists(const QString &username);   // 检查用户名是否存在 - 新增

    // 操作步骤
    bool AddStepInfo(StepInfo& info);                 // 新增
    bool UpdateStepInfo(const StepInfo &step);        // 根据传入结构体更新
    bool queryStepInfo(StepInfo& info);               // 查询

    // 子目录操作
    bool createSubCategoryTable();                                  // 创建子目录表
    bool AddSubCategory(const QString& mainCat, const QString& subCat); // 新增子目录
    bool DelSubCategory(const QString& mainCat, const QString& subCat); // 删除子目录
    bool GetSubCategories(const QString& mainCat, QStringList& subCats);// 获取子目录列表

private:
    bool createDataTable(QString table);
    bool createUserdataTable();
    bool createStepdataTable();
    QSqlDatabase m_db;

signals:
    void databaseInitialized(bool success); // 数据库初始化完成信号
    void databaseInitializedTable(bool success); // 表初始化完成信号

private:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();
    QSqlDatabase m_database;
    static DatabaseManager* m_instance;

    bool createDatabaseDirectory();      // 创建数据库目录
    bool checkDatabaseExists();          // 检查数据库是否存在


    static QMutex m_mutex;
};

#endif // DATABASE_MANAGER_H
