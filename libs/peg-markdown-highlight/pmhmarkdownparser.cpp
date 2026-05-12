// 文件说明：libs\peg-markdown-highlight\pmhmarkdownparser.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "pmhmarkdownparser.h"

#include <QString>

#include <pmh_definitions.h>
#include <pmh_parser.h>

QMap<MarkdownElement::Type, QList<MarkdownElement> > PmhMarkdownParser::parseMarkdown(const QString &text)
{
    // parse markdown and generate syntax elements
    pmh_element **elements;
    pmh_markdown_to_elements(text.toUtf8().data(), pmh_EXT_NONE, &elements);

    QMap<MarkdownElement::Type, QList<MarkdownElement> > elementMap;
    for (int i = 0; i < pmh_NUM_LANG_TYPES; i++) {
        if (elements[i] != NULL) {
            MarkdownElement::Type type = (MarkdownElement::Type)i;
            QList<MarkdownElement> list;

            pmh_element *element = elements[i];
            while (element != NULL) {
                MarkdownElement e;
                e.type = type;
                e.start = element->pos;
                e.end = element->end;
                list.append(e);
                element = element->next;
            }
            elementMap[type] = list;
        }
    }
    return elementMap;
}


