// 文件说明：app\markdowneditor.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef MARKDOWNEDITOR_H // 中文注释：开始头文件保护，避免重复包含。
#define MARKDOWNEDITOR_H // 中文注释：定义头文件保护宏。

#include <qplaintextedit.h> // 中文注释：引入当前文件需要的依赖头文件。

//避免头文件循环依赖，提升编译效率
class Dictionary; // 中文注释：声明类类型或前置声明类。
namespace hunspell { // 中文注释：声明命名空间范围。
class SpellChecker;//拼写检查器//前置声明
} // 中文注释：结束当前代码块。
class MarkdownHighlighter;//语法高亮前置声明
class SnippetCompleter;//代码片段补全器前置声明

/**
 * @brief Markdown 编辑器核心类
 * 继承自 QPlainTextEdit，实现专业 Markdown 编辑功能：
 * 行号显示、语法高亮、拼写检查、代码片段补全、标尺、特殊字符显示等
 */

class MarkdownEditor : public QPlainTextEdit // 中文注释：声明类类型或前置声明类。
{ // 中文注释：进入当前代码块。
    Q_OBJECT // 中文注释：启用 Qt 元对象系统能力。
public: // 中文注释：声明类成员的访问权限区域。
    explicit MarkdownEditor(QWidget *parent = nullptr); // 中文注释：声明变量、对象或函数。
    ~MarkdownEditor(); // 中文注释：执行当前语句。

    //绘制行号区域
    void lineNumberAreaPaintEvent(QPaintEvent *event); // 中文注释：声明变量、对象或函数。
    //计算行号区域所需要的宽度
    int lineNumberAreaWidth(); // 中文注释：声明变量、对象或函数。

    //重置语法高亮
    void resetHighlighting(); // 中文注释：声明变量、对象或函数。
    //从样式表文件加载编辑器样式
    void loadStyleFromStylesheet(const QString &fileName); // 中文注释：声明变量、对象或函数。
    //统计文档中单词的数量
    int countWords() const; // 中文注释：声明变量、对象或函数。
    //设置是否显示特殊字符（空格，换行）
    void setShowSpecialCharacters(bool enabled); // 中文注释：声明变量、对象或函数。
    //开启或关闭拼写检查
    void setSpellingCheckEnabled(bool enabled); // 中文注释：声明变量、对象或函数。
    //设置拼写检查使用的词典
    void setSpellingDictionary(const Dictionary &dictionary); // 中文注释：声明变量、对象或函数。
    //开启或关闭YAML头部语法支持
    void setYamlHeaderSupportEnabled(bool enabled); // 中文注释：声明变量、对象或函数。
    //设置代码片段自动补全器
    void setSnippetCompleter(SnippetCompleter *completer); // 中文注释：声明变量、对象或函数。
    //光标跳转到指定行
    void gotoLine(int line); // 中文注释：声明变量、对象或函数。

signals: // 中文注释：声明 Qt 信号区域。
    //拖拽文件时发出信号，传递文件路径
    void loadDroppedFile(const QString &fileName); // 中文注释：声明变量、对象或函数。

protected: // 中文注释：声明类成员的访问权限区域。
    //重写绘制事件 用于绘制标尺，换行标记
    void paintEvent(QPaintEvent *e) override; // 中文注释：声明变量、对象或函数。
    //重写窗口大小改变事件，同步更新行号区域大小
    void resizeEvent(QResizeEvent *event) override; // 中文注释：声明变量、对象或函数。
    //重写键盘事件，处理快捷键，Tab,补全操作
    void keyPressEvent(QKeyEvent *e) override; // 中文注释：声明变量、对象或函数。
    //判断是否可以粘贴传入的数据
    bool canInsertFromMimeData(const QMimeData *source) const override; // 中文注释：声明变量、对象或函数。
    //执行粘贴操作
    void insertFromMimeData(const QMimeData *source) override; // 中文注释：声明变量、对象或函数。

public slots: // 中文注释：声明 Qt 槽函数区域。
    //制表符宽度改变时更新
    void tabWidthChanged(int tabWidth); // 中文注释：声明变量、对象或函数。
    //编辑器字体改变时更新
    void editorFontChanged(const QFont &font); // 中文注释：声明变量、对象或函数。
    //开启/关闭右侧标尺
    void rulerEnabledChanged(bool enabled); // 中文注释：声明变量、对象或函数。
    //标尺位置改变时更新
    void rulerPosChanged(int pos); // 中文注释：声明变量、对象或函数。

private slots: // 中文注释：声明 Qt 槽函数区域。
    // 根据行数更新行号区域宽度
    void updateLineNumberAreaWidth(int newBlockCount); // 中文注释：声明变量、对象或函数。
     // 更新行号区域的显示内容
    void updateLineNumberArea(const QRect &rect, int dy); // 中文注释：声明变量、对象或函数。
    // 显示右键菜单
    void showContextMenu(const QPoint &pos); // 中文注释：声明变量、对象或函数。
    //用拼写建议替换错误单词
    void replaceWithSuggestion(); // 中文注释：声明变量、对象或函数。
    //执行自动补全操作
    void performCompletion(); // 中文注释：声明变量、对象或函数。
    //插入代码片段
    void insertSnippet(const QString &completionPrefix, const QString &completion, int newCursorPos); // 中文注释：声明变量、对象或函数。
    //将单词添加到用户自定义词典
    void addWordToUserWordlist(); // 中文注释：声明变量、对象或函数。

private: // 中文注释：声明类成员的访问权限区域。
    //判断粘贴的内容是否是本地文件链接
    bool isUrlToLocalFile(const QMimeData *source) const; // 中文注释：声明变量、对象或函数。
    //绘制换行符标记
    void drawLineEndMarker(QPaintEvent *e); // 中文注释：声明变量、对象或函数。
    //绘制垂直参考标尺
    void drawRuler(QPaintEvent *e); // 中文注释：声明变量、对象或函数。
    //获取光标所在位置的文本
    QString textUnderCursor() const; // 中文注释：声明变量、对象或函数。
    // 提取文档中所有不重复的单词
    QStringList extractDistinctWordsFromDocument() const; // 中文注释：声明变量、对象或函数。
// 获取文档中所有单词（包含重复）
    QStringList retrieveAllWordsFromDocument() const; // 中文注释：声明变量、对象或函数。
     // 单词列表过滤模板函数
    template <class UnaryPredicate> // 中文注释：声明模板代码。
    QStringList filterWordList(const QStringList &words, UnaryPredicate predicate) const; // 中文注释：声明变量、对象或函数。

private: // 中文注释：声明类成员的访问权限区域。
    QWidget *lineNumberArea;//行号显示区域
    MarkdownHighlighter *highlighter;// Markdown 语法高亮对象
    hunspell::SpellChecker *spellChecker;  //拼写检查器
    SnippetCompleter *completer;//代码片段补全
    bool showHardLinebreaks;//是否显示硬换行标记
    bool rulerEnabled;//是否显示标尺
    int rulerPos;//标尺显示
}; // 中文注释：结束类型或作用域声明。

#endif // MARKDOWNEDITOR_H

