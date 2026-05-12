// 文件说明：app\controls\findreplacewidget.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "findreplacewidget.h" // 中文注释：引入当前文件需要的依赖头文件。
#include "ui_findreplacewidget.h" // 中文注释：引入当前文件需要的依赖头文件。

#include <QMenu> // 中文注释：引入当前文件需要的依赖头文件。
#include <QPlainTextEdit> // 中文注释：引入当前文件需要的依赖头文件。
#include <QRegularExpression> // 中文注释：引入当前文件需要的依赖头文件。

// 函数说明：构造 FindReplaceWidget 对象，初始化本模块需要的状态、界面和资源。
FindReplaceWidget::FindReplaceWidget(QWidget *parent) : // 中文注释：保留当前代码结构。
    QWidget(parent), // 中文注释：声明变量、对象或函数。
    ui(new Ui::FindReplaceWidget), // 中文注释：创建新的对象实例。
    textEditor(0), // 中文注释：继续传入下一项参数。
    findCaseSensitively(false), // 中文注释：继续传入下一项参数。
    findWholeWordsOnly(false), // 中文注释：继续传入下一项参数。
    findUseRegExp(false) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    ui->setupUi(this); // 中文注释：执行当前语句。

    setupFindOptionsMenu(); // 中文注释：执行当前语句。
    setFocusProxy(ui->findLineEdit); // 中文注释：执行当前语句。
} // 中文注释：结束当前代码块。

// 函数说明：销毁 FindReplaceWidget 对象，释放本模块持有的资源。
FindReplaceWidget::~FindReplaceWidget() // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    delete ui; // 中文注释：释放动态创建的对象资源。
} // 中文注释：结束当前代码块。

// 函数说明：设置 FindReplaceWidget 的运行参数，并触发必要的界面或数据刷新。
void FindReplaceWidget::setTextEdit(QPlainTextEdit *editor) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    textEditor = editor; // 中文注释：更新变量或对象状态。
} // 中文注释：结束当前代码块。

// 函数说明：显示 FindReplaceWidget 管理的面板、对话框或提示信息。
void FindReplaceWidget::showEvent(QShowEvent *) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    ui->findLineEdit->selectAll(); // 中文注释：执行当前语句。
} // 中文注释：结束当前代码块。

// 函数说明：处理键盘事件，把快捷键或输入转成编辑器动作。
void FindReplaceWidget::keyPressEvent(QKeyEvent *event) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    if (event->key() == Qt::Key_Escape) { // 中文注释：判断条件是否成立。
        event->accept(); // 中文注释：执行当前语句。
        close(); // 中文注释：执行当前语句。
        emit dialogClosed(); // 中文注释：发出 Qt 信号通知外部对象。
    } // 中文注释：结束当前代码块。
} // 中文注释：结束当前代码块。

// 函数说明：实现 FindReplaceWidget::findPreviousClicked 的核心逻辑，供当前模块调用。
void FindReplaceWidget::findPreviousClicked() // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    if (!textEditor) return; // 中文注释：判断条件是否成立。

    find(ui->findLineEdit->text(), QTextDocument::FindBackward); // 中文注释：执行当前语句。
} // 中文注释：结束当前代码块。

// 函数说明：实现 FindReplaceWidget::findNextClicked 的核心逻辑，供当前模块调用。
void FindReplaceWidget::findNextClicked() // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    if (!textEditor) return; // 中文注释：判断条件是否成立。

    find(ui->findLineEdit->text()); // 中文注释：执行当前语句。
} // 中文注释：结束当前代码块。

// 函数说明：实现 FindReplaceWidget::replaceClicked 的核心逻辑，供当前模块调用。
void FindReplaceWidget::replaceClicked() // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    QString oldText = ui->findLineEdit->text(); // 中文注释：声明变量、对象或函数。
    QString newText = ui->replaceLineEdit->text(); // 中文注释：声明变量、对象或函数。

    QTextCursor cursor = textEditor->textCursor(); // 中文注释：声明变量、对象或函数。
    cursor.beginEditBlock(); // 中文注释：执行当前语句。

    if (cursor.hasSelection()) { // 中文注释：判断条件是否成立。
        cursor.insertText(newText); // 中文注释：执行当前语句。
    } // 中文注释：结束当前代码块。

    find(oldText); // 中文注释：执行当前语句。

    cursor.endEditBlock(); // 中文注释：执行当前语句。
} // 中文注释：结束当前代码块。

// 函数说明：实现 FindReplaceWidget::replaceAllClicked 的核心逻辑，供当前模块调用。
void FindReplaceWidget::replaceAllClicked() // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    QString oldText = ui->findLineEdit->text(); // 中文注释：声明变量、对象或函数。
    QString newText = ui->replaceLineEdit->text(); // 中文注释：声明变量、对象或函数。

    textEditor->moveCursor(QTextCursor::Start); // 中文注释：执行当前语句。
    QTextCursor cursor = textEditor->textCursor(); // 中文注释：声明变量、对象或函数。
    cursor.beginEditBlock(); // 中文注释：执行当前语句。

    bool found = find(oldText); // 中文注释：声明变量、对象或函数。
    while (found) { // 中文注释：按条件持续循环处理。
        QTextCursor tc = textEditor->textCursor(); // 中文注释：声明变量、对象或函数。
        if (tc.hasSelection()) { // 中文注释：判断条件是否成立。
            tc.insertText(newText); // 中文注释：执行当前语句。
        } // 中文注释：结束当前代码块。
        found = find(oldText); // 中文注释：更新变量或对象状态。
    } // 中文注释：结束当前代码块。

    cursor.endEditBlock(); // 中文注释：执行当前语句。
} // 中文注释：结束当前代码块。

// 函数说明：实现 FindReplaceWidget::caseSensitiveToggled 的核心逻辑，供当前模块调用。
void FindReplaceWidget::caseSensitiveToggled(bool enabled) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    findCaseSensitively = enabled; // 中文注释：更新变量或对象状态。
} // 中文注释：结束当前代码块。

// 函数说明：实现 FindReplaceWidget::wholeWordsOnlyToggled 的核心逻辑，供当前模块调用。
void FindReplaceWidget::wholeWordsOnlyToggled(bool enabled) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    findWholeWordsOnly = enabled; // 中文注释：更新变量或对象状态。
} // 中文注释：结束当前代码块。

// 函数说明：实现 FindReplaceWidget::useRegularExpressionsToggled 的核心逻辑，供当前模块调用。
void FindReplaceWidget::useRegularExpressionsToggled(bool enabled) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    findUseRegExp = enabled; // 中文注释：更新变量或对象状态。
} // 中文注释：结束当前代码块。

// 函数说明：显示 FindReplaceWidget 管理的面板、对话框或提示信息。
void FindReplaceWidget::showOptionsMenu() // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    QMenu *findOptionsMenu = qobject_cast<QAction*>(sender())->menu(); // 中文注释：声明变量、对象或函数。

    // show menu above the line edit
    QPoint pos = ui->findLineEdit->mapToGlobal(QPoint(0, 0)); // 中文注释：声明变量、对象或函数。
    pos.ry() -= findOptionsMenu->sizeHint().height(); // 中文注释：更新变量或对象状态。
    findOptionsMenu->exec(pos); // 中文注释：执行当前语句。
} // 中文注释：结束当前代码块。

// 函数说明：初始化 FindReplaceWidget 的 setupFindOptionsMenu 相关界面、动作或服务连接。
void FindReplaceWidget::setupFindOptionsMenu() // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    QMenu *findOptionsMenu = new QMenu(this); // 中文注释：创建新的对象实例。

    QAction *action = findOptionsMenu->addAction(tr("Case Sensitive")); // 中文注释：声明变量、对象或函数。
    action->setCheckable(true); // 中文注释：执行当前语句。
    connect(action, &QAction::toggled, this, &FindReplaceWidget::caseSensitiveToggled); // 中文注释：连接信号与槽，建立事件响应关系。

    action = findOptionsMenu->addAction(tr("Whole Words Only")); // 中文注释：更新变量或对象状态。
    action->setCheckable(true); // 中文注释：执行当前语句。
    connect(action, &QAction::toggled, this, &FindReplaceWidget::wholeWordsOnlyToggled); // 中文注释：连接信号与槽，建立事件响应关系。

    action = findOptionsMenu->addAction(tr("Use Regular Expressions")); // 中文注释：更新变量或对象状态。
    action->setCheckable(true); // 中文注释：执行当前语句。
    connect(action, &QAction::toggled, this, &FindReplaceWidget::useRegularExpressionsToggled); // 中文注释：连接信号与槽，建立事件响应关系。

    // add action to line edit to show the options menu
    action = new QAction(tr("Find Options"), this); // 中文注释：创建新的对象实例。
    action->setIcon(QIcon("fa-search.fontawesome")); // 中文注释：执行当前语句。
    action->setMenu(findOptionsMenu); // 中文注释：执行当前语句。
    connect(action, &QAction::triggered, this, &FindReplaceWidget::showOptionsMenu); // 中文注释：连接信号与槽，建立事件响应关系。
    ui->findLineEdit->addAction(action, QLineEdit::LeadingPosition); // 中文注释：执行当前语句。
} // 中文注释：结束当前代码块。

// 函数说明：实现 FindReplaceWidget::find 的核心逻辑，供当前模块调用。
bool FindReplaceWidget::find(const QString &searchString, QTextDocument::FindFlags findOptions) const // 中文注释：声明变量、对象或函数。
{ // 中文注释：进入当前代码块。
    if (searchString.isEmpty() || !textEditor) // 中文注释：判断条件是否成立。
        return false; // 中文注释：返回函数处理结果。

    if (findCaseSensitively) // 中文注释：判断条件是否成立。
        findOptions |= QTextDocument::FindCaseSensitively; // 中文注释：更新变量或对象状态。

    if (findWholeWordsOnly) // 中文注释：判断条件是否成立。
        findOptions |= QTextDocument::FindWholeWords; // 中文注释：更新变量或对象状态。

    bool found; // 中文注释：声明变量、对象或函数。
    if (findUseRegExp) { // 中文注释：判断条件是否成立。
        found = findUsingRegExp(searchString, findOptions); // 中文注释：更新变量或对象状态。
    } else { // 中文注释：结束当前代码块或分支。
        found = textEditor->find(searchString, findOptions); // 中文注释：更新变量或对象状态。
    } // 中文注释：结束当前代码块。

    // 如果没找到，回绕到文档开头/末尾再搜索一次
    if (!found) { // 中文注释：判断条件是否成立。
        QTextCursor cursor = textEditor->textCursor(); // 中文注释：声明变量、对象或函数。
        if (findOptions & QTextDocument::FindBackward) { // 中文注释：判断条件是否成立。
            cursor.movePosition(QTextCursor::End); // 中文注释：执行当前语句。
        } else { // 中文注释：结束当前代码块或分支。
            cursor.movePosition(QTextCursor::Start); // 中文注释：执行当前语句。
        } // 中文注释：结束当前代码块。
        textEditor->setTextCursor(cursor); // 中文注释：执行当前语句。

        if (findUseRegExp) { // 中文注释：判断条件是否成立。
            found = findUsingRegExp(searchString, findOptions); // 中文注释：更新变量或对象状态。
        } else { // 中文注释：结束当前代码块或分支。
            found = textEditor->find(searchString, findOptions); // 中文注释：更新变量或对象状态。
        } // 中文注释：结束当前代码块。
    } // 中文注释：结束当前代码块。

    return found; // 中文注释：返回函数处理结果。
} // 中文注释：结束当前代码块。

// 函数说明：实现 FindReplaceWidget::findUsingRegExp 的核心逻辑，供当前模块调用。
bool FindReplaceWidget::findUsingRegExp(const QString &pattern, QTextDocument::FindFlags findOptions) const // 中文注释：声明变量、对象或函数。
{ // 中文注释：进入当前代码块。
    QRegularExpression rx(pattern, findCaseSensitively ? // 中文注释：声明变量、对象或函数。
        QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption); // 中文注释：声明变量、对象或函数。

    if (!rx.isValid()) // 中文注释：判断条件是否成立。
        return false; // 中文注释：返回函数处理结果。

    QTextCursor search = textEditor->document()->find(rx, textEditor->textCursor(), findOptions); // 中文注释：声明变量、对象或函数。
    if (search.isNull()) // 中文注释：判断条件是否成立。
        return false; // 中文注释：返回函数处理结果。

    textEditor->setTextCursor(search); // 中文注释：执行当前语句。
    return true; // 中文注释：返回函数处理结果。
} // 中文注释：结束当前代码块。

