// 文件说明：app\snippetcompleter.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "snippetcompleter.h"
#include <QApplication>
#include <QAbstractItemView>
#include <QCompleter>
#include <QClipboard>
#include <QScrollBar>
#include <snippets/snippet.h>
#include <snippets/snippetcollection.h>
#include <completionlistmodel.h>

// 构造函数
// 初始化数据集合，创建补全器，设置界面和信号槽
SnippetCompleter::SnippetCompleter(SnippetCollection *collection, QWidget *parentWidget) :
    QObject(parentWidget),
    snippetCollection(collection),
    completer(new QCompleter(this))
{
    // 设置补全器依附的父控件
    completer->setWidget(parentWidget);
    // 设置补全模式：弹出窗口提示
    completer->setCompletionMode(QCompleter::PopupCompletion);
    // 大小写敏感
    completer->setCaseSensitivity(Qt::CaseSensitive);

    // 当用户选中补全项时，触发插入片段函数
    connect(completer, QOverload<const QString &>::of(&QCompleter::activated),
            this, &SnippetCompleter::insertSnippet);

    // 创建自定义补全模型
    CompletionListModel *model = new CompletionListModel(completer);
    // 数据集合变化时，通知模型更新
    connect(collection, &SnippetCollection::collectionChanged,
            model, &CompletionListModel::snippetCollectionChanged);
    // 给补全器设置模型
    completer->setModel(model);
}

// 执行补全逻辑
// 根据当前文本、单词列表，显示或更新补全弹窗
void SnippetCompleter::performCompletion(const QString &textUnderCursor, const QStringList &words, const QRect &popupRect)
{
    // 获取当前用于匹配的前缀文本
    const QString completionPrefix = textUnderCursor;

    // 将当前单词列表传递给补全模型
    qobject_cast<CompletionListModel*>(completer->model())->setWords(words);

    // 如果前缀发生变化，更新补全器的前缀
    if (completionPrefix != completer->completionPrefix()) {
        completer->setCompletionPrefix(completionPrefix);
        // 默认选中第一个补全项
        completer->popup()->setCurrentIndex(completer->completionModel()->index(0, 0));
    }

    // 如果只有一个匹配结果，直接插入
    if (completer->completionCount() == 1) {
        insertSnippet(completer->currentCompletion());
    } else {
        // 否则计算弹窗宽度并显示补全窗口
        QRect rect = popupRect;
        rect.setWidth(completer->popup()->sizeHintForColumn(0) +
                      completer->popup()->verticalScrollBar()->sizeHint().width());
        completer->complete(rect);
    }
}

// 判断补全弹窗是否可见
bool SnippetCompleter::isPopupVisible() const
{
    return completer->popup()->isVisible();
}

// 隐藏补全弹窗
void SnippetCompleter::hidePopup()
{
    completer->popup()->hide();
}

// 插入选中的代码片段
// 根据触发器找到片段，处理内容后发送信号给编辑器
void SnippetCompleter::insertSnippet(const QString &trigger)
{
    // 如果数据集合不存在或不包含该触发器，直接插入文本
    if (!snippetCollection || !snippetCollection->contains(trigger)) {
        emit snippetSelected(completer->completionPrefix(), trigger, trigger.length());
        return;
    }

    // 从数据集合中获取对应的代码片段
    const Snippet snippet = snippetCollection->snippet(trigger);

    // 复制片段内容
    QString snippetContent(snippet.snippet);
    // 替换内容中的剪贴板变量
    replaceClipboardVariable(snippetContent);

    // 发送信号，通知编辑器插入内容并移动光标
    emit snippetSelected(completer->completionPrefix(), snippetContent, snippet.cursorPosition);
}

// 替换片段中的 %clipboard 变量为系统剪贴板内容
void SnippetCompleter::replaceClipboardVariable(QString &snippetContent)
{
    if (snippetContent.contains("%clipboard")) {
        QClipboard *clipboard = QApplication::clipboard();
        snippetContent.replace("%clipboard", clipboard->text());
    }
}

