// 文件说明：app-static\themes\themecollection.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "themecollection.h"

#include <jsonfile.h>
#include <themes/jsonthemetranslatorfactory.h>

// 函数说明：加载 ThemeCollection 需要的数据、配置或外部资源。
bool ThemeCollection::load(const QString &fileName)
{
    return JsonFile<Theme>::load(fileName, this);
}

// 函数说明：实现 ThemeCollection::insert 的核心逻辑，供当前模块调用。
int ThemeCollection::insert(const Theme &theme)
{
    themesIndex << theme.name();
    themes << theme;
    return 0;
}

// 函数说明：实现 ThemeCollection::count 的核心逻辑，供当前模块调用。
int ThemeCollection::count() const
{
    return themes.count();
}

const Theme &ThemeCollection::at(int offset) const
{
    return themes.at(offset);
}

// 函数说明：实现 ThemeCollection::contains 的核心逻辑，供当前模块调用。
bool ThemeCollection::contains(const QString &name) const
{
    return themesIndex.contains(name);
}

// 函数说明：实现 ThemeCollection::theme 的核心逻辑，供当前模块调用。
const Theme ThemeCollection::theme(const QString &name) const
{
    return at(themesIndex.indexOf(name));
}

// 函数说明：实现 ThemeCollection::themeNames 的核心逻辑，供当前模块调用。
QStringList ThemeCollection::themeNames() const
{
    return themesIndex;
}

// 函数说明：实现 ThemeCollection::name 的核心逻辑，供当前模块调用。
const QString ThemeCollection::name() const
{
    return QStringLiteral("themes");
}

