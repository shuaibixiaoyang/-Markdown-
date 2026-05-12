// 文件说明：app-static\template\templatemanager.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "templatemanager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QDebug>

// 函数说明：构造 TemplateManager 对象，初始化本模块需要的状态、界面和资源。
TemplateManager::TemplateManager(QObject *parent)
    : QObject(parent)
{
    m_templateDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/templates";
    QDir().mkpath(m_templateDir);

    loadBuiltinTemplates();
    loadTemplates();
}

// 函数说明：销毁 TemplateManager 对象，释放本模块持有的资源。
TemplateManager::~TemplateManager()
{
    saveTemplates();
}

// 函数说明：加载 TemplateManager 需要的数据、配置或外部资源。
void TemplateManager::loadBuiltinTemplates()
{
    createDiaryTemplate();
    createMeetingNotesTemplate();
    createTechnicalDocTemplate();
    createBlogPostTemplate();
    createProjectPlanTemplate();
    createWeeklyReportTemplate();
    createReadingNotesTemplate();
    createResearchPaperTemplate();
}

// 函数说明：创建 TemplateManager 需要的对象、记录或输出内容。
void TemplateManager::createDiaryTemplate()
{
    Template tmpl;
    tmpl.id = "builtin_diary";
    tmpl.name = tr("日记");
    tmpl.description = tr("每日日记模板，记录心情和事件");
    tmpl.category = Category::Daily;
    tmpl.isBuiltin = true;
    tmpl.icon = "diary";
    tmpl.tags << "日记" << "日常" << "记录";

    tmpl.content = R"(# {{date}} 日记

## 今日天气
{{weather}}

## 心情指数
⭐⭐⭐⭐⭐ (5/5)

## 今日事件

### 上午
-

### 下午
-

### 晚上
-

## 今日感悟


## 明日计划
- [ ]
- [ ]
- [ ]

---
*记录于 {{time}}*
)";

    m_templates[tmpl.id] = tmpl;
}

// 函数说明：创建 TemplateManager 需要的对象、记录或输出内容。
void TemplateManager::createMeetingNotesTemplate()
{
    Template tmpl;
    tmpl.id = "builtin_meeting";
    tmpl.name = tr("会议记录");
    tmpl.description = tr("标准会议记录模板");
    tmpl.category = Category::Work;
    tmpl.isBuiltin = true;
    tmpl.icon = "meeting";
    tmpl.tags << "会议" << "工作" << "记录";

    tmpl.content = R"(# 会议记录

## 会议信息

| 项目 | 内容 |
|------|------|
| 会议主题 | {{title}} |
| 会议日期 | {{date}} |
| 会议时间 | {{time}} |
| 会议地点 |  |
| 主持人 | {{author}} |
| 记录人 |  |

## 参会人员
-
-
-

## 会议议程

### 1.

### 2.

### 3.

## 讨论内容

### 议题一

**讨论要点：**

**结论：**

### 议题二

**讨论要点：**

**结论：**

## 行动事项

| 序号 | 事项 | 负责人 | 截止日期 | 状态 |
|------|------|--------|----------|------|
| 1 |  |  |  | ⏳ |
| 2 |  |  |  | ⏳ |
| 3 |  |  |  | ⏳ |

## 下次会议
- 时间：
- 议题：

---
*记录于 {{datetime}}*
)";

    m_templates[tmpl.id] = tmpl;
}

// 函数说明：创建 TemplateManager 需要的对象、记录或输出内容。
void TemplateManager::createTechnicalDocTemplate()
{
    Template tmpl;
    tmpl.id = "builtin_techDoc";
    tmpl.name = tr("技术文档");
    tmpl.description = tr("技术文档/API文档模板");
    tmpl.category = Category::Technical;
    tmpl.isBuiltin = true;
    tmpl.icon = "code";
    tmpl.tags << "技术" << "文档" << "API";

    tmpl.content = R"(# {{title}}

> 版本: 1.0.0
> 作者: {{author}}
> 更新日期: {{date}}

## 概述

简要描述该技术/功能的目的和用途。

## 目录

- [环境要求](#环境要求)
- [安装](#安装)
- [快速开始](#快速开始)
- [API 参考](#api-参考)
- [配置选项](#配置选项)
- [常见问题](#常见问题)
- [更新日志](#更新日志)

## 环境要求

-
-
-

## 安装

```bash
# 安装命令
```

## 快速开始

### 基本用法

```python
# 示例代码
```

### 高级用法

```python
# 高级示例
```

## API 参考

### 函数名

**描述：**

**参数：**

| 参数 | 类型 | 必填 | 描述 |
|------|------|------|------|
| param1 | string | 是 | 参数描述 |
| param2 | int | 否 | 参数描述 |

**返回值：**

| 类型 | 描述 |
|------|------|
| object | 返回值描述 |

**示例：**

```python
# 使用示例
```

## 配置选项

| 选项 | 类型 | 默认值 | 描述 |
|------|------|--------|------|
| option1 | string | "" | 选项描述 |
| option2 | boolean | false | 选项描述 |

## 常见问题

### Q: 问题1?

A: 解答1

### Q: 问题2?

A: 解答2

## 更新日志

### v1.0.0 ({{date}})
- 初始版本发布

---
*由 {{author}} 创建于 {{datetime}}*
)";

    m_templates[tmpl.id] = tmpl;
}

// 函数说明：创建 TemplateManager 需要的对象、记录或输出内容。
void TemplateManager::createBlogPostTemplate()
{
    Template tmpl;
    tmpl.id = "builtin_blog";
    tmpl.name = tr("博客文章");
    tmpl.description = tr("博客文章/公众号文章模板");
    tmpl.category = Category::Creative;
    tmpl.isBuiltin = true;
    tmpl.icon = "blog";
    tmpl.tags << "博客" << "文章" << "写作";

    tmpl.content = R"(---
title: {{title}}
author: {{author}}
date: {{date}}
tags: []
categories: []
---

# {{title}}

![封面图片](cover.jpg)

> 引言或摘要

## 前言

开篇引入，吸引读者注意力。

## 正文

### 第一部分

详细内容...

### 第二部分

详细内容...

### 第三部分

详细内容...

## 总结

总结要点，升华主题。

## 参考资料

- [链接1](url1)
- [链接2](url2)

---

**关于作者：** {{author}}

**联系方式：**

**公众号：**

---
*发布于 {{date}}*
)";

    m_templates[tmpl.id] = tmpl;
}

// 函数说明：创建 TemplateManager 需要的对象、记录或输出内容。
void TemplateManager::createProjectPlanTemplate()
{
    Template tmpl;
    tmpl.id = "builtin_project";
    tmpl.name = tr("项目计划");
    tmpl.description = tr("项目规划和进度跟踪模板");
    tmpl.category = Category::Work;
    tmpl.isBuiltin = true;
    tmpl.icon = "project";
    tmpl.tags << "项目" << "计划" << "管理";

    tmpl.content = R"(# {{title}} - 项目计划

## 项目概述

| 项目名称 | {{title}} |
|----------|----------|
| 项目经理 | {{author}} |
| 开始日期 | {{date}} |
| 预计结束 |  |
| 当前状态 | 🟢 进行中 |

## 项目目标

1.
2.
3.

## 里程碑

| 阶段 | 目标 | 开始日期 | 结束日期 | 状态 |
|------|------|----------|----------|------|
| 阶段1 |  |  |  | ⏳ |
| 阶段2 |  |  |  | ⏳ |
| 阶段3 |  |  |  | ⏳ |

## 任务清单

### 阶段1

- [ ] 任务1
- [ ] 任务2
- [ ] 任务3

### 阶段2

- [ ] 任务1
- [ ] 任务2
- [ ] 任务3

## 资源分配

| 成员 | 角色 | 职责 |
|------|------|------|
|  |  |  |
|  |  |  |

## 风险评估

| 风险 | 可能性 | 影响 | 应对措施 |
|------|--------|------|----------|
|  | 中 | 高 |  |
|  | 低 | 中 |  |

## 进度更新

### {{date}}

-

---
*创建于 {{datetime}}*
)";

    m_templates[tmpl.id] = tmpl;
}

// 函数说明：创建 TemplateManager 需要的对象、记录或输出内容。
void TemplateManager::createWeeklyReportTemplate()
{
    Template tmpl;
    tmpl.id = "builtin_weekly";
    tmpl.name = tr("周报");
    tmpl.description = tr("每周工作总结报告模板");
    tmpl.category = Category::Work;
    tmpl.isBuiltin = true;
    tmpl.icon = "report";
    tmpl.tags << "周报" << "工作" << "总结";

    tmpl.content = R"(# 周报 - {{week_start}} 至 {{week_end}}

**姓名：** {{author}}
**部门：**
**日期：** {{date}}

## 本周工作总结

### 已完成工作

| 序号 | 工作内容 | 进度 | 备注 |
|------|----------|------|------|
| 1 |  | 100% |  |
| 2 |  | 100% |  |
| 3 |  | 100% |  |

### 进行中工作

| 序号 | 工作内容 | 进度 | 预计完成日期 |
|------|----------|------|--------------|
| 1 |  | 50% |  |
| 2 |  | 30% |  |

### 遇到的问题

1.
2.

### 需要的支持

1.
2.

## 下周工作计划

| 序号 | 工作内容 | 优先级 | 预计完成日期 |
|------|----------|--------|--------------|
| 1 |  | 高 |  |
| 2 |  | 中 |  |
| 3 |  | 低 |  |

## 其他事项


---
*提交于 {{datetime}}*
)";

    m_templates[tmpl.id] = tmpl;
}

// 函数说明：创建 TemplateManager 需要的对象、记录或输出内容。
void TemplateManager::createReadingNotesTemplate()
{
    Template tmpl;
    tmpl.id = "builtin_reading";
    tmpl.name = tr("读书笔记");
    tmpl.description = tr("读书笔记和书评模板");
    tmpl.category = Category::Daily;
    tmpl.isBuiltin = true;
    tmpl.icon = "book";
    tmpl.tags << "读书" << "笔记" << "学习";

    tmpl.content = R"(# 《书名》读书笔记

## 书籍信息

| 项目 | 内容 |
|------|------|
| 书名 |  |
| 作者 |  |
| 出版社 |  |
| 出版日期 |  |
| 阅读日期 | {{date}} |
| 评分 | ⭐⭐⭐⭐⭐ |

## 内容摘要

简要概述本书的主要内容和核心观点。

## 章节笔记

### 第一章

**主要内容：**

**金句摘录：**
>

**个人思考：**

### 第二章

**主要内容：**

**金句摘录：**
>

**个人思考：**

## 全书总结

### 核心观点

1.
2.
3.

### 实践建议

-
-
-

### 相关书籍

- 《相关书籍1》
- 《相关书籍2》

## 读后感

对这本书的整体评价和个人感悟。

---
*记录于 {{datetime}}*
)";

    m_templates[tmpl.id] = tmpl;
}

// 函数说明：创建 TemplateManager 需要的对象、记录或输出内容。
void TemplateManager::createResearchPaperTemplate()
{
    Template tmpl;
    tmpl.id = "builtin_research";
    tmpl.name = tr("研究论文");
    tmpl.description = tr("学术研究论文模板");
    tmpl.category = Category::Academic;
    tmpl.isBuiltin = true;
    tmpl.icon = "academic";
    tmpl.tags << "论文" << "学术" << "研究";

    tmpl.content = R"(# {{title}}

**作者：** {{author}}
**机构：**
**日期：** {{date}}

## 摘要

简要概述研究目的、方法、结果和结论（200-300字）。

**关键词：** 关键词1, 关键词2, 关键词3

## 1. 引言

### 1.1 研究背景

描述研究领域的背景和现状。

### 1.2 研究问题

明确要解决的核心问题。

### 1.3 研究目的

阐述本研究的目标和预期贡献。

## 2. 文献综述

### 2.1 理论基础

相关理论介绍。

### 2.2 研究现状

前人研究成果综述。

### 2.3 研究空白

现有研究的不足之处。

## 3. 研究方法

### 3.1 研究设计

### 3.2 数据收集

### 3.3 分析方法

## 4. 研究结果

### 4.1 主要发现

### 4.2 数据分析

## 5. 讨论

### 5.1 结果解释

### 5.2 理论贡献

### 5.3 实践意义

### 5.4 研究局限

## 6. 结论

总结主要发现，提出未来研究方向。

## 参考文献

[1] 作者. 标题. 期刊, 年份, 卷(期): 页码.

[2] 作者. 书名. 出版社, 年份.

---
*创建于 {{datetime}}*
)";

    m_templates[tmpl.id] = tmpl;
}

// 函数说明：读取 TemplateManager 当前保存的状态或计算结果。
QVector<TemplateManager::Template> TemplateManager::getAllTemplates() const
{
    QVector<Template> templates;
    for (const Template &tmpl : m_templates) {
        templates.append(tmpl);
    }
    return templates;
}

// 函数说明：读取 TemplateManager 当前保存的状态或计算结果。
QVector<TemplateManager::Template> TemplateManager::getTemplatesByCategory(Category category) const
{
    QVector<Template> templates;
    for (const Template &tmpl : m_templates) {
        if (tmpl.category == category) {
            templates.append(tmpl);
        }
    }
    return templates;
}

// 函数说明：读取 TemplateManager 当前保存的状态或计算结果。
TemplateManager::Template TemplateManager::getTemplate(const QString &id) const
{
    return m_templates.value(id);
}

// 函数说明：向 TemplateManager 管理的数据集合中添加一项内容。
bool TemplateManager::addTemplate(const Template &tmpl)
{
    Template newTmpl = tmpl;
    if (newTmpl.id.isEmpty()) {
        newTmpl.id = generateId();
    }
    newTmpl.createdTime = QDateTime::currentDateTime();
    newTmpl.modifiedTime = newTmpl.createdTime;
    newTmpl.isBuiltin = false;

    m_templates[newTmpl.id] = newTmpl;
    emit templateAdded(newTmpl.id);
    return true;
}

// 函数说明：刷新 TemplateManager 的内部状态，并同步到相关界面。
bool TemplateManager::updateTemplate(const Template &tmpl)
{
    if (!m_templates.contains(tmpl.id)) {
        m_lastError = tr("模板不存在: %1").arg(tmpl.id);
        return false;
    }

    if (m_templates[tmpl.id].isBuiltin) {
        m_lastError = tr("无法修改内置模板");
        return false;
    }

    Template updatedTmpl = tmpl;
    updatedTmpl.modifiedTime = QDateTime::currentDateTime();
    m_templates[tmpl.id] = updatedTmpl;

    emit templateUpdated(tmpl.id);
    return true;
}

// 函数说明：删除 TemplateManager 管理的指定数据或资源。
bool TemplateManager::deleteTemplate(const QString &id)
{
    if (!m_templates.contains(id)) {
        m_lastError = tr("模板不存在: %1").arg(id);
        return false;
    }

    if (m_templates[id].isBuiltin) {
        m_lastError = tr("无法删除内置模板");
        return false;
    }

    m_templates.remove(id);
    emit templateRemoved(id);
    return true;
}

// 函数说明：从 TemplateManager 管理的数据集合中移除指定内容。
bool TemplateManager::removeTemplate(const QString &id)
{
    return deleteTemplate(id);
}

// 函数说明：实现 TemplateManager::templateExists 的核心逻辑，供当前模块调用。
bool TemplateManager::templateExists(const QString &id) const
{
    return m_templates.contains(id);
}

// 函数说明：应用 TemplateManager 当前配置，让编辑器或预览立即生效。
QString TemplateManager::applyTemplate(const QString &templateId, const QMap<QString, QString> &variables)
{
    if (!m_templates.contains(templateId)) {
        m_lastError = tr("模板不存在: %1").arg(templateId);
        return QString();
    }

    return replaceVariables(m_templates[templateId].content, variables);
}

// 函数说明：应用 TemplateManager 当前配置，让编辑器或预览立即生效。
QString TemplateManager::applyTemplateContent(const QString &content, const QMap<QString, QString> &variables)
{
    return replaceVariables(content, variables);
}

// 函数说明：实现 TemplateManager::replaceVariables 的核心逻辑，供当前模块调用。
QString TemplateManager::replaceVariables(const QString &content, const QMap<QString, QString> &customVars)
{
    QString result = content;

    // 替换所有 {{variable}} 格式的变量
    QRegularExpression regex("\\{\\{(\\w+)\\}\\}");
    QRegularExpressionMatchIterator it = regex.globalMatch(content);

    QMap<QString, QString> replacements;

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString varName = match.captured(1);
        QString replacement;

        // 优先使用自定义变量
        if (customVars.contains(varName)) {
            replacement = customVars[varName];
        } else if (m_customVariables.contains(varName)) {
            replacement = m_customVariables[varName];
        } else {
            // 使用系统变量
            replacement = getSystemVariableValue(varName);
        }

        replacements[match.captured(0)] = replacement;
    }

    for (auto it = replacements.begin(); it != replacements.end(); ++it) {
        result.replace(it.key(), it.value());
    }

    return result;
}

// 函数说明：读取 TemplateManager 当前保存的状态或计算结果。
QString TemplateManager::getSystemVariableValue(const QString &name) const
{
    QDateTime now = QDateTime::currentDateTime();
    QDate today = now.date();

    if (name == "date") {
        return today.toString("yyyy-MM-dd");
    } else if (name == "time") {
        return now.toString("HH:mm");
    } else if (name == "datetime") {
        return now.toString("yyyy-MM-dd HH:mm");
    } else if (name == "year") {
        return QString::number(today.year());
    } else if (name == "month") {
        return QString::number(today.month());
    } else if (name == "day") {
        return QString::number(today.day());
    } else if (name == "weekday") {
        static QStringList weekdays = {
            tr("周日"), tr("周一"), tr("周二"), tr("周三"),
            tr("周四"), tr("周五"), tr("周六")
        };
        return weekdays[today.dayOfWeek() % 7];
    } else if (name == "week_start") {
        QDate weekStart = today.addDays(-(today.dayOfWeek() - 1));
        return weekStart.toString("yyyy-MM-dd");
    } else if (name == "week_end") {
        QDate weekEnd = today.addDays(7 - today.dayOfWeek());
        return weekEnd.toString("yyyy-MM-dd");
    } else if (name == "author") {
        return qgetenv("USER").isEmpty() ? qgetenv("USERNAME") : qgetenv("USER");
    } else if (name == "title") {
        return tr("无标题");
    } else if (name == "weather") {
        return tr("晴");
    }

    return QString("{{%1}}").arg(name);  // 保留未知变量
}

// 函数说明：读取 TemplateManager 当前保存的状态或计算结果。
QVector<TemplateManager::Variable> TemplateManager::getSystemVariables() const
{
    QVector<Variable> vars;

    auto makeVar = [](const QString &n, const QString &desc) {
        Variable v;
        v.name = n;
        v.description = desc;
        v.defaultValue = QString();
        v.isSystem = true;
        return v;
    };

    vars.append(makeVar("date", tr("当前日期 (yyyy-MM-dd)")));
    vars.append(makeVar("time", tr("当前时间 (HH:mm)")));
    vars.append(makeVar("datetime", tr("当前日期时间")));
    vars.append(makeVar("year", tr("当前年份")));
    vars.append(makeVar("month", tr("当前月份")));
    vars.append(makeVar("day", tr("当前日期")));
    vars.append(makeVar("weekday", tr("当前星期")));
    vars.append(makeVar("week_start", tr("本周开始日期")));
    vars.append(makeVar("week_end", tr("本周结束日期")));
    vars.append(makeVar("author", tr("作者/用户名")));
    vars.append(makeVar("title", tr("文档标题")));

    return vars;
}

// 函数说明：读取 TemplateManager 当前保存的状态或计算结果。
QVector<TemplateManager::Variable> TemplateManager::getCustomVariables() const
{
    QVector<Variable> vars;
    for (auto it = m_customVariables.begin(); it != m_customVariables.end(); ++it) {
        Variable var;
        var.name = it.key();
        var.defaultValue = it.value();
        var.isSystem = false;
        vars.append(var);
    }
    return vars;
}

// 函数说明：设置 TemplateManager 的运行参数，并触发必要的界面或数据刷新。
void TemplateManager::setCustomVariable(const QString &name, const QString &value)
{
    m_customVariables[name] = value;
}

// 函数说明：读取 TemplateManager 当前保存的状态或计算结果。
QString TemplateManager::getVariableValue(const QString &name) const
{
    if (m_customVariables.contains(name)) {
        return m_customVariables[name];
    }
    return getSystemVariableValue(name);
}

// 函数说明：读取 TemplateManager 当前保存的状态或计算结果。
QStringList TemplateManager::getBuiltinTemplateIds() const
{
    QStringList ids;
    for (const Template &tmpl : m_templates) {
        if (tmpl.isBuiltin) {
            ids.append(tmpl.id);
        }
    }
    return ids;
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
bool TemplateManager::exportTemplate(const QString &id, const QString &filePath)
{
    if (!m_templates.contains(id)) {
        m_lastError = tr("模板不存在: %1").arg(id);
        return false;
    }

    const Template &tmpl = m_templates[id];

    QJsonObject obj;
    obj["id"] = tmpl.id;
    obj["name"] = tmpl.name;
    obj["description"] = tmpl.description;
    obj["category"] = categoryToString(tmpl.category);
    obj["content"] = tmpl.content;
    obj["icon"] = tmpl.icon;
    obj["tags"] = QJsonArray::fromStringList(tmpl.tags);

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(obj).toJson());
        file.close();
        return true;
    }

    m_lastError = tr("无法写入文件: %1").arg(filePath);
    return false;
}

// 函数说明：实现 TemplateManager::importTemplate 的核心逻辑，供当前模块调用。
bool TemplateManager::importTemplate(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = tr("无法读取文件: %1").arg(filePath);
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (doc.isNull()) {
        m_lastError = tr("无效的模板文件");
        return false;
    }

    QJsonObject obj = doc.object();

    Template tmpl;
    tmpl.id = generateId();  // 生成新 ID
    tmpl.name = obj["name"].toString();
    tmpl.description = obj["description"].toString();
    tmpl.category = stringToCategory(obj["category"].toString());
    tmpl.content = obj["content"].toString();
    tmpl.icon = obj["icon"].toString();
    tmpl.isBuiltin = false;

    QJsonArray tagsArray = obj["tags"].toArray();
    for (const QJsonValue &val : tagsArray) {
        tmpl.tags.append(val.toString());
    }

    return addTemplate(tmpl);
}

// 函数说明：保存 TemplateManager 当前状态，保证用户修改可以持久化。
bool TemplateManager::saveTemplates()
{
    QString filePath = m_templateDir + "/templates.json";

    QJsonArray array;
    for (const Template &tmpl : m_templates) {
        if (tmpl.isBuiltin) continue;  // 不保存内置模板

        QJsonObject obj;
        obj["id"] = tmpl.id;
        obj["name"] = tmpl.name;
        obj["description"] = tmpl.description;
        obj["category"] = categoryToString(tmpl.category);
        obj["content"] = tmpl.content;
        obj["icon"] = tmpl.icon;
        obj["tags"] = QJsonArray::fromStringList(tmpl.tags);
        obj["createdTime"] = tmpl.createdTime.toString(Qt::ISODate);
        obj["modifiedTime"] = tmpl.modifiedTime.toString(Qt::ISODate);
        array.append(obj);
    }

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(array).toJson());
        file.close();
        return true;
    }

    return false;
}

// 函数说明：加载 TemplateManager 需要的数据、配置或外部资源。
bool TemplateManager::loadTemplates()
{
    QString filePath = m_templateDir + "/templates.json";

    QFile file(filePath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (doc.isNull()) {
        return false;
    }

    QJsonArray array = doc.array();
    for (const QJsonValue &val : array) {
        QJsonObject obj = val.toObject();

        Template tmpl;
        tmpl.id = obj["id"].toString();
        tmpl.name = obj["name"].toString();
        tmpl.description = obj["description"].toString();
        tmpl.category = stringToCategory(obj["category"].toString());
        tmpl.content = obj["content"].toString();
        tmpl.icon = obj["icon"].toString();
        tmpl.isBuiltin = false;
        tmpl.createdTime = QDateTime::fromString(obj["createdTime"].toString(), Qt::ISODate);
        tmpl.modifiedTime = QDateTime::fromString(obj["modifiedTime"].toString(), Qt::ISODate);

        QJsonArray tagsArray = obj["tags"].toArray();
        for (const QJsonValue &tagVal : tagsArray) {
            tmpl.tags.append(tagVal.toString());
        }

        m_templates[tmpl.id] = tmpl;
    }

    emit templatesLoaded();
    return true;
}

// 函数说明：设置 TemplateManager 的运行参数，并触发必要的界面或数据刷新。
void TemplateManager::setTemplateDirectory(const QString &path)
{
    m_templateDir = path;
    QDir().mkpath(m_templateDir);
}

// 函数说明：实现 TemplateManager::scanTemplateDirectory 的核心逻辑，供当前模块调用。
void TemplateManager::scanTemplateDirectory()
{
    QDir dir(m_templateDir);
    QStringList filters;
    filters << "*.json" << "*.md";

    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
    for (const QFileInfo &fileInfo : files) {
        if (fileInfo.fileName() != "templates.json") {
            importTemplate(fileInfo.filePath());
        }
    }
}

// 函数说明：根据当前数据生成 TemplateManager 需要的输出结果。
QString TemplateManager::generateId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

// 函数说明：实现 TemplateManager::categoryToString 的核心逻辑，供当前模块调用。
QString TemplateManager::categoryToString(Category category) const
{
    switch (category) {
        case Category::Daily: return "daily";
        case Category::Work: return "work";
        case Category::Technical: return "technical";
        case Category::Academic: return "academic";
        case Category::Creative: return "creative";
        case Category::Custom: return "custom";
    }
    return "custom";
}

// 函数说明：实现 TemplateManager::stringToCategory 的核心逻辑，供当前模块调用。
TemplateManager::Category TemplateManager::stringToCategory(const QString &str) const
{
    if (str == "daily") return Category::Daily;
    if (str == "work") return Category::Work;
    if (str == "technical") return Category::Technical;
    if (str == "academic") return Category::Academic;
    if (str == "creative") return Category::Creative;
    return Category::Custom;
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
bool TemplateManager::exportAllTemplates(const QString &dirPath)
{
    QDir dir(dirPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    bool success = true;
    for (const Template &tmpl : m_templates) {
        QString filePath = dirPath + "/" + tmpl.id + ".json";
        if (!exportTemplate(tmpl.id, filePath)) {
            success = false;
        }
    }

    return success;
}

// 函数说明：实现 TemplateManager::importTemplates 的核心逻辑，供当前模块调用。
bool TemplateManager::importTemplates(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = tr("无法打开文件: %1").arg(filePath);
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        m_lastError = tr("JSON 解析错误: %1").arg(error.errorString());
        return false;
    }

    if (doc.isArray()) {
        // 批量导入多个模板
        QJsonArray array = doc.array();
        for (const QJsonValue &value : array) {
            if (value.isObject()) {
                QJsonObject obj = value.toObject();
                Template tmpl;
                tmpl.id = obj["id"].toString();
                if (tmpl.id.isEmpty()) {
                    tmpl.id = generateId();
                }
                tmpl.name = obj["name"].toString();
                tmpl.description = obj["description"].toString();
                tmpl.category = stringToCategory(obj["category"].toString());
                tmpl.content = obj["content"].toString();
                tmpl.icon = obj["icon"].toString();
                tmpl.isBuiltin = false;

                QJsonArray tagsArray = obj["tags"].toArray();
                for (const QJsonValue &tagVal : tagsArray) {
                    tmpl.tags.append(tagVal.toString());
                }

                tmpl.createdTime = QDateTime::currentDateTime();
                tmpl.modifiedTime = QDateTime::currentDateTime();

                if (!tmpl.name.isEmpty() && !tmpl.content.isEmpty()) {
                    m_templates[tmpl.id] = tmpl;
                    emit templateAdded(tmpl.id);
                }
            }
        }
        return true;
    } else if (doc.isObject()) {
        // 单个模板导入
        return importTemplate(filePath);
    }

    m_lastError = tr("无效的模板文件格式");
    return false;
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
bool TemplateManager::exportTemplates(const QString &filePath)
{
    QJsonArray array;

    for (const Template &tmpl : m_templates) {
        if (!tmpl.isBuiltin) {
            QJsonObject obj;
            obj["id"] = tmpl.id;
            obj["name"] = tmpl.name;
            obj["description"] = tmpl.description;
            obj["category"] = categoryToString(tmpl.category);
            obj["content"] = tmpl.content;
            obj["icon"] = tmpl.icon;

            QJsonArray tagsArray;
            for (const QString &tag : tmpl.tags) {
                tagsArray.append(tag);
            }
            obj["tags"] = tagsArray;

            array.append(obj);
        }
    }

    QJsonDocument doc(array);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        m_lastError = tr("无法写入文件: %1").arg(filePath);
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    return true;
}

