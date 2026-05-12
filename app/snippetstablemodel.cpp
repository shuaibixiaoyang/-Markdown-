// 文件说明：app\snippetstablemodel.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "snippetstablemodel.h"

#include <QMessageBox>

#include <snippets/snippetcollection.h>

//构造函数：初始化父类和数据集合
SnippetsTableModel::SnippetsTableModel(SnippetCollection *collection, QObject *parent) :
    QAbstractTableModel(parent),
    snippetCollection(collection)
{
}
// 返回表格的行数，行数等于数据集合中的片段数量
int SnippetsTableModel::rowCount(const QModelIndex &) const
{
    return snippetCollection->count();
}
// 返回表格的列数，固定为2列：Trigger 和 Description
int SnippetsTableModel::columnCount(const QModelIndex &) const
{
    return 2;
}
// 设置单元格的交互标志
// 内置片段不可编辑，用户自定义片段可编辑
Qt::ItemFlags SnippetsTableModel::flags(const QModelIndex &index) const
{
    //获取默认的单元格标志
    Qt::ItemFlags itemFlags = QAbstractTableModel::flags(index);
    //如果索引有校
    if (index.isValid()) {
        //获取当前行对应的片段数据
        const Snippet snippet = snippetCollection->at(index.row());
        //如果不是内置片段，则添加可编辑标志
        if (!snippet.builtIn) {
            itemFlags |= Qt::ItemIsEditable;
        }
    }
    //返回单元格标志
    return itemFlags;
}

// 获取单元格显示或编辑的数据
QVariant SnippetsTableModel::data(const QModelIndex &index, int role) const
{
    //索引无效
    if (!index.isValid()) {
        return QVariant();
    }
  // 获取当前行对应的片段
    const Snippet snippet = snippetCollection->at(index.row());
    // 显示角色：界面展示的数据
    if (role == Qt::DisplayRole) {
        if (index.column() == 0) {
            return snippet.trigger;//触发器
        } else {
            return snippet.description + (snippet.builtIn ? " (built-in)" : "");//描述
        }
    }
    //双击角色，双击编辑时显示的数据
    if (role == Qt::EditRole) {
        if (index.column() == 0) {
            return snippet.trigger;
        } else {
            return snippet.description;
        }
    }
    //其他角色返回空
    return QVariant();
}
// 设置单元格数据，编辑完成后保存数据
bool SnippetsTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{    // 索引无效或不是编辑角色，直接返回失败
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    Snippet snippet = snippetCollection->at(index.row());

    if (index.column() == 0) {
        const QString &s = value.toString();//修改触发器
        //检验触发器是否合法
        if (!isValidTrigger(s)) {
            QMessageBox::critical(nullptr, tr("Error", "Title of error message box"), tr("Not a valid trigger."));
            if (snippet.trigger.isEmpty())//触发器为空
                removeSnippet(index);
            return false;
        }
        snippet.trigger = s;//合法
    } else {
        snippet.description = value.toString();//修改描述
    }
    // 替换数据集合中的片段
    replaceSnippet(snippet, index);
    return true;
}
// 设置表头显示文字
QVariant SnippetsTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{    // 只处理水平方向的显示角色
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QVariant();

    if (section == 0)
         return tr("Trigger");
     else
        return tr("Description");
}
// 创建一个新的空片段，插入到第一行
QModelIndex SnippetsTableModel::createSnippet()
{
    Snippet snippet;
        // 通知视图开始插入行
    beginInsertRows(QModelIndex(), 0, 0);
        // 将新片段插入数据集合
    int row = snippetCollection->insert(snippet);

    // 通知视图插入完成
    endInsertRows();
    // 返回新行的索引
    return index(row, 0);
}

// 删除指定索引对应的片段
void SnippetsTableModel::removeSnippet(const QModelIndex &index)
{
        // 通知视图开始删除行
    beginRemoveRows(QModelIndex(), index.row(), index.row());
            // 获取要删除的片段
    Snippet snippet = snippetCollection->at(index.row());
        // 从数据集合中删除
    snippetCollection->remove(snippet);
        // 通知视图删除完成
    endRemoveRows();
}
// 替换指定索引的片段数据，并处理数据变化或位置移动
void SnippetsTableModel::replaceSnippet(const Snippet &snippet, const QModelIndex &index)
{
    const int row = index.row();

    // 获取修改前的旧数据
    Snippet previousSnippet = snippetCollection->at(index.row());
      // 先删除旧数据
    snippetCollection->remove(previousSnippet);
    //    // 插入新数据，得到新的行号
    int insertedRow = snippetCollection->insert(snippet);
    //如果行号没有变化
    if (index.row() == insertedRow) {
        if (index.column() == 0)
            emit dataChanged(index, index.sibling(row, 1));
        else
            emit dataChanged(index.sibling(row, 0), index);
    } else {
        if (row < insertedRow)
            beginMoveRows(QModelIndex(), row, row, QModelIndex(), insertedRow+1);
        else
            beginMoveRows(QModelIndex(), row, row, QModelIndex(), insertedRow);
        endMoveRows();
    }
}

//检验触发器是否合法
bool SnippetsTableModel::isValidTrigger(const QString &trigger)
{
    //不为空
    if (trigger.isEmpty())
        return false;
    //不能已存在
    if (snippetCollection->contains(trigger))
        return false;
   //不能包含空格
    for (int i = 0; i < trigger.length(); ++i) {
        if (trigger.at(i).isSpace()) {
            return false;
        }
    }
   //合法
    return true;
}


