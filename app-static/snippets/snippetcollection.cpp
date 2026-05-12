// 文件说明：app-static\snippets\snippetcollection.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "snippetcollection.h"

#include <QSharedPointer>


// 函数说明：构造 SnippetCollection 对象，初始化本模块需要的状态、界面和资源。
SnippetCollection::SnippetCollection(QObject *parent) :
    QObject(parent)
{
}

// 函数说明：实现 SnippetCollection::count 的核心逻辑，供当前模块调用。
int SnippetCollection::count() const
{
    return snippets.count();
}

// 函数说明：实现 SnippetCollection::insert 的核心逻辑，供当前模块调用。
int SnippetCollection::insert(const Snippet &snippet)
{
    QMap<QString, Snippet>::iterator it = snippets.insert(snippet.trigger, snippet);
    emit collectionChanged(SnippetCollection::ItemAdded, snippet);
    return std::distance(snippets.begin(), it);
}

// 函数说明：刷新 SnippetCollection 的内部状态，并同步到相关界面。
void SnippetCollection::update(const Snippet &snippet)
{
    snippets.insert(snippet.trigger, snippet);
    emit collectionChanged(SnippetCollection::ItemChanged, snippet);
}

// 函数说明：从 SnippetCollection 管理的数据集合中移除指定内容。
void SnippetCollection::remove(const Snippet &snippet)
{
    snippets.remove(snippet.trigger);
    emit collectionChanged(SnippetCollection::ItemDeleted, snippet);
}

// 函数说明：实现 SnippetCollection::name 的核心逻辑，供当前模块调用。
const QString SnippetCollection::name() const
{
    return QStringLiteral("snippets");
}

// 函数说明：实现 SnippetCollection::contains 的核心逻辑，供当前模块调用。
bool SnippetCollection::contains(const QString &trigger) const
{
    return snippets.contains(trigger);
}

// 函数说明：实现 SnippetCollection::snippet 的核心逻辑，供当前模块调用。
const Snippet SnippetCollection::snippet(const QString &trigger) const
{
    return snippets.value(trigger);
}

const Snippet &SnippetCollection::at(int offset) const
{
    return (snippets.begin() + offset).value();
}

// 函数说明：实现 SnippetCollection::userDefinedSnippets 的核心逻辑，供当前模块调用。
QSharedPointer<SnippetCollection> SnippetCollection::userDefinedSnippets() const
{
    QSharedPointer<SnippetCollection> userDefinedSnippets = QSharedPointer<SnippetCollection>::create();

    for (const Snippet &snippet : snippets.values()) {
        if (!snippet.builtIn) {
            userDefinedSnippets->insert(snippet);
        }
    }

    return userDefinedSnippets;
}

