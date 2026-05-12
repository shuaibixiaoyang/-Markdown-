// 文件说明：app-static\datalocation.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef DATALOCATION_H
#define DATALOCATION_H

#include <QStringList>


class DataLocation
{
public:
    static QString writableLocation();
    static QStringList standardLocations();

private:
    DataLocation();
    ~DataLocation();

    static void ensurePathExists(const QString &path);
};

#endif // DATALOCATION_H

