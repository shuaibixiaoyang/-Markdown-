// 文件说明：app-static\datavisualization\datablockparser.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "datablockparser.h"

#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

// 函数说明：构造 DataBlockParser 对象，初始化本模块需要的状态、界面和资源。
DataBlockParser::DataBlockParser(QObject *parent)
    : QObject(parent)
{
}

DataBlockParser::~DataBlockParser() = default;

// 函数说明：解析输入内容，转换为 DataBlockParser 后续处理使用的数据结构。
QVector<DataBlockParser::DataBlock> DataBlockParser::parseDocument(const QString &markdown)
{
    QVector<DataBlock> blocks;
    
    // 匹配代码块: ```language {options}
    // 捕获组: 1=语言+选项, 2=代码内容
    QRegularExpression codeBlockRegex(
        "```(\\w+)\\s*(\\{[^}]*\\})?\\s*\\n([\\s\\S]*?)```",
        QRegularExpression::MultilineOption
    );
    
    QRegularExpressionMatchIterator it = codeBlockRegex.globalMatch(markdown);
    
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        
        QString language = match.captured(1).toLower();
        QString optionsStr = match.captured(2);
        QString code = match.captured(3).trimmed();
        
        // 计算行号
        int startPos = match.capturedStart();
        int startLine = markdown.left(startPos).count('\n') + 1;
        int endLine = startLine + code.count('\n') + 2;
        
        // 只处理有渲染选项的代码块
        if (!optionsStr.isEmpty()) {
            QString header = language + " " + optionsStr;
            DataBlock block = parseCodeBlock(header, code, startLine);
            block.endLine = endLine;
            
            if (block.renderType != RenderType::None) {
                blocks.append(block);
            }
        }
    }
    
    return blocks;
}

// 函数说明：解析输入内容，转换为 DataBlockParser 后续处理使用的数据结构。
DataBlockParser::DataBlock DataBlockParser::parseCodeBlock(const QString &header, 
                                                           const QString &code, 
                                                           int startLine)
{
    DataBlock block;
    block.id = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    block.code = code;
    block.startLine = startLine;
    block.valid = true;
    block.renderType = RenderType::None;
    
    // 提取语言
    QString language = extractLanguage(header);
    
    // 确定数据源类型
    if (language == "sql") {
        block.sourceType = SourceType::SQL;
    } else if (language == "json") {
        block.sourceType = SourceType::JSON;
    } else if (language == "csv") {
        block.sourceType = SourceType::CSV;
    } else {
        block.sourceType = SourceType::SQL; // 默认
    }
    
    // 解析选项
    QRegularExpression optionsRegex("\\{([^}]*)\\}");
    QRegularExpressionMatch optMatch = optionsRegex.match(header);
    
    if (optMatch.hasMatch()) {
        block.options = parseBlockOptions(optMatch.captured(1));
        
        // 提取渲染类型
        if (block.options.contains("render")) {
            block.renderType = stringToRenderType(block.options["render"].toString());
        }
        
        // 提取数据库连接
        if (block.options.contains("connection")) {
            block.connection = block.options["connection"].toString();
        }
        
        // 提取标题
        if (block.options.contains("title")) {
            block.title = block.options["title"].toString();
        }
        
        // API 源
        if (block.options.contains("source")) {
            QString source = block.options["source"].toString();
            if (source.startsWith("api://")) {
                block.sourceType = SourceType::API;
            }
        }
    }
    
    // 验证必要字段
    if (block.renderType == RenderType::None) {
        block.valid = false;
        block.errorMessage = "Missing or invalid render type";
    }
    
    if (block.sourceType == SourceType::SQL && block.connection.isEmpty()) {
        // 使用默认连接
        block.connection = "default";
    }
    
    return block;
}

// 函数说明：解析输入内容，转换为 DataBlockParser 后续处理使用的数据结构。
QVector<DataBlockParser::GlobalVariable> DataBlockParser::parseGlobalVariables(const QString &markdown)
{
    QVector<GlobalVariable> variables;
    
    // 匹配变量定义: $variable_name = "value" 或 $variable_name = value
    // 支持在文档顶部或 YAML 头部定义
    QRegularExpression varRegex(
        "^\\s*\\$(\\w+)\\s*=\\s*(?:\"([^\"]*)\"|'([^']*)'|(\\S+))",
        QRegularExpression::MultilineOption
    );
    
    QRegularExpressionMatchIterator it = varRegex.globalMatch(markdown);
    
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        
        GlobalVariable var;
        var.name = match.captured(1);
        
        // 取第一个非空的值
        if (!match.captured(2).isEmpty()) {
            var.value = match.captured(2);
        } else if (!match.captured(3).isEmpty()) {
            var.value = match.captured(3);
        } else {
            var.value = match.captured(4);
        }
        
        // 推断类型
        if (var.value.contains(QRegularExpression("^\\d{4}-\\d{2}-\\d{2}$"))) {
            var.type = "date";
        } else if (var.value.contains(QRegularExpression("^-?\\d+\\.?\\d*$"))) {
            var.type = "number";
        } else {
            var.type = "string";
        }
        
        variables.append(var);
    }
    
    return variables;
}

// 函数说明：实现 DataBlockParser::substituteVariables 的核心逻辑，供当前模块调用。
QString DataBlockParser::substituteVariables(const QString &code, 
                                             const QVector<GlobalVariable> &variables)
{
    QString result = code;
    
    for (const GlobalVariable &var : variables) {
        // 替换 $variable_name 或 ${variable_name}
        QRegularExpression varPattern(
            QString("\\$\\{?%1\\}?").arg(QRegularExpression::escape(var.name))
        );
        
        QString replacement = var.value;
        
        // 对于 SQL 中的字符串值，添加引号
        if (var.type == "string" || var.type == "date") {
            // 检查是否已在引号内，如果不在则添加
            // 这是简化处理，实际可能需要更复杂的逻辑
        }
        
        result.replace(varPattern, replacement);
    }
    
    return result;
}

// 函数说明：解析输入内容，转换为 DataBlockParser 后续处理使用的数据结构。
QVariantMap DataBlockParser::parseBlockOptions(const QString &optionsStr)
{
    QVariantMap options;
    
    // 解析 key: "value" 或 key: value 格式
    QRegularExpression optionRegex(
        "(\\w+)\\s*:\\s*(?:\"([^\"]*)\"|'([^']*)'|(\\w+))"
    );
    
    QRegularExpressionMatchIterator it = optionRegex.globalMatch(optionsStr);
    
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        
        QString key = match.captured(1);
        QString value;
        
        if (!match.captured(2).isEmpty()) {
            value = match.captured(2);
        } else if (!match.captured(3).isEmpty()) {
            value = match.captured(3);
        } else {
            value = match.captured(4);
        }
        
        options[key] = value;
    }
    
    return options;
}

// 函数说明：实现 DataBlockParser::extractLanguage 的核心逻辑，供当前模块调用。
QString DataBlockParser::extractLanguage(const QString &header)
{
    QRegularExpression langRegex("^(\\w+)");
    QRegularExpressionMatch match = langRegex.match(header.trimmed());
    
    if (match.hasMatch()) {
        return match.captured(1).toLower();
    }
    
    return QString();
}

// 函数说明：实现 DataBlockParser::stringToRenderType 的核心逻辑，供当前模块调用。
DataBlockParser::RenderType DataBlockParser::stringToRenderType(const QString &str)
{
    QString lower = str.toLower().replace("-", "").replace("_", "");
    
    if (lower == "barchart" || lower == "bar") {
        return RenderType::BarChart;
    } else if (lower == "linechart" || lower == "line") {
        return RenderType::LineChart;
    } else if (lower == "piechart" || lower == "pie") {
        return RenderType::PieChart;
    } else if (lower == "areachart" || lower == "area") {
        return RenderType::AreaChart;
    } else if (lower == "scatterchart" || lower == "scatter") {
        return RenderType::ScatterChart;
    } else if (lower == "table" || lower == "datatable") {
        return RenderType::Table;
    }
    
    return RenderType::None;
}

// 函数说明：渲染 DataBlockParser 的显示内容或导出片段。
QString DataBlockParser::renderTypeToString(RenderType type)
{
    switch (type) {
        case RenderType::BarChart: return "bar-chart";
        case RenderType::LineChart: return "line-chart";
        case RenderType::PieChart: return "pie-chart";
        case RenderType::AreaChart: return "area-chart";
        case RenderType::ScatterChart: return "scatter-chart";
        case RenderType::Table: return "table";
        default: return "none";
    }
}

// 函数说明：实现 DataBlockParser::stringToSourceType 的核心逻辑，供当前模块调用。
DataBlockParser::SourceType DataBlockParser::stringToSourceType(const QString &str)
{
    QString lower = str.toLower();
    
    if (lower == "sql") {
        return SourceType::SQL;
    } else if (lower == "json") {
        return SourceType::JSON;
    } else if (lower == "csv") {
        return SourceType::CSV;
    } else if (lower == "api") {
        return SourceType::API;
    }
    
    return SourceType::SQL;
}

