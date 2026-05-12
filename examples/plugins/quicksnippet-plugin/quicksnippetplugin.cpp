#include "quicksnippetplugin.h"
#include <QMenu>
#include <QAction>
#include <QDateTime>

QuickSnippetPlugin::QuickSnippetPlugin(QObject *parent)
    : QObject(parent)
{
}

QuickSnippetPlugin::~QuickSnippetPlugin()
{
}

PluginMetadata QuickSnippetPlugin::metadata() const
{
    PluginMetadata meta;
    meta.id = "quicksnippet";
    meta.name = "快捷片段";
    meta.version = "1.0.0";
    meta.author = "CuteMarkEd";
    meta.description = "通过菜单快速插入常用 Markdown 代码片段";
    meta.license = "GPL-2.0";
    meta.apiVersion = 1;
    return meta;
}

void QuickSnippetPlugin::registerSnippets()
{
    // 文档结构
    m_snippets["frontmatter"] =
        "---\n"
        "title: 标题\n"
        "author: 作者\n"
        "date: %DATE%\n"
        "tags: []\n"
        "---\n\n";

    m_snippets["toc_placeholder"] =
        "<!-- TOC -->\n\n"
        "## 目录\n\n"
        "- [章节一](#章节一)\n"
        "- [章节二](#章节二)\n"
        "- [章节三](#章节三)\n\n"
        "<!-- /TOC -->\n\n";

    // 代码片段
    m_snippets["code_cpp"] =
        "```cpp\n"
        "#include <iostream>\n\n"
        "int main() {\n"
        "    \n"
        "    return 0;\n"
        "}\n"
        "```\n";

    m_snippets["code_python"] =
        "```python\n"
        "#!/usr/bin/env python3\n\n"
        "def main():\n"
        "    pass\n\n"
        "if __name__ == '__main__':\n"
        "    main()\n"
        "```\n";

    m_snippets["code_js"] =
        "```javascript\n"
        "function main() {\n"
        "    \n"
        "}\n\n"
        "main();\n"
        "```\n";

    // 常用模板
    m_snippets["table_3x3"] =
        "| 列1 | 列2 | 列3 |\n"
        "| --- | --- | --- |\n"
        "|     |     |     |\n"
        "|     |     |     |\n"
        "|     |     |     |\n";

    m_snippets["checklist"] =
        "## 待办事项\n\n"
        "- [ ] 任务一\n"
        "- [ ] 任务二\n"
        "- [ ] 任务三\n"
        "- [ ] 任务四\n";

    m_snippets["admonition_note"] =
        "> **注意**\n"
        "> \n"
        "> 在此输入提示内容。\n\n";

    m_snippets["admonition_warning"] =
        "> **警告**\n"
        "> \n"
        "> 在此输入警告内容。\n\n";

    m_snippets["details"] =
        "<details>\n"
        "<summary>点击展开</summary>\n\n"
        "在此输入折叠内容。\n\n"
        "</details>\n\n";

    m_snippets["badge"] =
        "![badge](https://img.shields.io/badge/标签-内容-blue)\n";

    m_snippets["footnote"] =
        "这是一段带脚注的文本[^1]。\n\n"
        "[^1]: 这是脚注内容。\n";

    m_snippets["math_block"] =
        "$$\n"
        "E = mc^2\n"
        "$$\n";
}

bool QuickSnippetPlugin::initialize(PluginContext *context)
{
    m_context = context;

    registerSnippets();

    // 创建菜单
    m_menu = m_context->addMenu("快捷片段");

    // 文档结构子菜单
    QMenu *structMenu = m_menu->addMenu("文档结构");
    QAction *a1 = m_context->addAction(structMenu, "YAML Front Matter");
    connect(a1, &QAction::triggered, this, [this]() { insertSnippet("frontmatter"); });

    QAction *a2 = m_context->addAction(structMenu, "目录模板");
    connect(a2, &QAction::triggered, this, [this]() { insertSnippet("toc_placeholder"); });

    // 代码块子菜单
    QMenu *codeMenu = m_menu->addMenu("代码块");
    QAction *c1 = m_context->addAction(codeMenu, "C++");
    connect(c1, &QAction::triggered, this, [this]() { insertSnippet("code_cpp"); });

    QAction *c2 = m_context->addAction(codeMenu, "Python");
    connect(c2, &QAction::triggered, this, [this]() { insertSnippet("code_python"); });

    QAction *c3 = m_context->addAction(codeMenu, "JavaScript");
    connect(c3, &QAction::triggered, this, [this]() { insertSnippet("code_js"); });

    // 常用模板子菜单
    QMenu *tmplMenu = m_menu->addMenu("常用模板");
    QAction *t1 = m_context->addAction(tmplMenu, "3x3 表格");
    connect(t1, &QAction::triggered, this, [this]() { insertSnippet("table_3x3"); });

    QAction *t2 = m_context->addAction(tmplMenu, "待办清单");
    connect(t2, &QAction::triggered, this, [this]() { insertSnippet("checklist"); });

    QAction *t3 = m_context->addAction(tmplMenu, "注意提示");
    connect(t3, &QAction::triggered, this, [this]() { insertSnippet("admonition_note"); });

    QAction *t4 = m_context->addAction(tmplMenu, "警告提示");
    connect(t4, &QAction::triggered, this, [this]() { insertSnippet("admonition_warning"); });

    QAction *t5 = m_context->addAction(tmplMenu, "折叠内容");
    connect(t5, &QAction::triggered, this, [this]() { insertSnippet("details"); });

    QAction *t6 = m_context->addAction(tmplMenu, "徽章 Badge");
    connect(t6, &QAction::triggered, this, [this]() { insertSnippet("badge"); });

    QAction *t7 = m_context->addAction(tmplMenu, "脚注");
    connect(t7, &QAction::triggered, this, [this]() { insertSnippet("footnote"); });

    QAction *t8 = m_context->addAction(tmplMenu, "数学公式块");
    connect(t8, &QAction::triggered, this, [this]() { insertSnippet("math_block"); });

    m_context->log("QuickSnippetPlugin: 初始化完成，注册了 " + QString::number(m_snippets.size()) + " 个片段");
    return true;
}

void QuickSnippetPlugin::shutdown()
{
    if (m_menu && m_context) {
        m_context->removeMenu(m_menu);
        m_menu = nullptr;
    }
    m_context = nullptr;
}

void QuickSnippetPlugin::setEnabled(bool enabled)
{
    m_enabled = enabled;
    if (m_menu) {
        m_menu->setEnabled(enabled);
    }
}

bool QuickSnippetPlugin::isEnabled() const
{
    return m_enabled;
}

void QuickSnippetPlugin::insertSnippet(const QString &key)
{
    if (!m_context || !m_enabled) return;

    QString snippet = m_snippets.value(key);
    if (snippet.isEmpty()) return;

    // 替换变量
    snippet.replace("%DATE%", QDateTime::currentDateTime().toString("yyyy-MM-dd"));
    snippet.replace("%TIME%", QDateTime::currentDateTime().toString("HH:mm:ss"));
    snippet.replace("%DATETIME%", QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));

    m_context->insertText(snippet);
    m_context->showStatusMessage("已插入片段: " + key, 2000);
}
