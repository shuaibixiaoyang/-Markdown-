// 文件说明：app-static\completionlistmodel.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "completionlistmodel.h"

#include <QFont>
#include <QIcon>

//Qt 代码补全列表模型，
//构造函数
CompletionListModel::CompletionListModel(QObject *parent) :
    QAbstractListModel(parent)
{
}

//返回列表总行数
//行数 = 代码片段数量+普通单词数量
int CompletionListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return snippets.count() + words.count();
}

//获取列表项数据
QVariant CompletionListModel::data(const QModelIndex &index, int role) const
{
    //索引无效直接返回空
    if (!index.isValid())
        return QVariant();
 // 行号超出范围返回空
    if (index.row() > rowCount())
        return QVariant();

    if (index.row() < snippets.count()) {
        const Snippet snippet = snippets.at(index.row());

        switch (role) {
            //设置图标
            case Qt::DecorationRole:
                return QIcon("fa-puzzle-piece.fontawesome");
            //显示文本
            case Qt::DisplayRole:
                return QString("%1 %2").arg(snippet.trigger, -15).arg(snippet.description);
             // 编辑时使用触发器文本
            case Qt::EditRole:
                return snippet.trigger;
            // 鼠标悬浮提示：显示片段原文
            case Qt::ToolTipRole:
                return snippet.snippet.toHtmlEscaped();
           // 设置等宽字体，方便阅读
            case Qt::FontRole:
                {
                    QFont font("Monospace", 8);
                    font.setStyleHint(QFont::TypeWriter);
                    return font;
                }
                 break;
        }
    }
     // 后半部分：普通单词项
    else {
        switch (role) {
            case Qt::DisplayRole:
            case Qt::EditRole:
                return words.at(index.row() - snippets.count());
        }
    }

    return QVariant();
}

// 设置普通单词列表
void CompletionListModel::setWords(const QStringList &words)
{
    //通知视图开始更新数据
    beginInsertRows(QModelIndex(), snippets.count(), snippets.count() + words.count());
    this->words = words;
    //通知试图数据更新完成
    endInsertRows();
}
// 响应片段集合变化：新增、修改、删除
void CompletionListModel::snippetCollectionChanged(SnippetCollection::CollectionChangedType changedType, const Snippet &snippet)
{
    switch (changedType) {
        //新增片段
    case SnippetCollection::ItemAdded:
        {
        //找到插入位置，保持有序
            QList<Snippet>::iterator it = std::lower_bound(snippets.begin(), snippets.end(), snippet);
            int row = std::distance(snippets.begin(), it);
            beginInsertRows(QModelIndex(), row, row);
            snippets.insert(it, snippet);
            endInsertRows();
        }
        break;
        //修改片段
    case SnippetCollection::ItemChanged:
        {
            int row = snippets.indexOf(snippet);
            snippets.replace(row, snippet);
        }
        break;
        //删除片段
    case SnippetCollection::ItemDeleted:
        {
            int row = snippets.indexOf(snippet);
            beginRemoveRows(QModelIndex(), row, row);
            snippets.removeAt(row);
            endRemoveRows();
        }
        break;
    default:
        break;
    }
}

