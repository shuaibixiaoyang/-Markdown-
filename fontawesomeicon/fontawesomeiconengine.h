// 文件说明：fontawesomeicon\fontawesomeiconengine.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef FONTAWESOMEICONENGINE_H
#define FONTAWESOMEICONENGINE_H

#include <QIconEngine>

class FontAwesomeIconEngine : public QIconEngine
{
public:
    FontAwesomeIconEngine();
    FontAwesomeIconEngine(const QString &iconName);

    void setIconName(const QString &iconName);

    QIconEngine *clone() const override;
    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override;
    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override;

private:
    void loadFont();

    QString iconName;

    static int fontId;
    static QString fontName;
    static QHash<QString, QChar> namedCodepoints;
};

#endif // FONTAWESOMEICONENGINE_H

