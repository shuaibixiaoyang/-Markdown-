// 文件说明：libs\peg-markdown-highlight\styleparser.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef STYLEPARSER_H
#define STYLEPARSER_H

#include "definitions.h"
#include <pmh_styleparser.h>


namespace PegMarkdownHighlight
{

class StyleParser
{
public:
    explicit StyleParser(const QString& styleSheet);
    ~StyleParser();

    QVector<HighlightingStyle> highlightingStyles(QFont baseFont) const;
    QPalette editorPalette() const;

    void handleStyleParsingError(char *errorMessage, int lineNumber);

private:
    pmh_style_collection *styles;
    QList<QPair<int, QString> > styleParsingErrorList;
};

}

#endif // STYLEPARSER_H

