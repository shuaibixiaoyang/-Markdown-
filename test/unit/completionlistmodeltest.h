// 文件说明：test\unit\completionlistmodeltest.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef COMPLETIONLISTMODELTEST_H
#define COMPLETIONLISTMODELTEST_H

#include <QObject>

struct Snippet;
class CompletionListModel;


class CompletionListModelTest : public QObject
{
    Q_OBJECT
    
private slots:
    void acceptsNewSnippet();
    void updatesCorrectRowForSnippet();
    void removesCorrectRowForSnippet();
    void holdsSnippetsInTriggerOrder();

private:
    void assertItemMatchesSnippet(int row, const Snippet &snippet);
    QVariant itemValue(int row, int role);

private:
    CompletionListModel *model;
};

#endif // COMPLETIONLISTMODELTEST_H

