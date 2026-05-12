// 文件说明：libs\peg-markdown-highlight\pmhmarkdownparser.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef PMHMARKDOWNPARSER_H
#define PMHMARKDOWNPARSER_H

#include <QMap>
class QString;

class MarkdownElement
{
public:
    enum Type
    {
        LINK,
        AUTO_LINK_URL,
        AUTO_LINK_EMAIL,
        IMAGE,
        CODE,
        HTML,
        HTML_ENTITY,
        EMPHASIZED,
        STRONG,
        LIST_BULLET,
        LIST_ENUMERATOR,
        COMMENT,
        H1,
        H2,
        H3,
        H4,
        H5,
        H6,
        BLOCKQUOTE,
        VERBATIM,
        HTMLBLOCK,
        HRULE,
        REFERENCE,
        NOTE
    };
    
    Type type;
    unsigned long start;
    unsigned long end;
};

class PmhMarkdownParser
{
public:
    QMap<MarkdownElement::Type, QList<MarkdownElement> > parseMarkdown(const QString &text);
};

#endif // PMHMARKDOWNPARSER_H


