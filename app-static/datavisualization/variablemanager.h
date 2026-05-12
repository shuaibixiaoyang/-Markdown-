// 文件说明：app-static\datavisualization\variablemanager.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef VARIABLEMANAGER_H
#define VARIABLEMANAGER_H

#include <QObject>
#include <QWidget>
#include <QString>
#include <QVariant>
#include <QMap>
#include <QVector>

/**
 * @brief 全局变量管理器 - 管理文档中的全局变量
 * 
 * 支持在 Markdown 文档顶部定义变量:
 * $start_date = "2024-03-01"
 * $end_date = "2024-03-31"
 * $department = "Sales"
 * 
 * 这些变量可以在 SQL 查询和其他数据块中使用:
 * SELECT * FROM orders WHERE date >= '$start_date' AND date <= '$end_date'
 */
class VariableManager : public QObject
{
    Q_OBJECT

public:
    // 变量类型
    enum class VariableType {
        String,
        Date,
        Number,
        Boolean,
        List
    };
    Q_ENUM(VariableType)

    // 变量结构
    struct Variable {
        QString name;
        QVariant value;
        VariableType type;
        QString description;    // 可选描述
        QVariant defaultValue;  // 默认值
        bool isReadOnly;        // 是否只读
    };

    explicit VariableManager(QObject *parent = nullptr);
    ~VariableManager();

    // 单例访问
    static VariableManager& instance();

    // 变量操作
    void setVariable(const QString &name, const QVariant &value, 
                     VariableType type = VariableType::String);
    QVariant getVariable(const QString &name) const;
    bool hasVariable(const QString &name) const;
    void removeVariable(const QString &name);
    void clearVariables();

    // 获取所有变量
    QVector<Variable> allVariables() const;
    QStringList variableNames() const;

    // 从 Markdown 文档解析变量
    void parseFromMarkdown(const QString &markdown);

    // 变量替换
    QString substituteVariables(const QString &text) const;
    
    // 用于 SQL 查询的安全替换（防止 SQL 注入）
    QString substituteForSQL(const QString &sql) const;
    QString substituteForSQL(const QString &sql, QVariantList *params) const;

    // 类型转换
    static VariableType inferType(const QVariant &value);
    static QString typeToString(VariableType type);
    static VariableType stringToType(const QString &str);

    // 验证变量名
    static bool isValidVariableName(const QString &name);

signals:
    // 变量变化通知
    void variableChanged(const QString &name, const QVariant &newValue);
    void variableAdded(const QString &name);
    void variableRemoved(const QString &name);
    void variablesCleared();

private:
    QMap<QString, Variable> m_variables;
    
    QString escapeForSQL(const QString &value) const;
};


/**
 * @brief 变量编辑器控件 - 用于在 UI 中编辑全局变量
 */
class QWidget;
class QVBoxLayout;
class QTableWidget;
class QPushButton;
class QLineEdit;
class QComboBox;
class QDateEdit;

class VariableEditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VariableEditorWidget(QWidget *parent = nullptr);
    ~VariableEditorWidget();

    // 设置变量管理器
    void setVariableManager(VariableManager *manager);

    // 刷新显示
    void refresh();

signals:
    // 变量被修改
    void variablesModified();

private slots:
    void onAddVariable();
    void onRemoveVariable();
    void onCellChanged(int row, int column);
    void onApplyClicked();

private:
    void setupUI();
    void updateTable();

    VariableManager *m_manager;
    QTableWidget *m_tableWidget;
    QPushButton *m_addButton;
    QPushButton *m_removeButton;
    QPushButton *m_applyButton;
};

#endif // VARIABLEMANAGER_H

