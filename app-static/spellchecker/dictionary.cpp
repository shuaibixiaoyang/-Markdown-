// 文件说明：app-static\spellchecker\dictionary.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "dictionary.h"

#include <QLocale>

// 函数说明：构造 Dictionary 对象，初始化本模块需要的状态、界面和资源。
Dictionary::Dictionary()
{
}

// 函数说明：构造 Dictionary 对象，初始化本模块需要的状态、界面和资源。
Dictionary::Dictionary(const QString &language, const QString &filePath) :
    m_language(language),
    m_filePath(filePath)
{
}

// 函数说明：构造 Dictionary 对象，初始化本模块需要的状态、界面和资源。
Dictionary::Dictionary(const Dictionary &other)
{
    m_language = other.m_language;
    m_filePath = other.m_filePath;
}

// 函数说明：销毁 Dictionary 对象，释放本模块持有的资源。
Dictionary::~Dictionary()
{
}

// 函数说明：实现 Dictionary::language 的核心逻辑，供当前模块调用。
QString Dictionary::language() const
{
    return m_language;
}

// 函数说明：实现 Dictionary::languageName 的核心逻辑，供当前模块调用。
QString Dictionary::languageName() const
{
    return QLocale(m_language).nativeLanguageName();
}

// 函数说明：实现 Dictionary::countryName 的核心逻辑，供当前模块调用。
QString Dictionary::countryName() const
{
    return QLocale(m_language).nativeTerritoryName();
}

// 函数说明：处理主窗口的文件菜单动作，衔接文档读写和界面状态。
QString Dictionary::filePath() const
{
    return m_filePath;
}

