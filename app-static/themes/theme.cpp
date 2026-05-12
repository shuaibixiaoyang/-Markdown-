// 文件说明：app-static\themes\theme.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "theme.h"

// 函数说明：构造 Theme 对象，初始化本模块需要的状态、界面和资源。
Theme::Theme(const QString &name, 
             const QString &markdownHighlighting,
             const QString &codeHighlighting,
             const QString &previewStylesheet,
             bool builtIn) :
    m_name(name),
    m_markdownHighlighting(markdownHighlighting),
    m_codeHighlighting(codeHighlighting),
    m_previewStylesheet(previewStylesheet),
    m_builtIn(builtIn)
{
    checkInvariants();
}

// 函数说明：实现 Theme::checkInvariants 的核心逻辑，供当前模块调用。
void Theme::checkInvariants() const
{
    if (m_name.isEmpty()) {
        throw std::runtime_error("theme name must not be empty");
    }
    if (m_markdownHighlighting.isEmpty()) {
        throw std::runtime_error("markdown highlighting style must not be empty");
    }
    if (m_codeHighlighting.isEmpty()) {
        throw std::runtime_error("code highlighting style must not be empty");
    }
    if (m_previewStylesheet.isEmpty()) {
        throw std::runtime_error("preview stylesheet must not be empty");
    }
}

