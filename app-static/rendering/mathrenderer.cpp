// 文件说明：app-static\rendering\mathrenderer.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "mathrenderer.h"
#include "renderingstylemanager.h"

#include <QFontMetrics>
#include <QPainterPath>
#include <QtMath>

// 函数说明：构造 MathRenderer 对象，初始化本模块需要的状态、界面和资源。
MathRenderer::MathRenderer(QObject *parent)
    : QObject(parent)
{
}

MathRenderer::~MathRenderer() = default;

// 函数说明：解析输入内容，转换为 MathRenderer 后续处理使用的数据结构。
MathRenderer::MathElement MathRenderer::parse(const QString &latex)
{
    MathElement root;
    root.type = ElementType::Group;
    
    int pos = 0;
    while (pos < latex.length()) {
        QChar c = latex[pos];
        
        if (c == '\\') {
            // Command
            int cmdStart = pos + 1;
            while (pos + 1 < latex.length() && latex[pos + 1].isLetter()) {
                pos++;
            }
            QString cmd = latex.mid(cmdStart, pos - cmdStart + 1);
            pos++;
            
            if (cmd == "frac") {
                root.children.append(parseFraction(latex, pos));
            } else if (cmd == "sqrt") {
                root.children.append(parseSquareRoot(latex, pos));
            } else if (cmd == "begin") {
                // Check for matrix
                if (latex.mid(pos).startsWith("{matrix}") || 
                    latex.mid(pos).startsWith("{pmatrix}") ||
                    latex.mid(pos).startsWith("{bmatrix}")) {
                    root.children.append(parseMatrix(latex, pos));
                }
            } else {
                // Symbol command
                MathElement symbol;
                symbol.type = ElementType::Symbol;
                symbol.content = cmd;
                root.children.append(symbol);
            }
        } else if (c == '{') {
            root.children.append(parseGroup(latex, pos));
        } else if (c == '^') {
            pos++;
            root.children.append(parseSuperscript(latex, pos));
        } else if (c == '_') {
            pos++;
            root.children.append(parseSubscript(latex, pos));
        } else if (!c.isSpace()) {
            MathElement text;
            text.type = ElementType::Text;
            text.content = c;
            root.children.append(text);
            pos++;
        } else {
            pos++;
        }
    }
    
    return root;
}

// 函数说明：解析输入内容，转换为 MathRenderer 后续处理使用的数据结构。
MathRenderer::MathElement MathRenderer::parseGroup(const QString &latex, int &pos)
{
    MathElement group;
    group.type = ElementType::Group;
    
    if (pos < latex.length() && latex[pos] == '{') {
        pos++;
        int depth = 1;
        int start = pos;
        
        while (pos < latex.length() && depth > 0) {
            if (latex[pos] == '{') depth++;
            else if (latex[pos] == '}') depth--;
            pos++;
        }
        
        QString content = latex.mid(start, pos - start - 1);
        group = parse(content);
    }
    
    return group;
}

// 函数说明：解析输入内容，转换为 MathRenderer 后续处理使用的数据结构。
MathRenderer::MathElement MathRenderer::parseFraction(const QString &latex, int &pos)
{
    MathElement frac;
    frac.type = ElementType::Fraction;
    
    // Parse numerator
    if (pos < latex.length() && latex[pos] == '{') {
        frac.children.append(parseGroup(latex, pos));
    }
    
    // Parse denominator
    if (pos < latex.length() && latex[pos] == '{') {
        frac.children.append(parseGroup(latex, pos));
    }
    
    return frac;
}

// 函数说明：解析输入内容，转换为 MathRenderer 后续处理使用的数据结构。
MathRenderer::MathElement MathRenderer::parseSquareRoot(const QString &latex, int &pos)
{
    MathElement sqrt;
    sqrt.type = ElementType::SquareRoot;
    sqrt.rootIndex = 2;
    
    // Check for nth root
    if (pos < latex.length() && latex[pos] == '[') {
        pos++;
        int start = pos;
        while (pos < latex.length() && latex[pos] != ']') {
            pos++;
        }
        sqrt.rootIndex = latex.mid(start, pos - start).toInt();
        if (sqrt.rootIndex == 0) sqrt.rootIndex = 2;
        pos++;  // skip ]
        sqrt.type = ElementType::NthRoot;
    }
    
    // Parse content
    if (pos < latex.length() && latex[pos] == '{') {
        MathElement content = parseGroup(latex, pos);
        content.isNestedContent = true;
        sqrt.children.append(content);
    }
    
    return sqrt;
}

// 函数说明：解析输入内容，转换为 MathRenderer 后续处理使用的数据结构。
MathRenderer::MathElement MathRenderer::parseNthRoot(const QString &latex, int &pos)
{
    return parseSquareRoot(latex, pos);
}

// 函数说明：解析输入内容，转换为 MathRenderer 后续处理使用的数据结构。
MathRenderer::MathElement MathRenderer::parseSuperscript(const QString &latex, int &pos)
{
    MathElement sup;
    sup.type = ElementType::Superscript;
    
    if (pos < latex.length()) {
        if (latex[pos] == '{') {
            sup.children.append(parseGroup(latex, pos));
        } else {
            MathElement text;
            text.type = ElementType::Text;
            text.content = latex[pos];
            sup.children.append(text);
            pos++;
        }
    }
    
    return sup;
}

// 函数说明：解析输入内容，转换为 MathRenderer 后续处理使用的数据结构。
MathRenderer::MathElement MathRenderer::parseSubscript(const QString &latex, int &pos)
{
    MathElement sub;
    sub.type = ElementType::Subscript;
    
    if (pos < latex.length()) {
        if (latex[pos] == '{') {
            sub.children.append(parseGroup(latex, pos));
        } else {
            MathElement text;
            text.type = ElementType::Text;
            text.content = latex[pos];
            sub.children.append(text);
            pos++;
        }
    }
    
    return sub;
}

// 函数说明：解析输入内容，转换为 MathRenderer 后续处理使用的数据结构。
MathRenderer::MathElement MathRenderer::parseMatrix(const QString &latex, int &pos)
{
    MathElement matrix;
    matrix.type = ElementType::Matrix;
    
    // Skip to end of matrix
    int endPos = latex.indexOf("\\end{", pos);
    if (endPos != -1) {
        pos = endPos;
        while (pos < latex.length() && latex[pos] != '}') pos++;
        pos++;
    }
    
    return matrix;
}

// 函数说明：渲染 MathRenderer 的显示内容或导出片段。
QImage MathRenderer::render(const MathElement &element, qreal fontSize)
{
    const auto &style = RenderingStyleManager::instance();
    
    QImage tempImage(1, 1, QImage::Format_ARGB32);
    QPainter tempPainter(&tempImage);
    
    QSizeF size = measureElement(tempPainter, element, fontSize);
    tempPainter.end();
    
    int width = qCeil(size.width()) + 20;
    int height = qCeil(size.height()) + 20;
    
    QImage image(width, height, QImage::Format_ARGB32_Premultiplied);
    image.fill(style.colorScheme().backgroundColor);
    
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    
    renderElement(painter, element, 10, height / 2, fontSize);
    
    painter.end();
    return image;
}

// 函数说明：实现 MathRenderer::measureElement 的核心逻辑，供当前模块调用。
QSizeF MathRenderer::measureElement(QPainter &painter, const MathElement &element, qreal fontSize)
{
    const auto &style = RenderingStyleManager::instance();
    QFont font = style.fontSettings().mathFont;
    font.setPointSizeF(fontSize);
    painter.setFont(font);
    QFontMetrics fm(font);
    
    switch (element.type) {
        case ElementType::Text:
            return QSizeF(fm.horizontalAdvance(element.content), fm.height());
            
        case ElementType::Fraction: {
            if (element.children.size() >= 2) {
                QSizeF numSize = measureElement(painter, element.children[0], fontSize * 0.8);
                QSizeF denSize = measureElement(painter, element.children[1], fontSize * 0.8);
                qreal width = qMax(numSize.width(), denSize.width()) + 10;
                qreal height = numSize.height() + denSize.height() + 8;
                return QSizeF(width, height);
            }
            return QSizeF(20, 30);
        }
        
        case ElementType::SquareRoot:
        case ElementType::NthRoot: {
            QSizeF contentSize(20, fontSize);
            if (!element.children.isEmpty()) {
                contentSize = measureElement(painter, element.children[0], fontSize);
            }
            return QSizeF(contentSize.width() + 15, contentSize.height() + 5);
        }
        
        case ElementType::Superscript:
        case ElementType::Subscript: {
            QSizeF childSize(10, fontSize * 0.7);
            if (!element.children.isEmpty()) {
                childSize = measureElement(painter, element.children[0], fontSize * 0.7);
            }
            return childSize;
        }
        
        case ElementType::Group: {
            qreal width = 0;
            qreal height = fontSize;
            for (const MathElement &child : element.children) {
                QSizeF childSize = measureElement(painter, child, fontSize);
                width += childSize.width();
                height = qMax(height, childSize.height());
            }
            return QSizeF(width, height);
        }
        
        case ElementType::Symbol:
            return QSizeF(fm.horizontalAdvance(element.content), fm.height());
            
        default:
            return QSizeF(10, fontSize);
    }
}

// 函数说明：渲染 MathRenderer 的显示内容或导出片段。
void MathRenderer::renderElement(QPainter &painter, const MathElement &element, 
                                 qreal x, qreal y, qreal fontSize)
{
    const auto &style = RenderingStyleManager::instance();
    QFont font = style.fontSettings().mathFont;
    font.setPointSizeF(fontSize);
    painter.setFont(font);
    painter.setPen(style.colorScheme().nodeText);
    
    switch (element.type) {
        case ElementType::Text:
            painter.drawText(QPointF(x, y), element.content);
            break;
            
        case ElementType::Fraction:
            renderFraction(painter, element, x, y, fontSize);
            break;
            
        case ElementType::SquareRoot:
        case ElementType::NthRoot:
            renderSquareRoot(painter, element, x, y, fontSize);
            break;
            
        case ElementType::Group: {
            qreal currentX = x;
            for (const MathElement &child : element.children) {
                renderElement(painter, child, currentX, y, fontSize);
                QSizeF size = measureElement(painter, child, fontSize);
                currentX += size.width();
            }
            break;
        }
        
        case ElementType::Superscript:
            if (!element.children.isEmpty()) {
                renderElement(painter, element.children[0], x, y - fontSize * 0.4, fontSize * 0.7);
            }
            break;
            
        case ElementType::Subscript:
            if (!element.children.isEmpty()) {
                renderElement(painter, element.children[0], x, y + fontSize * 0.3, fontSize * 0.7);
            }
            break;
            
        case ElementType::Symbol:
            painter.drawText(QPointF(x, y), element.content);
            break;
            
        default:
            break;
    }
}

// 函数说明：渲染 MathRenderer 的显示内容或导出片段。
void MathRenderer::renderFraction(QPainter &painter, const MathElement &element, 
                                  qreal x, qreal y, qreal fontSize)
{
    if (element.children.size() < 2) return;
    
    const auto &style = RenderingStyleManager::instance();
    
    QSizeF numSize = measureElement(painter, element.children[0], fontSize * 0.8);
    QSizeF denSize = measureElement(painter, element.children[1], fontSize * 0.8);
    
    qreal width = qMax(numSize.width(), denSize.width()) + 10;
    
    // Draw numerator
    qreal numX = x + (width - numSize.width()) / 2;
    renderElement(painter, element.children[0], numX, y - 5, fontSize * 0.8);
    
    // Draw fraction line
    painter.setPen(QPen(style.colorScheme().nodeText, 1));
    painter.drawLine(QPointF(x, y), QPointF(x + width, y));
    
    // Draw denominator
    qreal denX = x + (width - denSize.width()) / 2;
    renderElement(painter, element.children[1], denX, y + denSize.height(), fontSize * 0.8);
}

// 函数说明：渲染 MathRenderer 的显示内容或导出片段。
void MathRenderer::renderSquareRoot(QPainter &painter, const MathElement &element, 
                                    qreal x, qreal y, qreal fontSize)
{
    const auto &style = RenderingStyleManager::instance();
    
    QSizeF contentSize(20, fontSize);
    if (!element.children.isEmpty()) {
        contentSize = measureElement(painter, element.children[0], fontSize);
    }
    
    // Draw radical symbol
    QPainterPath radical;
    qreal h = contentSize.height() + 5;
    radical.moveTo(x, y);
    radical.lineTo(x + 5, y + 3);
    radical.lineTo(x + 8, y + h);
    radical.lineTo(x + 12, y - h / 2);
    radical.lineTo(x + 12 + contentSize.width() + 3, y - h / 2);
    
    painter.setPen(QPen(style.colorScheme().nodeText, 1.5));
    painter.drawPath(radical);
    
    // Draw content
    if (!element.children.isEmpty()) {
        renderElement(painter, element.children[0], x + 14, y, fontSize);
    }
    
    // Draw nth root index if present
    if (element.type == ElementType::NthRoot && element.rootIndex != 2) {
        QFont smallFont = style.fontSettings().mathFont;
        smallFont.setPointSizeF(fontSize * 0.6);
        painter.setFont(smallFont);
        painter.drawText(QPointF(x, y - h / 2 - 2), QString::number(element.rootIndex));
    }
}

// 函数说明：渲染 MathRenderer 的显示内容或导出片段。
void MathRenderer::renderMatrix(QPainter &painter, const MathElement &element, 
                                qreal x, qreal y, qreal fontSize)
{
    // Minimal matrix rendering
    const auto &style = RenderingStyleManager::instance();
    painter.setPen(style.colorScheme().nodeText);
    painter.drawText(QPointF(x, y), "[matrix]");
}

