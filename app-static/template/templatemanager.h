// 文件说明：app-static\template\templatemanager.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef TEMPLATEMANAGER_H
#define TEMPLATEMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QMap>
#include <QVector>

/**
 * @brief 文档模板管理器
 *
 * 功能：
 * - 内置模板（日记、会议记录、技术文档等）
 * - 自定义模板
 * - 模板变量替换
 * - 模板分类管理
 * - 模板导入导出
 */
class TemplateManager : public QObject
{
    Q_OBJECT

public:
    // 模板类别
    enum class Category {
        Daily,          // 日常
        Work,           // 工作
        Technical,      // 技术
        Academic,       // 学术
        Creative,       // 创意
        Custom          // 自定义
    };
    Q_ENUM(Category)

    // 模板信息
    struct Template {
        QString id;             // 唯一标识
        QString name;           // 模板名称
        QString description;    // 描述
        Category category;      // 类别
        QString content;        // 模板内容
        QString icon;           // 图标名称
        QStringList tags;       // 标签
        bool isBuiltin;         // 是否内置
        QDateTime createdTime;  // 创建时间
        QDateTime modifiedTime; // 修改时间

        Template() : category(Category::Custom), isBuiltin(false) {}
    };

    // 变量定义
    struct Variable {
        QString name;           // 变量名 (如 {{date}})
        QString description;    // 描述
        QString defaultValue;   // 默认值
        bool isSystem;          // 是否系统变量

        Variable() : isSystem(false) {}
    };

    explicit TemplateManager(QObject *parent = nullptr);
    ~TemplateManager();

    // 模板管理
    QVector<Template> getAllTemplates() const;
    QVector<Template> getTemplatesByCategory(Category category) const;
    Template getTemplate(const QString &id) const;
    bool addTemplate(const Template &tmpl);
    bool updateTemplate(const Template &tmpl);
    bool removeTemplate(const QString &id);
    bool deleteTemplate(const QString &id);  // Alias for removeTemplate
    bool templateExists(const QString &id) const;

    // 模板应用
    QString applyTemplate(const QString &templateId, const QMap<QString, QString> &variables = QMap<QString, QString>());
    QString applyTemplateContent(const QString &content, const QMap<QString, QString> &variables = QMap<QString, QString>());

    // 变量管理
    QVector<Variable> getSystemVariables() const;
    QVector<Variable> getCustomVariables() const;
    void setCustomVariable(const QString &name, const QString &value);
    QString getVariableValue(const QString &name) const;

    // 内置模板
    void loadBuiltinTemplates();
    QStringList getBuiltinTemplateIds() const;

    // 导入导出
    bool exportTemplate(const QString &id, const QString &filePath);
    bool importTemplate(const QString &filePath);
    bool importTemplates(const QString &filePath);  // 批量导入
    bool exportTemplates(const QString &filePath);  // 批量导出
    bool exportAllTemplates(const QString &dirPath);

    // 模板目录
    void setTemplateDirectory(const QString &path);
    QString templateDirectory() const { return m_templateDir; }
    void scanTemplateDirectory();

    // 持久化
    bool saveTemplates();
    bool loadTemplates();

    // 错误信息
    QString lastError() const { return m_lastError; }

signals:
    void templateAdded(const QString &id);
    void templateUpdated(const QString &id);
    void templateRemoved(const QString &id);
    void templatesLoaded();
    void errorOccurred(const QString &error);

private:
    // 内置模板创建
    void createDiaryTemplate();
    void createMeetingNotesTemplate();
    void createTechnicalDocTemplate();
    void createBlogPostTemplate();
    void createProjectPlanTemplate();
    void createWeeklyReportTemplate();
    void createReadingNotesTemplate();
    void createResearchPaperTemplate();

    // 变量替换
    QString replaceVariables(const QString &content, const QMap<QString, QString> &customVars);
    QString getSystemVariableValue(const QString &name) const;

    // 辅助函数
    QString generateId();
    QString categoryToString(Category category) const;
    Category stringToCategory(const QString &str) const;

    QMap<QString, Template> m_templates;
    QMap<QString, QString> m_customVariables;
    QString m_templateDir;
    QString m_lastError;
};

#endif // TEMPLATEMANAGER_H

