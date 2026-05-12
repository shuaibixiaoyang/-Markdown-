// 文件说明：app-static\template\template.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef TEMPLATE_H
#define TEMPLATE_H

#include <QString>

class Template
{
public:
    enum RenderOption {
        ScrollbarSynchronization = 0x00000001,
        MathSupport              = 0x00000002,
        CodeHighlighting         = 0x00000004,
        DiagramSupport           = 0x00000008,
        MathInlineSupport        = 0x00000010
    };
    Q_DECLARE_FLAGS(RenderOptions, RenderOption)

    virtual ~Template() {}

    QString codeHighlightingStyle() const { return highlightingStyle; }
    void setCodeHighlightingStyle(const QString &style) { highlightingStyle = style; }

    virtual QString render(const QString &body, RenderOptions options) const = 0;
    virtual QString exportAsHtml(const QString &header, const QString &body, RenderOptions options) const = 0;

private:
    QString highlightingStyle;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Template::RenderOptions)

#endif // TEMPLATE_H

