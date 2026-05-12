// 文件说明：app\snippetcompleter.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef SNIPPETCOMPLETER_H
#define SNIPPETCOMPLETER_H

#include <QObject>

class QCompleter;
class SnippetCollection;

//这个类是代码片段补全的核心工具，负责弹窗、选择、插入
class SnippetCompleter : public QObject
{
    Q_OBJECT
public:
    //构造
    explicit SnippetCompleter(SnippetCollection *collection, QWidget *parentWidget);

    //执行补全，显示补全列表
    void performCompletion(const QString &textUnderCursor, const QStringList &words, const QRect &popupRect);

    //判断补全窗口是否显示
    bool isPopupVisible() const;
    //隐藏补全窗口
    void hidePopup();

signals:
    //选中片段后发出信号，通知外部插入内容
    void snippetSelected(const QString &trigger, const QString &snippetContent, int newCursorPos);

private slots:
    //选中补全项时插入片段
    void insertSnippet(const QString &trigger);

private:
    //替换内容里的剪贴板变量
    void replaceClipboardVariable(QString &snippetContent);

private:
    //代码片段数据集合
    SnippetCollection *snippetCollection;
    //Qt补全组件
    QCompleter *completer;
};

#endif // SNIPPETCOMPLETER_H

