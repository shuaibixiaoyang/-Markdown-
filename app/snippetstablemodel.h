// 文件说明：app\snippetstablemodel.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef SNIPPETSTABLEMODEL_H
#define SNIPPETSTABLEMODEL_H

#include <QAbstractTableModel>

//这是一个完整的 Qt 自定义 TableModel 头文件，专门用来管理代码片段


// 前置声明：告诉编译器 Snippet 和 SnippetCollection 是存在的类型
// 避免头文件循环依赖，具体定义放在 cpp 文件中引入
struct Snippet;
class SnippetCollection;
//继承自 QAbstractTableModel，用于为 QTableView 提供数据接口
class SnippetsTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    SnippetsTableModel(SnippetCollection *collection, QObject *parent);
    ~SnippetsTableModel() {}
    //获取行数
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    //获取列数
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    //获取单元格的交互标志
    Qt::ItemFlags flags(const QModelIndex &index) const;
    //数据
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    //设置数据
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    //头部数据
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    //创建一条新的代码片段
    QModelIndex createSnippet();
    //删除指定索引的代码片段
    void removeSnippet(const QModelIndex &index);

private:
    //替换指定索引位置的代码片段数据
    void replaceSnippet(const Snippet &snippet, const QModelIndex &index);
    //校验触发器字符串是否合法
    bool isValidTrigger(const QString &trigger);

private:
    // 代码片段数据源（指针持有，不负责释放）
    SnippetCollection *snippetCollection;
};

#endif // SNIPPETSTABLEMODEL_H

