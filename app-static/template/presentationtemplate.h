// 文件说明：app-static\template\presentationtemplate.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef PRESENTATIONTEMPLATE_H
#define PRESENTATIONTEMPLATE_H

#include "template.h"

class PresentationTemplate : public Template
{
public:
    PresentationTemplate();

    virtual QString render(const QString &body, RenderOptions options) const;
    virtual QString exportAsHtml(const QString &header, const QString &body, RenderOptions options) const;

private:
    QString buildRevealPlugins(RenderOptions options) const;

    QString presentationTemplate;
};

#endif // PRESENTATIONTEMPLATE_H

