// 文件说明：app-static\themes\stylemanager.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef STYLEMANAGER_H
#define STYLEMANAGER_H

#include <QMap>
#include <QString>
#include "theme.h"


class StyleManager
{
public:
    void insertCustomPreviewStylesheet(const QString &styleName, const QString &stylePath);

    static QString markdownHighlightingPath(const Theme &theme);
    static QString codeHighlightingPath(const Theme &theme);
    static QString previewStylesheetPath(const Theme &theme);

private:
    static QMap<QString, QString> customPreviewStylesheets;
};

#endif // STYLEMANAGER_H


