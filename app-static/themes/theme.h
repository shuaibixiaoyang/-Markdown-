// 文件说明：app-static\themes\theme.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef THEME_H
#define THEME_H

#include <QString>
#include <stdexcept>


class Theme
{
public:
    Theme(const QString &name, 
          const QString &markdownHighlighting,
          const QString &codeHighlighting,
          const QString &previewStylesheet,
          bool builtIn = false);

    QString name() const { return m_name; }

    QString markdownHighlighting() const { return m_markdownHighlighting; }

    QString codeHighlighting() const { return m_codeHighlighting; }

    QString previewStylesheet() const { return m_previewStylesheet; }

    bool isBuiltIn() const { return m_builtIn; }

    bool operator<(const Theme &rhs) const
    {
        return m_name < rhs.name();
    }

    bool operator ==(const Theme &rhs) const
    {
        return m_name == rhs.name();
    }

private:
    void checkInvariants() const;

private:
    QString m_name;
    QString m_markdownHighlighting;
    QString m_codeHighlighting;
    QString m_previewStylesheet;
    bool m_builtIn;
};

#endif // THEME_H

