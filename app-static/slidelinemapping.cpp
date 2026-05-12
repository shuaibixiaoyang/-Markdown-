// 文件说明：app-static\slidelinemapping.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "slidelinemapping.h"

#include <QRegularExpression>

// 函数说明：实现 SlideLineMapping::build 的核心逻辑，供当前模块调用。
void SlideLineMapping::build(const QString &code)
{
    static const QRegularExpression re("\n|\r\n|\r");

    m_lineToSlide.clear();
    m_slideToLine.clear();
    int horizontal = 0;
    int vertical = 0;
    int lineNumber = 0;

    QStringList lines = code.split(re);
    m_slideToLine.insert(qMakePair(horizontal, vertical), lineNumber+1);
    for (int i = 0; i < lines.count(); ++i) {
        ++lineNumber;
        if (isHorizontalSlideSeparator(lines, i)) {
            m_lineToSlide.insert(lineNumber, qMakePair(horizontal, vertical));
            horizontal++;
            vertical = 0;
            m_slideToLine.insert(qMakePair(horizontal, vertical), lineNumber+1);
        }
        if (isVerticalSlideSeparator(lines, i)) {
            m_lineToSlide.insert(lineNumber, qMakePair(horizontal, vertical));
            vertical++;
            m_slideToLine.insert(qMakePair(horizontal, vertical), lineNumber+1);
        }
    }
    m_lineToSlide.insert(lineNumber, qMakePair(horizontal, vertical));
}

// 函数说明：实现 SlideLineMapping::lineForSlide 的核心逻辑，供当前模块调用。
int SlideLineMapping::lineForSlide(const QPair<int, int>& slide) const
{
    QMap<QPair<int, int>, int>::const_iterator it = m_slideToLine.find(slide);
    if (it != m_slideToLine.end()) {
        return it.value();
    }

    return -1;
}

// 函数说明：实现 SlideLineMapping::slideForLine 的核心逻辑，供当前模块调用。
QPair<int, int> SlideLineMapping::slideForLine(int lineNumber) const
{
    QMap<int, QPair<int, int> >::const_iterator it = m_lineToSlide.lowerBound(lineNumber);
    if (it != m_lineToSlide.end()) {
        return it.value();
    }

    return qMakePair(-1, -1);
}

QMap<int, QPair<int, int> > SlideLineMapping::lineToSlide() const
{
    return m_lineToSlide;
}

// 函数说明：实现 SlideLineMapping::slideToLine 的核心逻辑，供当前模块调用。
QMap<QPair<int, int>, int> SlideLineMapping::slideToLine() const
{
    return m_slideToLine;
}

// 函数说明：判断 SlideLineMapping 当前是否满足指定状态。
bool SlideLineMapping::isHorizontalSlideSeparator(const QStringList &lines, int lineNumber) const
{
    static const QString horizontalMarker("---");

    return lineNumber > 1 &&
           lineNumber < lines.count()-1 &&
           lines[lineNumber-1].isEmpty() &&
           lines[lineNumber] == horizontalMarker &&
           lines[lineNumber+1].isEmpty();
}

// 函数说明：判断 SlideLineMapping 当前是否满足指定状态。
bool SlideLineMapping::isVerticalSlideSeparator(const QStringList &lines, int lineNumber) const
{
    static const QString verticalMarker("--");

    return lineNumber > 1 &&
           lineNumber < lines.count()-1 &&
           lines[lineNumber-1].isEmpty() &&
           lines[lineNumber] == verticalMarker &&
           lines[lineNumber+1].isEmpty();
}

