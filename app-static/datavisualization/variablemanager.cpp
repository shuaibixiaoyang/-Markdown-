// 文件说明：app-static\datavisualization\variablemanager.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "variablemanager.h"

#include <QRegularExpression>
#include <QDate>
#include <QDateTime>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QLabel>
#include <QMessageBox>

// ==================== VariableManager ====================

VariableManager::VariableManager(QObject *parent)
    : QObject(parent)
{
}

VariableManager::~VariableManager() = default;

// 函数说明：实现 VariableManager::instance 的核心逻辑，供当前模块调用。
VariableManager& VariableManager::instance()
{
    static VariableManager instance;
    return instance;
}

// 函数说明：设置 VariableManager 的运行参数，并触发必要的界面或数据刷新。
void VariableManager::setVariable(const QString &name, const QVariant &value, VariableType type)
{
    if (!isValidVariableName(name)) {
        return;
    }

    bool isNew = !m_variables.contains(name);

    Variable var;
    var.name = name;
    var.value = value;
    var.type = type;
    var.isReadOnly = false;

    m_variables[name] = var;

    if (isNew) {
        emit variableAdded(name);
    }
    emit variableChanged(name, value);
}

// 函数说明：读取 VariableManager 当前保存的状态或计算结果。
QVariant VariableManager::getVariable(const QString &name) const
{
    if (m_variables.contains(name)) {
        return m_variables[name].value;
    }
    return QVariant();
}

// 函数说明：检查 VariableManager 是否具备对应的数据或能力。
bool VariableManager::hasVariable(const QString &name) const
{
    return m_variables.contains(name);
}

// 函数说明：从 VariableManager 管理的数据集合中移除指定内容。
void VariableManager::removeVariable(const QString &name)
{
    if (m_variables.contains(name)) {
        m_variables.remove(name);
        emit variableRemoved(name);
    }
}

// 函数说明：清空 VariableManager 保存的临时状态或缓存数据。
void VariableManager::clearVariables()
{
    m_variables.clear();
    emit variablesCleared();
}

// 函数说明：实现 VariableManager::allVariables 的核心逻辑，供当前模块调用。
QVector<VariableManager::Variable> VariableManager::allVariables() const
{
    QVector<Variable> result;
    for (const Variable &var : m_variables) {
        result.append(var);
    }
    return result;
}

// 函数说明：实现 VariableManager::variableNames 的核心逻辑，供当前模块调用。
QStringList VariableManager::variableNames() const
{
    return m_variables.keys();
}

// 函数说明：解析输入内容，转换为 VariableManager 后续处理使用的数据结构。
void VariableManager::parseFromMarkdown(const QString &markdown)
{
    // 匹配变量定义:
    // $variable_name = "value"
    // $variable_name = 'value'
    // $variable_name = value
    // $variable_name: "value"  (YAML 风格)
    
    QRegularExpression varRegex(
        "^\\s*\\$(\\w+)\\s*[=:]\\s*(?:\"([^\"]*)\"|'([^']*)'|(\\S+))",
        QRegularExpression::MultilineOption
    );
    
    QRegularExpressionMatchIterator it = varRegex.globalMatch(markdown);
    
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        
        QString name = match.captured(1);
        QString value;
        
        // 取第一个非空的值
        if (!match.captured(2).isEmpty()) {
            value = match.captured(2);
        } else if (!match.captured(3).isEmpty()) {
            value = match.captured(3);
        } else {
            value = match.captured(4);
        }
        
        // 推断类型并设置
        VariableType type = inferType(value);
        
        QVariant varValue;
        switch (type) {
            case VariableType::Date:
                varValue = QDate::fromString(value, Qt::ISODate);
                if (!varValue.toDate().isValid()) {
                    varValue = value;
                    type = VariableType::String;
                }
                break;
            case VariableType::Number:
                if (value.contains('.')) {
                    varValue = value.toDouble();
                } else {
                    varValue = value.toLongLong();
                }
                break;
            case VariableType::Boolean:
                varValue = (value.toLower() == "true" || value == "1");
                break;
            default:
                varValue = value;
                break;
        }
        
        setVariable(name, varValue, type);
    }
}

// 函数说明：实现 VariableManager::substituteVariables 的核心逻辑，供当前模块调用。
QString VariableManager::substituteVariables(const QString &text) const
{
    QString result = text;
    
    // 替换 ${variable_name} 和 $variable_name
    for (auto it = m_variables.begin(); it != m_variables.end(); ++it) {
        const Variable &var = it.value();
        
        // ${name} 格式
        QRegularExpression bracedPattern(QString("\\$\\{%1\\}").arg(QRegularExpression::escape(var.name)));
        result.replace(bracedPattern, var.value.toString());
        
        // $name 格式（后面不能跟字母数字下划线）
        QRegularExpression simplePattern(QString("\\$%1(?!\\w)").arg(QRegularExpression::escape(var.name)));
        result.replace(simplePattern, var.value.toString());
    }
    
    return result;
}

// 函数说明：实现 VariableManager::substituteForSQL 的核心逻辑，供当前模块调用。
QString VariableManager::substituteForSQL(const QString &sql) const
{
    QVariantList unusedParams;
    return substituteForSQL(sql, &unusedParams);
}

// 函数说明：实现 VariableManager::substituteForSQL 的核心逻辑，供当前模块调用。
QString VariableManager::substituteForSQL(const QString &sql, QVariantList *params) const
{
    if (params) {
        params->clear();
    }

    QString result = sql;

    // 匹配 '$name' / '${name}' / $name / ${name}
    QRegularExpression tokenRegex(
        QStringLiteral("'\\$(?:\\{([a-zA-Z_][a-zA-Z0-9_]*)\\}|([a-zA-Z_][a-zA-Z0-9_]*))'|\\$(?:\\{([a-zA-Z_][a-zA-Z0-9_]*)\\}|([a-zA-Z_][a-zA-Z0-9_]*))"));

    int searchOffset = 0;
    while (true) {
        QRegularExpressionMatch match = tokenRegex.match(result, searchOffset);
        if (!match.hasMatch()) {
            break;
        }

        const QString varName = !match.captured(1).isEmpty() ? match.captured(1)
                                 : !match.captured(2).isEmpty() ? match.captured(2)
                                 : !match.captured(3).isEmpty() ? match.captured(3)
                                 : match.captured(4);

        // 未定义变量保留原样，避免错误替换。
        if (!m_variables.contains(varName)) {
            searchOffset = match.capturedEnd();
            continue;
        }

        const Variable &var = m_variables.value(varName);
        int replacePos = match.capturedStart();
        int replaceLen = match.capturedLength();

        if (var.type == VariableType::List) {
            const QVariantList values = var.value.toList();
            if (values.isEmpty()) {
                result.replace(replacePos, replaceLen, QStringLiteral("NULL"));
                searchOffset = replacePos + 4;
            } else {
                QStringList placeholders;
                placeholders.reserve(values.size());
                for (const QVariant &value : values) {
                    placeholders.append(QStringLiteral("?"));
                    if (params) {
                        params->append(value);
                    }
                }
                const QString replacement = placeholders.join(QStringLiteral(","));
                result.replace(replacePos, replaceLen, replacement);
                searchOffset = replacePos + replacement.size();
            }
        } else {
            result.replace(replacePos, replaceLen, QStringLiteral("?"));
            if (params) {
                params->append(var.value);
            }
            searchOffset = replacePos + 1;
        }
    }

    return result;
}

// 函数说明：实现 VariableManager::escapeForSQL 的核心逻辑，供当前模块调用。
QString VariableManager::escapeForSQL(const QString &value) const
{
    QString escaped = value;
    // 转义单引号（SQL 标准）
    escaped.replace("'", "''");
    // 移除可能导致注入的控制字符
    escaped.remove(QChar('\0'));      // NULL 字符
    escaped.remove(QChar('\x1a'));    // Ctrl+Z (某些数据库的EOF)
    // 转义反斜杠（MySQL 特殊处理）
    escaped.replace("\\", "\\\\");
    // 移除注释标记
    escaped.replace("--", "");
    escaped.replace("/*", "");
    escaped.replace("*/", "");
    return escaped;
}

// 函数说明：实现 VariableManager::inferType 的核心逻辑，供当前模块调用。
VariableManager::VariableType VariableManager::inferType(const QVariant &value)
{
    QString str = value.toString();
    
    // 日期格式检测
    if (str.contains(QRegularExpression("^\\d{4}-\\d{2}-\\d{2}$"))) {
        return VariableType::Date;
    }
    
    // 数字格式检测
    if (str.contains(QRegularExpression("^-?\\d+\\.?\\d*$"))) {
        return VariableType::Number;
    }
    
    // 布尔值检测
    if (str.toLower() == "true" || str.toLower() == "false") {
        return VariableType::Boolean;
    }
    
    return VariableType::String;
}

// 函数说明：实现 VariableManager::typeToString 的核心逻辑，供当前模块调用。
QString VariableManager::typeToString(VariableType type)
{
    switch (type) {
        case VariableType::String: return "string";
        case VariableType::Date: return "date";
        case VariableType::Number: return "number";
        case VariableType::Boolean: return "boolean";
        case VariableType::List: return "list";
        default: return "string";
    }
}

// 函数说明：实现 VariableManager::stringToType 的核心逻辑，供当前模块调用。
VariableManager::VariableType VariableManager::stringToType(const QString &str)
{
    QString lower = str.toLower();
    if (lower == "date") return VariableType::Date;
    if (lower == "number" || lower == "int" || lower == "integer" || lower == "double" || lower == "float") 
        return VariableType::Number;
    if (lower == "boolean" || lower == "bool") return VariableType::Boolean;
    if (lower == "list" || lower == "array") return VariableType::List;
    return VariableType::String;
}

// 函数说明：判断 VariableManager 当前是否满足指定状态。
bool VariableManager::isValidVariableName(const QString &name)
{
    if (name.isEmpty()) return false;
    
    // 变量名必须以字母或下划线开头，后续可以是字母、数字或下划线
    QRegularExpression validName("^[a-zA-Z_][a-zA-Z0-9_]*$");
    return validName.match(name).hasMatch();
}


// ==================== VariableEditorWidget ====================

VariableEditorWidget::VariableEditorWidget(QWidget *parent)
    : QWidget(parent)
    , m_manager(nullptr)
    , m_tableWidget(nullptr)
    , m_addButton(nullptr)
    , m_removeButton(nullptr)
    , m_applyButton(nullptr)
{
    setupUI();
}

VariableEditorWidget::~VariableEditorWidget() = default;

// 函数说明：初始化 VariableEditorWidget 的 setupUI 相关界面、动作或服务连接。
void VariableEditorWidget::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // 标题
    QLabel *titleLabel = new QLabel(tr("全局变量"), this);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14px; padding: 5px;");
    layout->addWidget(titleLabel);

    // 表格
    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setColumnCount(3);
    m_tableWidget->setHorizontalHeaderLabels({tr("变量名"), tr("类型"), tr("值")});
    m_tableWidget->horizontalHeader()->setStretchLastSection(true);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setAlternatingRowColors(true);
    
    connect(m_tableWidget, &QTableWidget::cellChanged, this, &VariableEditorWidget::onCellChanged);
    
    layout->addWidget(m_tableWidget);

    // 按钮区
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    m_addButton = new QPushButton(tr("添加"), this);
    connect(m_addButton, &QPushButton::clicked, this, &VariableEditorWidget::onAddVariable);
    buttonLayout->addWidget(m_addButton);

    m_removeButton = new QPushButton(tr("删除"), this);
    connect(m_removeButton, &QPushButton::clicked, this, &VariableEditorWidget::onRemoveVariable);
    buttonLayout->addWidget(m_removeButton);

    buttonLayout->addStretch();

    m_applyButton = new QPushButton(tr("应用"), this);
    m_applyButton->setStyleSheet("background-color: #4285f4; color: white;");
    connect(m_applyButton, &QPushButton::clicked, this, &VariableEditorWidget::onApplyClicked);
    buttonLayout->addWidget(m_applyButton);

    layout->addLayout(buttonLayout);
}

// 函数说明：设置 VariableEditorWidget 的运行参数，并触发必要的界面或数据刷新。
void VariableEditorWidget::setVariableManager(VariableManager *manager)
{
    m_manager = manager;
    updateTable();
    
    if (m_manager) {
        connect(m_manager, &VariableManager::variableChanged, this, [this]() {
            updateTable();
        });
    }
}

// 函数说明：实现 VariableEditorWidget::refresh 的核心逻辑，供当前模块调用。
void VariableEditorWidget::refresh()
{
    updateTable();
}

// 函数说明：刷新 VariableEditorWidget 的内部状态，并同步到相关界面。
void VariableEditorWidget::updateTable()
{
    if (!m_manager) return;
    
    m_tableWidget->blockSignals(true);
    m_tableWidget->setRowCount(0);
    
    QVector<VariableManager::Variable> vars = m_manager->allVariables();
    m_tableWidget->setRowCount(vars.size());
    
    for (int i = 0; i < vars.size(); ++i) {
        const VariableManager::Variable &var = vars[i];
        
        // 变量名
        QTableWidgetItem *nameItem = new QTableWidgetItem(var.name);
        if (var.isReadOnly) {
            nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        }
        m_tableWidget->setItem(i, 0, nameItem);
        
        // 类型（下拉框）
        QComboBox *typeCombo = new QComboBox();
        typeCombo->addItems({tr("字符串"), tr("日期"), tr("数字"), tr("布尔")});
        typeCombo->setCurrentIndex(static_cast<int>(var.type));
        m_tableWidget->setCellWidget(i, 1, typeCombo);
        
        // 值
        QTableWidgetItem *valueItem = new QTableWidgetItem(var.value.toString());
        m_tableWidget->setItem(i, 2, valueItem);
    }
    
    m_tableWidget->blockSignals(false);
}

// 函数说明：响应 VariableEditorWidget 收到的信号或异步回调，并更新界面状态。
void VariableEditorWidget::onAddVariable()
{
    if (!m_manager) return;
    
    // 生成唯一变量名
    int suffix = 1;
    QString baseName = "new_var";
    QString varName = baseName;
    while (m_manager->hasVariable(varName)) {
        varName = QString("%1_%2").arg(baseName).arg(suffix++);
    }
    
    m_manager->setVariable(varName, "", VariableManager::VariableType::String);
    updateTable();
    
    // 选中新添加的行并开始编辑
    int newRow = m_tableWidget->rowCount() - 1;
    m_tableWidget->selectRow(newRow);
    m_tableWidget->editItem(m_tableWidget->item(newRow, 0));
}

// 函数说明：响应 VariableEditorWidget 收到的信号或异步回调，并更新界面状态。
void VariableEditorWidget::onRemoveVariable()
{
    if (!m_manager) return;
    
    QList<QTableWidgetItem*> selected = m_tableWidget->selectedItems();
    if (selected.isEmpty()) return;
    
    int row = selected.first()->row();
    QString varName = m_tableWidget->item(row, 0)->text();
    
    if (QMessageBox::question(this, tr("确认删除"), 
            tr("确定要删除变量 '%1' 吗？").arg(varName)) == QMessageBox::Yes) {
        m_manager->removeVariable(varName);
        updateTable();
    }
}

// 函数说明：响应 VariableEditorWidget 收到的信号或异步回调，并更新界面状态。
void VariableEditorWidget::onCellChanged(int row, int column)
{
    // 标记为需要应用
    m_applyButton->setStyleSheet("background-color: #ea4335; color: white;");
}

// 函数说明：响应 VariableEditorWidget 收到的信号或异步回调，并更新界面状态。
void VariableEditorWidget::onApplyClicked()
{
    if (!m_manager) return;
    
    // 先清除所有变量
    m_manager->clearVariables();
    
    // 重新添加表格中的变量
    for (int row = 0; row < m_tableWidget->rowCount(); ++row) {
        QString name = m_tableWidget->item(row, 0)->text();
        QComboBox *typeCombo = qobject_cast<QComboBox*>(m_tableWidget->cellWidget(row, 1));
        QString value = m_tableWidget->item(row, 2)->text();
        
        VariableManager::VariableType type = VariableManager::VariableType::String;
        if (typeCombo) {
            type = static_cast<VariableManager::VariableType>(typeCombo->currentIndex());
        }
        
        m_manager->setVariable(name, value, type);
    }
    
    m_applyButton->setStyleSheet("background-color: #4285f4; color: white;");
    emit variablesModified();
}

