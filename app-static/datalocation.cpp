// 文件说明：app-static\datalocation.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "datalocation.h"

#include <QDir>
#include <QStandardPaths>


// 函数说明：实现 DataLocation::writableLocation 的核心逻辑，供当前模块调用。
QString DataLocation::writableLocation()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    ensurePathExists(path);
    return path;
}

// 函数说明：实现 DataLocation::standardLocations 的核心逻辑，供当前模块调用。
QStringList DataLocation::standardLocations()
{
    return QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
}

// 函数说明：实现 DataLocation::ensurePathExists 的核心逻辑，供当前模块调用。
void DataLocation::ensurePathExists(const QString &path)
{
    QDir p(path);
    if (!p.exists()) {
        p.mkpath(path);
    }
}

