// 文件说明：app-static\datavisualization\datablockparser.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef DATABLOCKPARSER_H
#define DATABLOCKPARSER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QVector>

/**
 * @brief 数据块解析器 - 解析带有渲染指令的代码块
 * 
 * 支持的语法格式:
 * ```sql {render: "bar-chart", connection: "sales_db"}
 * SELECT category, SUM(amount) AS total FROM orders GROUP BY category;
 * ```
 * 
 * ```json {render: "pie-chart", source: "api://data"}
 * ```
 */
class DataBlockParser : public QObject
{
    Q_OBJECT

public:
    // 渲染类型
    enum class RenderType {
        None,           // 普通代码块，不渲染
        BarChart,       // 柱状图
        LineChart,      // 折线图
        PieChart,       // 饼图
        AreaChart,      // 面积图
        ScatterChart,   // 散点图
        Table           // 数据表格
    };
    Q_ENUM(RenderType)

    // 数据源类型
    enum class SourceType {
        SQL,            // SQL 查询
        JSON,           // JSON 数据
        CSV,            // CSV 数据
        API             // API 请求
    };
    Q_ENUM(SourceType)

    // 数据块结构
    struct DataBlock {
        QString id;                     // 唯一标识
        SourceType sourceType;          // 数据源类型
        RenderType renderType;          // 渲染类型
        QString code;                   // 代码/查询内容
        QString connection;             // 数据库连接名
        QString title;                  // 图表标题
        QVariantMap options;            // 其他选项
        int startLine;                  // 起始行号
        int endLine;                    // 结束行号
        bool valid;                     // 是否有效
        QString errorMessage;           // 错误信息
    };

    // 全局变量结构
    struct GlobalVariable {
        QString name;
        QString value;
        QString type;  // string, date, number
    };

    explicit DataBlockParser(QObject *parent = nullptr);
    ~DataBlockParser();

    // 解析整个 Markdown 文档
    QVector<DataBlock> parseDocument(const QString &markdown);

    // 解析单个代码块
    DataBlock parseCodeBlock(const QString &header, const QString &code, int startLine);

    // 解析全局变量区
    QVector<GlobalVariable> parseGlobalVariables(const QString &markdown);

    // 替换变量到代码中
    QString substituteVariables(const QString &code, const QVector<GlobalVariable> &variables);

    // 工具方法
    static RenderType stringToRenderType(const QString &str);
    static QString renderTypeToString(RenderType type);
    static SourceType stringToSourceType(const QString &str);

private:
    // 解析代码块头部的选项 {render: "...", connection: "..."}
    QVariantMap parseBlockOptions(const QString &optionsStr);

    // 提取代码块语言
    QString extractLanguage(const QString &header);
};

#endif // DATABLOCKPARSER_H

