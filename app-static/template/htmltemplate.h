// 文件说明：app-static\template\htmltemplate.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef HTMLTEMPLATE_H
#define HTMLTEMPLATE_H

#include "template.h"

class HtmlTemplate : public Template
{
public:
    HtmlTemplate();
	explicit HtmlTemplate(const QString &templateString);

    virtual QString render(const QString &body, RenderOptions options) const;
    virtual QString exportAsHtml(const QString &header, const QString &body, RenderOptions options) const;

private:
    QString renderAsHtml(const QString &header, const QString &body, RenderOptions options) const;
    QString buildHtmlHeader(RenderOptions options) const;
    void convertDiagramCodeSectionToDiv(QString &body) const;

    QString htmlTemplate;
};

#endif // HTMLTEMPLATE_H

