// 文件说明：app-static\themes\themecollection.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef THEMECOLLECTION_H
#define THEMECOLLECTION_H

#include <QString>
#include <QStringList>
#include <jsoncollection.h>
#include "theme.h"


class ThemeCollection : public JsonCollection<Theme>
{
public:
    bool load(const QString &fileName);

    int insert(const Theme &theme);

    int count() const;
    const Theme &at(int offset) const;
    bool contains(const QString &name) const;
    const Theme theme(const QString &name) const;
    QStringList themeNames() const;

    const QString name() const;

private:
    QStringList themesIndex;
    QList<Theme> themes;
};

#endif // THEMECOLLECTION_H


