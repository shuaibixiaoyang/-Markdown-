// 文件说明：app-static\rendering\mathrenderer.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef MATHRENDERER_H
#define MATHRENDERER_H

#include <QObject>
#include <QString>
#include <QImage>
#include <QPainter>
#include <QSizeF>

class MathRenderer : public QObject
{
    Q_OBJECT

public:
    explicit MathRenderer(QObject *parent = nullptr);
    ~MathRenderer();

    enum class ElementType {
        Text,
        Fraction,
        SquareRoot,
        NthRoot,
        Superscript,
        Subscript,
        Matrix,
        Group,
        Symbol
    };

    struct MathElement {
        ElementType type;
        QString content;
        QVector<MathElement> children;
        int rootIndex;  // For nth root
        bool isNestedContent;
        QSizeF size;
        QPointF position;
    };

    // Main API
    MathElement parse(const QString &latex);
    QImage render(const MathElement &element, qreal fontSize = 14.0);
    
    // Size calculation
    QSizeF measureElement(QPainter &painter, const MathElement &element, qreal fontSize);

private:
    // Parsing helpers
    MathElement parseGroup(const QString &latex, int &pos);
    MathElement parseFraction(const QString &latex, int &pos);
    MathElement parseSquareRoot(const QString &latex, int &pos);
    MathElement parseNthRoot(const QString &latex, int &pos);
    MathElement parseSuperscript(const QString &latex, int &pos);
    MathElement parseSubscript(const QString &latex, int &pos);
    MathElement parseMatrix(const QString &latex, int &pos);
    
    // Rendering helpers
    void renderElement(QPainter &painter, const MathElement &element, 
                      qreal x, qreal y, qreal fontSize);
    void renderFraction(QPainter &painter, const MathElement &element, 
                       qreal x, qreal y, qreal fontSize);
    void renderSquareRoot(QPainter &painter, const MathElement &element, 
                         qreal x, qreal y, qreal fontSize);
    void renderMatrix(QPainter &painter, const MathElement &element, 
                     qreal x, qreal y, qreal fontSize);
};

#endif // MATHRENDERER_H

