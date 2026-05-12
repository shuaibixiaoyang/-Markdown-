// 文件说明：app\htmlpreviewgenerator.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef HTMLPREVIEWGENERATOR_H
#define HTMLPREVIEWGENERATOR_H

#include <QtCore/qthread.h>
#include <QtCore/qqueue.h>
#include <QtCore/qmutex.h>
#include <QtCore/qwaitcondition.h>

#include <converter/markdownconverter.h>
#include <template/template.h>

//后台线程把 Markdown 转换成 HTML，实时预览，绝不卡顿界面！
//类的声明
class MarkdownDocument;
class Options;

// HTML 预览生成器
// 继承 QThread → 在后台线程生成 HTML，不卡 UI
class HtmlPreviewGenerator : public QThread
{
    Q_OBJECT

public:
    //构造函数
    explicit HtmlPreviewGenerator(Options *opt, QObject *parent = nullptr);
    //判断是否支持某种转换选项
    bool isSupported(MarkdownConverter::ConverterOption option) const;

public slots:
    // 槽函数：Markdown 文本发生变化时调用
    void markdownTextChanged(const QString &text);
      // 导出完整 HTML（带样式、高亮脚本）
    QString exportHtml(const QString &styleSheet, const QString &highlightingScript);
        // 开启/关闭 数学公式支持
    void setMathSupportEnabled(bool enabled);
    //图表支持
    void setDiagramSupportEnabled(bool enabled);
    //代码高亮
    void setCodeHighlightingEnabled(bool enabled);
    //设置代码高亮的形式
    void setCodeHighlightingStyle(const QString &style);
    //重新初始化
    void markdownConverterChanged();

signals:
    //html结果生成完成
    void htmlResultReady(const QString &html);
    //目录生成完成
    void tocResultReady(const QString &toc);

protected:
    //重写线程运行函数
    virtual void run();

private:
    //从markdown生成html
    void generateHtmlFromMarkdown();
    //生成目录
    void generateTableOfContents();
    //获取转换选项
    MarkdownConverter::ConverterOptions converterOptions() const;
    //获取模板渲染选项
    Template::RenderOptions renderOptions() const;
    //计算延迟
    int calculateDelay(const QString &text);

private:
    Options *options;                // 预览相关配置
    MarkdownDocument *document;      // Markdown 文档
    MarkdownConverter *converter;    // Markdown 转换器

    QQueue<QString> tasks;           // 任务队列（存放待转换文本）
    QMutex tasksMutex;               // 互斥锁（保证队列线程安全）
    QWaitCondition bufferNotEmpty;   // 等待条件（队列空时休眠）

};

#endif // HTMLPREVIEWGENERATOR_H

