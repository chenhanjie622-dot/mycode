#ifndef DATATYPES_H
#define DATATYPES_H

#include <QString>

struct UserInfo
{
    int id;             // 数据库id
    QString userId;     // 用户id备用，可能联网用
    QString username;   // 用户名，登录用。
    QString name;       // 用户名称，显示用。
    QString password;   // 用户密码，验证用。
    QString roleType;   // 用户权限，类型等。
    QString telephone;  // 用户电话
    QString createTime; // 创建时间
    QString modifyTime; // 修改时间
    QString reserve1;   // 备用
    QString reserve2;
    QString reserve3;
    UserInfo()
    {
        roleType = 0;
    }
};

struct StepInfo
{
    int id;             // 数据库id
    QString stepname;   // 步骤名称
    QString steppath;   // 步骤地址
    QString itemname;   // 小步骤名称
    QString rackdata;   // 货架数据
    QString reserve1;   // 备用
    QString reserve2;
    QString reserve3;
};


#endif // DATATYPES_H
