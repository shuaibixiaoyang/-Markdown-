// 文件说明：libs\peg-markdown-highlight\definitions.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <pmh_definitions.h>
#include <QtGui/QTextCharFormat>


namespace PegMarkdownHighlight
{

struct HighlightingStyle
{
    pmh_element_type type;
    QTextCharFormat format;
};

}

#endif // DEFINITIONS_H

