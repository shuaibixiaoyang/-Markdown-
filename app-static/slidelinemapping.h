// 文件说明：app-static\slidelinemapping.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef SLIDELINEMAPPING_H
#define SLIDELINEMAPPING_H

#include <QMap>
#include <QString>


class SlideLineMapping
{
public:
    void build(const QString &code);

    int lineForSlide(const QPair<int, int>& slide) const;
    QPair<int, int> slideForLine(int lineNumber) const;

    QMap<int, QPair<int, int> > lineToSlide() const;
    QMap<QPair<int, int>, int> slideToLine() const;

private:
    bool isHorizontalSlideSeparator(const QStringList &lines, int lineNumber) const;
    bool isVerticalSlideSeparator(const QStringList &lines, int lineNumber) const;

    QMap<int, QPair<int, int> > m_lineToSlide;
    QMap<QPair<int, int>, int> m_slideToLine;
};

#endif // SLIDELINEMAPPING_H

