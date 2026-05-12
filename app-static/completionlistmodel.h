// 文件说明：app-static\completionlistmodel.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef COMPLETIONLISTMODEL_H
#define COMPLETIONLISTMODEL_H

#include <QAbstractListModel>
#include <snippets/snippetcollection.h>
struct Snippet;

//Qt 代码补全列表模型，
class CompletionListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    //构造函数
    explicit CompletionListModel(QObject *parent = nullptr);
    //统计行数
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    //获取指定索引的数据
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    //设置需要显示的补全单词列表
    void setWords(const QStringList &words);

public slots:
    //代码片段库发生变化时的槽函数
    void snippetCollectionChanged(SnippetCollection::CollectionChangedType changedType, const Snippet &snippet);

private:
    QList<Snippet> snippets;//存储代码片段数据
    QStringList words;//存储用于显示的补全单词
};

#endif // COMPLETIONLISTMODEL_H

