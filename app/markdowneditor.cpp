// 文件说明：app\markdowneditor.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "markdowneditor.h" // 中文注释：引入当前文件需要的依赖头文件。

#include <QAction> // 中文注释：引入当前文件需要的依赖头文件。
#include <QApplication> // 中文注释：引入当前文件需要的依赖头文件。
#include <QFile> // 中文注释：引入当前文件需要的依赖头文件。
#include <QMenu> // 中文注释：引入当前文件需要的依赖头文件。
#include <QMimeData> // 中文注释：引入当前文件需要的依赖头文件。
#include <QPainter> // 中文注释：引入当前文件需要的依赖头文件。
#include <QRegularExpression> // 中文注释：引入当前文件需要的依赖头文件。
#include <QShortcut> // 中文注释：引入当前文件需要的依赖头文件。
#include <QStyle> // 中文注释：引入当前文件需要的依赖头文件。
#include <QTextBlock> // 中文注释：引入当前文件需要的依赖头文件。
#include <QTextStream> // 中文注释：引入当前文件需要的依赖头文件。

#include <controls/linenumberarea.h> // 中文注释：引入当前文件需要的依赖头文件。
#include <peg-markdown-highlight/styleparser.h> // 中文注释：引入当前文件需要的依赖头文件。
#include <markdownhighlighter.h> // 中文注释：引入当前文件需要的依赖头文件。
#include "markdownmanipulator.h" // 中文注释：引入当前文件需要的依赖头文件。
#include "snippetcompleter.h" // 中文注释：引入当前文件需要的依赖头文件。

#include <spellchecker/dictionary.h> // 中文注释：引入当前文件需要的依赖头文件。
#include "hunspell/spellchecker.h" // 中文注释：引入当前文件需要的依赖头文件。
using hunspell::SpellChecker; // 中文注释：引入命名空间或类型别名，简化后续书写。

#include <QScrollBar> // 中文注释：引入当前文件需要的依赖头文件。
//修复滚动条信号阻塞问题的自定义滚动条类
class ScrollBarFix : public QScrollBar { // 中文注释：声明类类型或前置声明类。
public: // 中文注释：声明类成员的访问权限区域。
    //创建一个水平/  垂直滚动条，并且挂在父窗口上，其他完全沿用 Qt 自带滚动条的功能。
    //Qt::Orientation orient决定垂直还是水平
    ScrollBarFix(Qt::Orientation orient, QWidget *parent=0) // 中文注释：更新变量或对象状态。
        : QScrollBar(orient, parent) {} // 中文注释：保留当前代码结构。

protected: // 中文注释：声明类成员的访问权限区域。
    //当滑块值发生变化时，强制把 “信号阻塞” 关掉，保证滚动一定能生效。
    //重写滑块变化事件,确保滑块值变化时信号正常发送
    void sliderChange(SliderChange change) { // 中文注释：声明变量、对象或函数。
        //如果信号被阻塞 并且是滑块值变化事件
        if (signalsBlocked() && change == QAbstractSlider::SliderValueChange) // 中文注释：判断条件是否成立。
            blockSignals(false);//立刻解除信号阻塞
        //调用原来Qt滚动条正常逻辑
        QScrollBar::sliderChange(change); // 中文注释：声明变量、对象或函数。
    } // 中文注释：结束当前代码块。
}; // 中文注释：结束类型或作用域声明。

//构造函数：初始化编辑器，行号，高亮，拼写检查，快捷键
//这是 Markdown 编辑器的 “启动初始化函数”，负责创建行号、高亮、拼写检查、字体、滚动条、右键菜单、快捷键等所有基础功能。
MarkdownEditor::MarkdownEditor(QWidget *parent) : // 中文注释：保留当前代码结构。
    QPlainTextEdit(parent), // 中文注释：声明变量、对象或函数。
    lineNumberArea(new LineNumberArea(this)), // 中文注释：创建新的对象实例。
    spellChecker(new SpellChecker()), // 中文注释：创建新的对象实例。
    completer(0), // 中文注释：继续传入下一项参数。
    showHardLinebreaks(false) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    //创建语法高亮器，关联文档和拼写检查
    highlighter = new MarkdownHighlighter(this->document(), spellChecker); // 中文注释：创建新的对象实例。
    //设置等宽字体。适合代码编辑
    QFont font("Monospace", 10); // 中文注释：声明变量、对象或函数。
    //设置字体风格位“打字机模式”，保证所有字符宽度一致
    font.setStyleHint(QFont::TypeWriter); // 中文注释：执行当前语句。
    //给行号区域设置刚才创建的等宽字体
    lineNumberArea->setFont(font); // 中文注释：执行当前语句。
    //给编辑器正文也设置相同的字体
    setFont(font); // 中文注释：执行当前语句。

    //使用修复后的垂直滚动条
    setVerticalScrollBar(new ScrollBarFix(Qt::Vertical, this)); // 中文注释：创建新的对象实例。

    //文本块数量变化时，更新行号区域宽度
    connect(this, &MarkdownEditor::blockCountChanged, // 中文注释：连接信号与槽，建立事件响应关系。
            this, &MarkdownEditor::updateLineNumberAreaWidth); // 中文注释：执行当前语句。
    //编辑内容更新时，同步更新行号区域
    connect(this, &MarkdownEditor::updateRequest, // 中文注释：连接信号与槽，建立事件响应关系。
            this, &MarkdownEditor::updateLineNumberArea); // 中文注释：执行当前语句。
    //设置右键菜单
    setContextMenuPolicy(Qt::CustomContextMenu); // 中文注释：执行当前语句。
    connect(this, &MarkdownEditor::customContextMenuRequested, // 中文注释：连接信号与槽，建立事件响应关系。
            this, &MarkdownEditor::showContextMenu); // 中文注释：执行当前语句。
    //初始化行号区域宽度
    updateLineNumberAreaWidth(0); // 中文注释：执行当前语句。
    // 创建代码补全快捷键 Ctrl+Space
    QAction * actionComplete = new QAction(tr("Snippet Complete"), this); // 中文注释：创建新的对象实例。
    // 给动作设置对象名称，方便后续查找
    actionComplete->setObjectName("actionComplete"); // 中文注释：执行当前语句。
    //快捷键
    actionComplete->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Space)); // 中文注释：执行当前语句。
    //保存快捷键
    actionComplete->setProperty("defaultshortcut", actionComplete->shortcut()); // 中文注释：执行当前语句。
    // 连接动作触发信号
    // 按下快捷键时执行代码补全函数
    connect(actionComplete, &QAction::triggered, // 中文注释：连接信号与槽，建立事件响应关系。
            this, &MarkdownEditor::performCompletion); // 中文注释：执行当前语句。
    //将这个快捷键动作添加到编辑器中
    this->addAction(actionComplete); // 中文注释：执行当前语句。
} // 中文注释：结束当前代码块。

// 函数说明：销毁 MarkdownEditor 对象，释放本模块持有的资源。
MarkdownEditor::~MarkdownEditor() // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    delete spellChecker; // 中文注释：释放动态创建的对象资源。
} // 中文注释：结束当前代码块。
//绘制行号区域
//一行一行遍历编辑器可见区域 → 画出每一行的行号 → 选中行变色高亮。
void MarkdownEditor::lineNumberAreaPaintEvent(QPaintEvent *event) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    //创建画家对象，在行号区域上绘图
    QPainter painter(lineNumberArea); // 中文注释：声明变量、对象或函数。
    //获取选中文本的起止位置
    int selStart = textCursor().selectionStart(); // 中文注释：声明变量、对象或函数。
    int selEnd = textCursor().selectionEnd(); // 中文注释：声明变量、对象或函数。
    //获取行号区域的调色板：颜色配置
    QPalette palette = lineNumberArea->palette(); // 中文注释：声明变量、对象或函数。
    //设置为激活状态的颜色
    palette.setCurrentColorGroup(QPalette::Active); // 中文注释：执行当前语句。
    //填充行号区域背景
    painter.fillRect(event->rect(), palette.color(QPalette::Window)); // 中文注释：执行当前语句。
    //从第一个可见行开始遍历
    QTextBlock block = firstVisibleBlock(); // 中文注释：声明变量、对象或函数。
    //获取这一行的行号
    int blockNumber = block.blockNumber(); // 中文注释：声明变量、对象或函数。
    //计算该行在屏幕上的顶部坐标
    qreal top = blockBoundingGeometry(block).translated(contentOffset()).top(); // 中文注释：声明变量、对象或函数。
    //底部坐标初始化为顶部坐标
    qreal bottom = top; // 中文注释：声明变量、对象或函数。
    //只要行有效，并且没有画到屏幕底部，就继续画
    while (block.isValid() && top <= event->rect().bottom()) { // 中文注释：按条件持续循环处理。
        //把当前行的顶部设为上一行的底部
        top = bottom; // 中文注释：更新变量或对象状态。
        //获取当前行的高度
        const qreal height = blockBoundingRect(block).height(); // 中文注释：声明变量、对象或函数。
        //计算当前行的底部坐标
        bottom = top + height; // 中文注释：更新变量或对象状态。
        //当前行可见，并且在需要绘制的区域内
        if (block.isVisible() && bottom >= event->rect().top()) { // 中文注释：判断条件是否成立。
            //设置画笔颜色为默认文字颜色
            painter.setPen(palette.windowText().color()); // 中文注释：执行当前语句。
            //判断该行是否被选中
            bool selected = ( // 中文注释：声明变量、对象或函数。
                                (selStart < block.position() + block.length() && selEnd > block.position()) // 中文注释：保留当前代码结构。
                                || (selStart == selEnd && selStart == block.position()) // 中文注释：更新变量或对象状态。
                            ); // 中文注释：执行当前语句。
              //选中
            if (selected) { // 中文注释：判断条件是否成立。
                painter.save(); // 中文注释：执行当前语句。
                painter.setPen(palette.highlight().color()); // 中文注释：执行当前语句。
            } // 中文注释：结束当前代码块。
             // 把行号转成字符串（+1 是因为界面行号从1开始）
            const QString number = QString::number(blockNumber + 1); // 中文注释：声明变量、对象或函数。
            // 在行号区域靠右绘制行号
            painter.drawText(0, top, lineNumberArea->width() - 4, height, Qt::AlignRight, number); // 中文注释：执行当前语句。

            if (selected) // 中文注释：判断条件是否成立。
                painter.restore(); // 中文注释：执行当前语句。
        } // 中文注释：结束当前代码块。
        //换掉下一行
        block = block.next(); // 中文注释：更新变量或对象状态。
        //行号+1
        ++blockNumber; // 中文注释：执行当前语句。
    } // 中文注释：结束当前代码块。
} // 中文注释：结束当前代码块。

//计算机行号区域需要的宽度
int MarkdownEditor::lineNumberAreaWidth() // 中文注释：声明变量、对象或函数。
{ // 中文注释：进入当前代码块。
    int digits = 2; // 中文注释：声明变量、对象或函数。
    int max = qMax(1, blockCount()); // 中文注释：声明变量、对象或函数。
    //计算最大行号需要多少位数字
    while (max >= 100) { // 中文注释：按条件持续循环处理。
        max /= 10; // 中文注释：更新变量或对象状态。
        ++digits; // 中文注释：执行当前语句。
    } // 中文注释：结束当前代码块。

    QFont font = lineNumberArea->font(); // 中文注释：声明变量、对象或函数。
    const QFontMetrics linefmt(font); // 中文注释：声明变量、对象或函数。
    //计算宽度
    int space = 10 + linefmt.horizontalAdvance(QLatin1Char('9')) * digits; // 中文注释：声明变量、对象或函数。
    return space; // 中文注释：返回函数处理结果。
} // 中文注释：结束当前代码块。
//重置语法高亮
void MarkdownEditor::resetHighlighting() // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    highlighter->reset(); // 中文注释：执行当前语句。
} // 中文注释：结束当前代码块。
// 重写绘制事件：绘制换行标记、标尺等附加内容
void MarkdownEditor::paintEvent(QPaintEvent *e) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    QPlainTextEdit::paintEvent(e); // 中文注释：声明变量、对象或函数。

    // draw line end markers if enabled
    if (showHardLinebreaks) { // 中文注释：判断条件是否成立。
        drawLineEndMarker(e); // 中文注释：执行当前语句。
    } // 中文注释：结束当前代码块。

    // draw column ruler
    if (rulerEnabled) { // 中文注释：判断条件是否成立。
        drawRuler(e); // 中文注释：执行当前语句。
    } // 中文注释：结束当前代码块。
} // 中文注释：结束当前代码块。
// 窗口大小改变时，同步调整行号区域大小
void MarkdownEditor::resizeEvent(QResizeEvent *event) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    QPlainTextEdit::resizeEvent(event); // 中文注释：声明变量、对象或函数。

    // update line number area
    QRect cr = contentsRect(); // 中文注释：声明变量、对象或函数。
    lineNumberArea->setGeometry(QStyle::visualRect(layoutDirection(), cr, // 中文注释：继续传入下一项参数。
                                QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()))); // 中文注释：声明变量、对象或函数。
} // 中文注释：结束当前代码块。
// 键盘事件处理：处理自动补全弹出时的按键
void MarkdownEditor::keyPressEvent(QKeyEvent *e) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    if (completer && completer->isPopupVisible()) { // 中文注释：判断条件是否成立。
        // The following keys are forwarded by the completer to the widget
       switch (e->key()) { // 中文注释：根据不同取值进入分支处理。
       case Qt::Key_Enter: // 中文注释：处理 switch 的指定分支。
       case Qt::Key_Return: // 中文注释：处理 switch 的指定分支。
       case Qt::Key_Escape: // 中文注释：处理 switch 的指定分支。
       case Qt::Key_Tab: // 中文注释：处理 switch 的指定分支。
       case Qt::Key_Backtab: // 中文注释：处理 switch 的指定分支。
            e->ignore(); // 中文注释：执行当前语句。
            return; // let the completer do default behavior
       default: // 中文注释：处理 switch 的默认分支。
           break; // 中文注释：跳出当前分支或循环。
       } // 中文注释：结束当前代码块。
    } // 中文注释：结束当前代码块。

    if (completer) // 中文注释：判断条件是否成立。
        completer->hidePopup(); // 中文注释：执行当前语句。

    QPlainTextEdit::keyPressEvent(e); // 中文注释：声明变量、对象或函数。
} // 中文注释：结束当前代码块。
// 判断是否可以粘贴数据（支持拖拽本地文件）
bool MarkdownEditor::canInsertFromMimeData(const QMimeData *source) const // 中文注释：声明变量、对象或函数。
{ // 中文注释：进入当前代码块。
    if (isUrlToLocalFile(source)) { // 中文注释：判断条件是否成立。
        return true; // 中文注释：返回函数处理结果。
    } // 中文注释：结束当前代码块。

    return QPlainTextEdit::canInsertFromMimeData(source); // 中文注释：返回函数处理结果。
} // 中文注释：结束当前代码块。
// 粘贴数据：如果是文件，发送加载信号；否则正常粘贴
void MarkdownEditor::insertFromMimeData(const QMimeData *source) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    if (isUrlToLocalFile(source)) { // 中文注释：判断条件是否成立。
        emit loadDroppedFile(source->urls().first().toLocalFile()); // 中文注释：发出 Qt 信号通知外部对象。
    } else { // 中文注释：结束当前代码块或分支。
        QPlainTextEdit::insertFromMimeData(source); // 中文注释：声明变量、对象或函数。
    } // 中文注释：结束当前代码块。
} // 中文注释：结束当前代码块。
// 更新编辑器左侧边距（给行号区域留出空间）
void MarkdownEditor::updateLineNumberAreaWidth(int newBlockCount) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    Q_UNUSED(newBlockCount) // 中文注释：标记参数暂未使用，避免编译警告。
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0); // 中文注释：执行当前语句。
} // 中文注释：结束当前代码块。
// 滚动或内容变化时，更新行号区域显示
void MarkdownEditor::updateLineNumberArea(const QRect &rect, int dy) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    if (dy) // 中文注释：判断条件是否成立。
        lineNumberArea-> scroll(0, dy); // 中文注释：执行当前语句。
    else // 中文注释：处理条件不成立时的逻辑。
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height()); // 中文注释：执行当前语句。

    if (rect.contains(viewport()->rect())) // 中文注释：判断条件是否成立。
        updateLineNumberAreaWidth(0); // 中文注释：执行当前语句。
} // 中文注释：结束当前代码块。

// 字体改变时，同步更新行号区域字体
void MarkdownEditor::editorFontChanged(const QFont &font) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    lineNumberArea->setFont(font); // 中文注释：执行当前语句。
    setFont(font); // 中文注释：执行当前语句。
} // 中文注释：结束当前代码块。
// 制表符宽度改变时重新计算
void MarkdownEditor::tabWidthChanged(int tabWidth) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    QFontMetrics fm(font()); // 中文注释：声明变量、对象或函数。
    setTabStopDistance(tabWidth*fm.horizontalAdvance(' ')); // 中文注释：执行当前语句。
} // 中文注释：结束当前代码块。
// 标尺开关状态改变
void MarkdownEditor::rulerEnabledChanged(bool enabled) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    rulerEnabled = enabled; // 中文注释：更新变量或对象状态。
    viewport()->update(); // 中文注释：执行当前语句。
} // 中文注释：结束当前代码块。
// 标尺位置改变
void MarkdownEditor::rulerPosChanged(int pos) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    rulerPos = pos; // 中文注释：更新变量或对象状态。
    viewport()->update(); // 中文注释：执行当前语句。
} // 中文注释：结束当前代码块。
// 显示右键菜单，包含拼写建议
void MarkdownEditor::showContextMenu(const QPoint &pos) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    QMenu *contextMenu = createStandardContextMenu(); // 中文注释：声明变量、对象或函数。

    QTextCursor cursor = cursorForPosition(pos); // 中文注释：声明变量、对象或函数。
    int cursorPosition = cursor.position(); // 中文注释：声明变量、对象或函数。
    cursor.select(QTextCursor::WordUnderCursor); // 中文注释：执行当前语句。

    // if word under cursor not spelled correctly, add suggestions to context menu
     // 如果光标下单词拼写错误，添加拼写建议
    if (cursor.hasSelection() && !spellChecker->isCorrect(cursor.selectedText())) { // 中文注释：判断条件是否成立。
        contextMenu->addSeparator(); // 中文注释：执行当前语句。

        // add new submenu for the suggestions
        QMenu *subMenu = new QMenu(tr("Suggestions"), contextMenu); // 中文注释：创建新的对象实例。
        contextMenu->addMenu(subMenu); // 中文注释：执行当前语句。

        // add action for each suggested replacement
        QStringList suggestions = spellChecker->suggestions(cursor.selectedText()); // 中文注释：声明变量、对象或函数。
        for (const QString &suggestion : suggestions) { // 中文注释：开始循环遍历数据。
            QAction *action = subMenu->addAction(suggestion); // 中文注释：声明变量、对象或函数。
            action->setData(cursorPosition); // 中文注释：执行当前语句。
            connect(action, &QAction::triggered, // 中文注释：连接信号与槽，建立事件响应关系。
                    this, &MarkdownEditor::replaceWithSuggestion); // 中文注释：执行当前语句。
        } // 中文注释：结束当前代码块。

        // disable submenu when no suggestions available
        if (suggestions.isEmpty()) { // 中文注释：判断条件是否成立。
            subMenu->setEnabled(false); // 中文注释：执行当前语句。
        } // 中文注释：结束当前代码块。

        QAction *userWordlistAction = contextMenu->addAction(tr("Add to User Dictionary")); // 中文注释：声明变量、对象或函数。
        userWordlistAction->setData(cursor.selectedText()); // 中文注释：执行当前语句。
        connect(userWordlistAction, &QAction::triggered, // 中文注释：连接信号与槽，建立事件响应关系。
                this, &MarkdownEditor::addWordToUserWordlist); // 中文注释：执行当前语句。
    } // 中文注释：结束当前代码块。

    // show context menu
    contextMenu->exec(mapToGlobal(pos)); // 中文注释：执行当前语句。
    delete contextMenu; // 中文注释：释放动态创建的对象资源。
} // 中文注释：结束当前代码块。

// 用拼写建议替换错误单词
void MarkdownEditor::replaceWithSuggestion() // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    QAction *action = qobject_cast<QAction*>(sender()); // 中文注释：声明变量、对象或函数。

    QTextCursor cursor = textCursor(); // 中文注释：声明变量、对象或函数。
    cursor.beginEditBlock(); // 中文注释：执行当前语句。

    // replace wrong spelled word with suggestion
    cursor.setPosition(action->data().toInt()); // 中文注释：执行当前语句。
    cursor.select(QTextCursor::WordUnderCursor); // 中文注释：执行当前语句。
    cursor.insertText(action->text()); // 中文注释：执行当前语句。

    cursor.endEditBlock(); // 中文注释：执行当前语句。
} // 中文注释：结束当前代码块。
// 执行代码片段补全
void MarkdownEditor::performCompletion() // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    if (!completer) return; // 中文注释：判断条件是否成立。

    QRect popupRect = cursorRect(); // 中文注释：声明变量、对象或函数。
    popupRect.setLeft(popupRect.left() + lineNumberAreaWidth()); // 中文注释：执行当前语句。

    QStringList words = extractDistinctWordsFromDocument(); // 中文注释：声明变量、对象或函数。
    completer->performCompletion(textUnderCursor(), words, popupRect); // 中文注释：执行当前语句。
} // 中文注释：结束当前代码块。
// 插入代码片段到编辑器
void MarkdownEditor::insertSnippet(const QString &completionPrefix, const QString &completion, int newCursorPos) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    QTextCursor cursor = this->textCursor(); // 中文注释：声明变量、对象或函数。

    // select the completion prefix
    cursor.clearSelection(); // 中文注释：执行当前语句。
    cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, completionPrefix.length()); // 中文注释：执行当前语句。

    int pos = cursor.position(); // 中文注释：声明变量、对象或函数。

    // replace completion prefix with snippet
    cursor.insertText(completion); // 中文注释：执行当前语句。

    // move cursor to requested position
    cursor.setPosition(pos); // 中文注释：执行当前语句。
    cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, newCursorPos); // 中文注释：执行当前语句。

    this->setTextCursor(cursor); // 中文注释：执行当前语句。
} // 中文注释：结束当前代码块。

// 将单词添加到用户自定义词典
void MarkdownEditor::addWordToUserWordlist() // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    QAction *action = qobject_cast<QAction*>(sender()); // 中文注释：声明变量、对象或函数。
    QString word = action->data().toString(); // 中文注释：声明变量、对象或函数。
    spellChecker->addToUserWordlist(word); // 中文注释：执行当前语句。
} // 中文注释：结束当前代码块。
// 判断粘贴数据是否为本地文件
bool MarkdownEditor::isUrlToLocalFile(const QMimeData *source) const // 中文注释：声明变量、对象或函数。
{ // 中文注释：进入当前代码块。
    return source->hasUrls() && (source->urls().count() == 1) && source->urls().first().isLocalFile(); // 中文注释：返回函数处理结果。
} // 中文注释：结束当前代码块。
// 从样式表文件加载高亮和编辑器主题
void MarkdownEditor::loadStyleFromStylesheet(const QString &fileName) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    QFile f(fileName); // 中文注释：声明变量、对象或函数。
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) { // 中文注释：判断条件是否成立。
        return; // 中文注释：返回函数处理结果。
    } // 中文注释：结束当前代码块。

    QTextStream ts(&f); // 中文注释：声明变量、对象或函数。
    QString input = ts.readAll(); // 中文注释：声明变量、对象或函数。

    // parse the stylesheet
    PegMarkdownHighlight::StyleParser parser(input); // 中文注释：声明变量、对象或函数。
    QVector<PegMarkdownHighlight::HighlightingStyle> styles = parser.highlightingStyles(this->font()); // 中文注释：声明变量、对象或函数。

    // set new style & rehighlight markdown document
    highlighter->setStyles(styles); // 中文注释：执行当前语句。
    highlighter->rehighlight(); // 中文注释：执行当前语句。

    // update color palette
    this->setPalette(parser.editorPalette()); // 中文注释：执行当前语句。
    this->viewport()->setPalette(this->palette()); // 中文注释：执行当前语句。
} // 中文注释：结束当前代码块。
// 统计文档中的单词数量
int MarkdownEditor::countWords() const // 中文注释：声明变量、对象或函数。
{ // 中文注释：进入当前代码块。
    QString text = toPlainText(); // 中文注释：声明变量、对象或函数。

    // empty or only whitespaces?
    if (text.trimmed().isEmpty()) { // 中文注释：判断条件是否成立。
        return 0; // 中文注释：返回函数处理结果。
    } // 中文注释：结束当前代码块。

    int words = 0; // 中文注释：声明变量、对象或函数。
    bool lastWasWhitespace = false; // 中文注释：声明变量、对象或函数。
    bool firstCharacter = false; // 中文注释：声明变量、对象或函数。

    for (int i = 0; i < text.count(); ++i) { // 中文注释：开始循环遍历数据。
        if (text.at(i).isSpace()) { // 中文注释：判断条件是否成立。
            if (firstCharacter && !lastWasWhitespace) { // 中文注释：判断条件是否成立。
                words++; // 中文注释：执行当前语句。
            } // 中文注释：结束当前代码块。
            lastWasWhitespace = true; // 中文注释：更新变量或对象状态。
        } // 中文注释：结束当前代码块。
        else // 中文注释：处理条件不成立时的逻辑。
        { // 中文注释：进入当前代码块。
            firstCharacter = true; // 中文注释：更新变量或对象状态。
            lastWasWhitespace = false; // 中文注释：更新变量或对象状态。
        } // 中文注释：结束当前代码块。
    } // 中文注释：结束当前代码块。

    if (!lastWasWhitespace && text.count() > 0) { // 中文注释：判断条件是否成立。
        words++; // 中文注释：执行当前语句。
    } // 中文注释：结束当前代码块。

    return words; // 中文注释：返回函数处理结果。
} // 中文注释：结束当前代码块。

// 设置是否显示特殊字符（空格、换行、制表符）
void MarkdownEditor::setShowSpecialCharacters(bool enabled) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    showHardLinebreaks = enabled; // 中文注释：更新变量或对象状态。

    QTextOption textOption = document()->defaultTextOption(); // 中文注释：声明变量、对象或函数。
    QTextOption::Flags optionFlags = textOption.flags(); // 中文注释：声明变量、对象或函数。
    if (enabled) { // 中文注释：判断条件是否成立。
        optionFlags |= QTextOption::ShowLineAndParagraphSeparators; // 中文注释：更新变量或对象状态。
        optionFlags |= QTextOption::ShowTabsAndSpaces; // 中文注释：更新变量或对象状态。
    } else { // 中文注释：结束当前代码块或分支。
        optionFlags &= ~QTextOption::ShowLineAndParagraphSeparators; // 中文注释：更新变量或对象状态。
        optionFlags &= ~QTextOption::ShowTabsAndSpaces; // 中文注释：更新变量或对象状态。
    } // 中文注释：结束当前代码块。
    textOption.setFlags(optionFlags); // 中文注释：执行当前语句。

    document()->setDefaultTextOption(textOption); // 中文注释：执行当前语句。

    // repaint
    viewport()->update(); // 中文注释：执行当前语句。
} // 中文注释：结束当前代码块。
// 启用/禁用拼写检查
void MarkdownEditor::setSpellingCheckEnabled(bool enabled) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    highlighter->setSpellingCheckEnabled(enabled); // 中文注释：执行当前语句。

    // rehighlight markdown document
    highlighter->reset(); // 中文注释：执行当前语句。
    highlighter->rehighlight(); // 中文注释：执行当前语句。
} // 中文注释：结束当前代码块。
// 设置拼写检查使用的词典
void MarkdownEditor::setSpellingDictionary(const Dictionary &dictionary) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    spellChecker->loadDictionary(dictionary.filePath()); // 中文注释：执行当前语句。

#ifdef Q_OS_MAC // 中文注释：根据编译条件启用对应代码。
    // 对于系统字典（filePath 为空），设置 NSSpellChecker 语言
    if (dictionary.filePath().isEmpty()) { // 中文注释：判断条件是否成立。
        hunspell::SpellChecker::nativeSetLanguage(dictionary.language()); // 中文注释：执行当前语句。
    } // 中文注释：结束当前代码块。
#endif // 中文注释：结束条件编译或头文件保护。

    // rehighlight markdown document
    highlighter->reset(); // 中文注释：执行当前语句。
    highlighter->rehighlight(); // 中文注释：执行当前语句。
} // 中文注释：结束当前代码块。
// 设置代码片段补全器
void MarkdownEditor::setSnippetCompleter(SnippetCompleter *completer) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    this->completer = completer; // 中文注释：更新变量或对象状态。

    connect(completer, &SnippetCompleter::snippetSelected, // 中文注释：连接信号与槽，建立事件响应关系。
            this, &MarkdownEditor::insertSnippet); // 中文注释：执行当前语句。
} // 中文注释：结束当前代码块。
// 启用/禁用 YAML 头部语法支持
void MarkdownEditor::setYamlHeaderSupportEnabled(bool enabled) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    highlighter->setYamlHeaderSupportEnabled(enabled); // 中文注释：执行当前语句。

    // rehighlight markdown document
    highlighter->reset(); // 中文注释：执行当前语句。
    highlighter->rehighlight(); // 中文注释：执行当前语句。
} // 中文注释：结束当前代码块。
// 光标跳转到指定行
void MarkdownEditor::gotoLine(int line) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    QTextCursor cursor(document()->findBlockByNumber(line-1)); // 中文注释：声明变量、对象或函数。
    this->setTextCursor(cursor); // 中文注释：执行当前语句。
} // 中文注释：结束当前代码块。
// 绘制硬换行标记（行尾两个空格时显示 ¶）
void MarkdownEditor::drawLineEndMarker(QPaintEvent *e) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    QPainter painter(viewport()); // 中文注释：声明变量、对象或函数。

    int leftMargin = qRound(fontMetrics().horizontalAdvance(" ") / 2.0); // 中文注释：声明变量、对象或函数。
    int lineEndCharWidth = fontMetrics().horizontalAdvance("\u00B6"); // 中文注释：声明变量、对象或函数。
    int fontHeight = fontMetrics().height(); // 中文注释：声明变量、对象或函数。

    QTextBlock block = firstVisibleBlock(); // 中文注释：声明变量、对象或函数。
    while (block.isValid()) { // 中文注释：按条件持续循环处理。
        QRectF blockGeometry = blockBoundingGeometry(block).translated(contentOffset()); // 中文注释：声明变量、对象或函数。
        if (blockGeometry.top() > e->rect().bottom()) // 中文注释：判断条件是否成立。
            break; // 中文注释：跳出当前分支或循环。

        if (block.isVisible() && blockGeometry.toRect().intersects(e->rect())) { // 中文注释：判断条件是否成立。
            QString text = block.text(); // 中文注释：声明变量、对象或函数。
            if (text.endsWith("  ")) { // 中文注释：判断条件是否成立。
                painter.drawText(blockGeometry.left() + fontMetrics().horizontalAdvance(text) + leftMargin, // 中文注释：继续传入下一项参数。
                                 blockGeometry.top(), // 中文注释：继续传入下一项参数。
                                 lineEndCharWidth, // 中文注释：继续传入下一项参数。
                                 fontHeight, // 中文注释：继续传入下一项参数。
                                 Qt::AlignLeft | Qt::AlignVCenter, // 中文注释：声明变量、对象或函数。
                                 "\u00B6"); // 中文注释：执行当前语句。
            } // 中文注释：结束当前代码块。
        } // 中文注释：结束当前代码块。

        block = block.next(); // 中文注释：更新变量或对象状态。
    } // 中文注释：结束当前代码块。
} // 中文注释：结束当前代码块。
// 绘制垂直标尺（代码宽度参考线）
void MarkdownEditor::drawRuler(QPaintEvent *e) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    const QRect rect = e->rect(); // 中文注释：声明变量、对象或函数。
    const QFont font = currentCharFormat().font(); // 中文注释：声明变量、对象或函数。

    // calculate vertical offset corresponding given
    // column margin in font metrics
    int verticalOffset = qRound(QFontMetricsF(font).averageCharWidth() * rulerPos) // 中文注释：声明变量、对象或函数。
            + contentOffset().x() // 中文注释：保留当前代码结构。
            + document()->documentMargin(); // 中文注释：执行当前语句。

    // draw a ruler with color invert to background color (better readability)
    // and with 50% opacity
    QPainter p(viewport()); // 中文注释：声明变量、对象或函数。
    p.setCompositionMode(QPainter::RasterOp_SourceXorDestination); // 中文注释：执行当前语句。
    p.setPen(QColor(0xff, 0xff, 0xff)); // 中文注释：执行当前语句。
    p.setOpacity(0.5); // 中文注释：执行当前语句。

    p.drawLine(verticalOffset, rect.top(), verticalOffset, rect.bottom()); // 中文注释：执行当前语句。
} // 中文注释：结束当前代码块。
// 获取光标所在位置的单词/文本
QString MarkdownEditor::textUnderCursor() const // 中文注释：声明变量、对象或函数。
{ // 中文注释：进入当前代码块。
    QTextCursor cursor = this->textCursor(); // 中文注释：声明变量、对象或函数。
    QTextDocument *document = this->document(); // 中文注释：声明变量、对象或函数。

    // empty text if cursor at start of line
    if (cursor.atBlockStart()) { // 中文注释：判断条件是否成立。
        return QString(); // 中文注释：返回函数处理结果。
    } // 中文注释：结束当前代码块。

    cursor.clearSelection(); // 中文注释：执行当前语句。

    // move left until we find a space or reach the start of line
    while(!document->characterAt(cursor.position()-1).isSpace() && !cursor.atBlockStart()) { // 中文注释：按条件持续循环处理。
        cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor); // 中文注释：执行当前语句。
    } // 中文注释：结束当前代码块。


    return cursor.selectedText(); // 中文注释：返回函数处理结果。
} // 中文注释：结束当前代码块。
// 过滤单词长度：只保留长度大于3的单词
bool GreaterThanMinimumWordLength(const QString &word) // 中文注释：声明变量、对象或函数。
{ // 中文注释：进入当前代码块。
    static const int MINIMUM_WORD_LENGTH = 3; // 中文注释：声明变量、对象或函数。
    return word.length() > MINIMUM_WORD_LENGTH; // 中文注释：返回函数处理结果。
} // 中文注释：结束当前代码块。
// 提取文档中所有不重复的有效单词
QStringList MarkdownEditor::extractDistinctWordsFromDocument() const // 中文注释：声明变量、对象或函数。
{ // 中文注释：进入当前代码块。
    QStringList allWords = retrieveAllWordsFromDocument(); // 中文注释：声明变量、对象或函数。
    allWords.removeDuplicates(); // 中文注释：执行当前语句。

    QStringList words = filterWordList(allWords, GreaterThanMinimumWordLength); // 中文注释：声明变量、对象或函数。
    words.sort(Qt::CaseInsensitive); // 中文注释：执行当前语句。

    return words; // 中文注释：返回函数处理结果。
} // 中文注释：结束当前代码块。
// 从文档中提取所有单词（按非字母字符分割）
QStringList MarkdownEditor::retrieveAllWordsFromDocument() const // 中文注释：声明变量、对象或函数。
{ // 中文注释：进入当前代码块。
    return toPlainText().split(QRegularExpression("\\W+"), Qt::SkipEmptyParts); // 中文注释：返回函数处理结果。
} // 中文注释：结束当前代码块。

template <class UnaryPredicate> // 中文注释：声明模板代码。
// 函数说明：实现 MarkdownEditor::filterWordList 的核心逻辑，供当前模块调用。
QStringList MarkdownEditor::filterWordList(const QStringList &words, UnaryPredicate predicate) const // 中文注释：声明变量、对象或函数。
{ // 中文注释：进入当前代码块。
    QStringList filteredWordList; // 中文注释：声明变量、对象或函数。

    for (const QString &word : words) { // 中文注释：开始循环遍历数据。
        if (predicate(word)) // 中文注释：判断条件是否成立。
        { // 中文注释：进入当前代码块。
           filteredWordList << word; // 中文注释：声明变量、对象或函数。
        } // 中文注释：结束当前代码块。
    } // 中文注释：结束当前代码块。

    return filteredWordList; // 中文注释：返回函数处理结果。
} // 中文注释：结束当前代码块。

