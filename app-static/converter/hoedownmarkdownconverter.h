// 文件说明：app-static\converter\hoedownmarkdownconverter.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef HOEDOWNMARKDOWNCONVERTER_H
#define HOEDOWNMARKDOWNCONVERTER_H

#include "markdownconverter.h"

class HoedownMarkdownConverter : public MarkdownConverter
{
public:
    HoedownMarkdownConverter();

    virtual MarkdownDocument *createDocument(const QString &text, ConverterOptions options);
    virtual QString renderAsHtml(MarkdownDocument *document);
    virtual QString renderAsTableOfContents(MarkdownDocument *document);

    virtual Template *templateRenderer() const;

    virtual ConverterOptions supportedOptions() const;

private:
    unsigned long translateConverterOptions(ConverterOptions options) const;
};

#endif // HOEDOWNMARKDOWNCONVERTER_H

