// 文件说明：app\markdownhighlighter.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "markdownhighlighter.h"

#include <QDebug>
#include <QFile>
#include <QRegularExpression>
#include <QTextDocument>
#include <QTextLayout>

#include "pmh_parser.h"
#include "yamlheaderchecker.h"

#include "peg-markdown-highlight/definitions.h"
using PegMarkdownHighlight::HighlightingStyle;

#include "hunspell/spellchecker.h"
using hunspell::SpellChecker;

#include <QDebug>

//构造函数：初始化高亮器，拼写检查，后台线程
MarkdownHighlighter::MarkdownHighlighter(QTextDocument *document, hunspell::SpellChecker *spellChecker) :
    QSyntaxHighlighter(document),//继承Qt语法高亮基类，绑定文档
    workerThread(new HighlightWorkerThread(this)),//创建后台高亮计算线程
    spellingCheckEnabled(false),//默认关闭拼写检查
    yamlHeaderSupportEnabled(false)//默认关闭YAML头部支持
{
    //保存外部传入的拼写检查器
    this->spellChecker = spellChecker;

    // Use WaveUnderline for stable cross-platform rendering in Qt 6.
    //设置拼写错误格式：红色波浪下划线
    spellFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    spellFormat.setUnderlineColor(Qt::red);
    //连接信号：后台线程解析完成->通知高亮器上色
    connect(workerThread, &HighlightWorkerThread::resultReady,
            this, &MarkdownHighlighter::resultReady);
    //启动后台高亮线程
    workerThread->start();
}

//析构函数
MarkdownHighlighter::~MarkdownHighlighter()
{
    // stop background worker thread
    workerThread->enqueue(QString());
    workerThread->wait();
    delete workerThread;
}
//
void MarkdownHighlighter::reset()
{
    previousText.clear();
}

// 函数说明：设置 MarkdownHighlighter 的运行参数，并触发必要的界面或数据刷新。
void MarkdownHighlighter::setStyles(const QVector<PegMarkdownHighlight::HighlightingStyle> &styles)
{
    highlightingStyles = styles;
    reset();
}

// 函数说明：设置 MarkdownHighlighter 的运行参数，并触发必要的界面或数据刷新。
void MarkdownHighlighter::setSpellingCheckEnabled(bool enabled)
{
    spellingCheckEnabled = enabled;
}

// 函数说明：设置 MarkdownHighlighter 的运行参数，并触发必要的界面或数据刷新。
void MarkdownHighlighter::setYamlHeaderSupportEnabled(bool enabled)
{
    yamlHeaderSupportEnabled = enabled;
}

//对每一段文本进行语法高亮
void MarkdownHighlighter::highlightBlock(const QString &textBlock)
{
    //如果文档是空的，直接返回，不做任何处理
    if (document()->isEmpty()) {
        return;
    }

    // check spelling of passed text block
    //如果开启了拼写检查，就对当前这段文本检查拼写
    if (spellingCheckEnabled) {
        checkSpelling(textBlock);
    }
    //获取整个文档的纯文本内容
    QString text = document()->toPlainText();

    // document changed since last call?
    //如果文档内容和上次一样，直接返回，不重复解析
    if (text == previousText) {
        return;
    }

    // cut YAML headers
    //定义实际要解析的文本和文本偏移量
    QString actualText;
    unsigned long offset = 0;
        // 如果开启了 YAML 头部支持
    if (yamlHeaderSupportEnabled) {
        YamlHeaderChecker checker(text);
        actualText = checker.body();
        offset = checker.bodyOffset();
    } else {
        actualText = text;
    }

    workerThread->enqueue(actualText, offset);
   // 保存当前文本，下次用来判断是否变化
    previousText = text;
}

//对指定范围的文本应用字符格式
// pos：起始位置
// end：结束位置
// format：要应用的格式（颜色、下划线、字体等）
// merge：是否与已有格式合并
/*
这个函数就是给文本上色的底层工具：
接收起始位置、结束位置、颜色 / 样式
自动跨行处理
把格式应用到对应文字
支持保留原有格式或覆盖*/
// 函数说明：应用 MarkdownHighlighter 当前配置，让编辑器或预览立即生效。
void MarkdownHighlighter::applyFormat(unsigned long pos, unsigned long end,
                                      QTextCharFormat format, bool merge)
{
    // The QTextDocument contains an additional single paragraph separator (unicode 0x2029).
    // https://bugreports.qt-project.org/browse/QTBUG-4841
    //QTextDocument会额外包含一个段落分隔符，这里做安全判断
    //如果文档为空，则直接返回
    if (document()->characterCount() == 0) {
        return;
    }
    //获取文档最大有效偏移量
    unsigned long max_offset = document()->characterCount() - 1;
    //如果范围无效，直接返回
    if (end <= pos || max_offset < pos) {
        return;
    }
    //如果结束位置超出文档最大范围,修正到最大位置
    if (max_offset < end) {
        end = max_offset;
    }

    // "The QTextLayout object can only be modified from the
    // documentChanged implementation of a QAbstractTextDocumentLayout
    // subclass. Any changes applied from the outside cause undefined
    // behavior." -- we are breaking this rule here. There might be
    // a better (more correct) way to do this.
    // 注意：
    // Qt官方规定QTextLayout只能在特定函数内修改
    // 这里直接修改是为了实现高效高亮，属于实用方案

    //找到起始位置和结束位置所在的行号
    int startBlockNum = document()->findBlock(pos).blockNumber();
    int endBlockNum = document()->findBlock(end).blockNumber();
    //遍历从起始行到结束行
    for (int j = startBlockNum; j <= endBlockNum; j++)
    {
        //获取第j行的文本块
        QTextBlock block = document()->findBlockByNumber(j);
        //如果行无效，跳过
        if (!block.isValid()) continue;
        //获取改行的文本布局对象
        QTextLayout *layout = block.layout();
        if (!layout) continue;
        //该行在整个文档中的起始位置
        int blockpos = block.position();
        //定义格式范围
        QTextLayout::FormatRange r;
        r.format = format;
        //格式范围列表
        QList<QTextLayout::FormatRange> list;
        //如果需要合并格式，先读取已有格式
        if (merge) {
            list = layout->formats();
        }
        //处理第一行
        if (j == startBlockNum) {
            //格式在本行所在的起始位置
            r.start = pos - blockpos;
            //格式长度
            r.length = (startBlockNum == endBlockNum)
                        ? end - pos
                        : block.length() - r.start;
        }
        //处理最后一行
        else if (j == endBlockNum) {
            r.start = 0;
            r.length = end - blockpos;
        }
        //处理中间所有行
            else {
            r.start = 0;
            r.length = block.length();
        }
        //将格式范围加入列表
        list.append(r);
        layout->setFormats(list);
    }
}

//对当前文本块进行检查
//拼写错误的单词会显示红色波浪线
void MarkdownHighlighter::checkSpelling(const QString &textBlock)
{
    //使用正则表达式分割文本
    QStringList wordList = textBlock.split(QRegularExpression("\\W+"), Qt::SkipEmptyParts);
    //记录单词在文本中的位置索引
    int index = 0;
    //遍历所有提取出来的单词
    for (const QString &word : wordList) {
        //从当前索引位置开始，查找这个单词在文本中的位置
        index = textBlock.indexOf(word, index);
        //判断单词错误
        if (!spellChecker->isCorrect(word)) {
            //设置红色波浪线
            setFormat(index, word.length(), spellFormat);
        }
        //准备查找下一个
        index += word.length();
    }
}

// 后台线程解析完成，返回语法元素，执行最终上色
// elements：解析好的 Markdown 语法结构
// base_offset：文本偏移量（处理 YAML 头部时用到）
//执行最终上色
void MarkdownHighlighter::resultReady(pmh_element **elements, unsigned long base_offset)
{
    //如果没有解析结果，直接返回
    if (!elements) {
        qDebug() << "elements is null";
        return;
    }

    // 如果有偏移量（比如跳过了 YAML 头部）
    // 先清除偏移量之前的所有格式，避免重叠
    // clear any format before base_offset
    if (base_offset > 0) {
        applyFormat(0, base_offset - 1, QTextCharFormat(), false);
    }

    // apply highlight results
    //遍历所有高亮样式，给对应语法元素上色
    for (int i = 0; i < highlightingStyles.size(); i++) {
        HighlightingStyle style = highlightingStyles.at(i);
        pmh_element *elem_cursor = elements[style.type];
        //将该类型的全部上色
        while (elem_cursor != NULL) {
            unsigned long pos = elem_cursor->pos + base_offset;
            unsigned long end = elem_cursor->end + base_offset;

            QTextCharFormat format = style.format;
            if (/*_makeLinksClickable
                &&*/ (elem_cursor->type == pmh_LINK
                    || elem_cursor->type == pmh_AUTO_LINK_URL
                    || elem_cursor->type == pmh_AUTO_LINK_EMAIL
                    || elem_cursor->type == pmh_REFERENCE)
                && elem_cursor->address != NULL)
            {
                QString address(elem_cursor->address);
                if (elem_cursor->type == pmh_AUTO_LINK_EMAIL && !address.startsWith("mailto:"))
                    address = "mailto:" + address;
                format.setAnchor(true);
                format.setAnchorHref(address);
                format.setToolTip(address);
            }
            applyFormat(pos, end, format, true);

            elem_cursor = elem_cursor->next;
        }
    }

    // mark complete document as dirty
     // 标记整个文档内容已变更，强制刷新显示
    document()->markContentsDirty(0, document()->characterCount());

    // free highlighting elements
    // 释放解析结果的内存
    pmh_free_elements(elements);
}

