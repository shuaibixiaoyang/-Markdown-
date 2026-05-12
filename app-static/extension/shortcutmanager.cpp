// 文件说明：app-static\extension\shortcutmanager.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "shortcutmanager.h"

#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QDebug>

// 分类常量
const QString ShortcutManager::CategoryFile = "file";
const QString ShortcutManager::CategoryEdit = "edit";
const QString ShortcutManager::CategoryView = "view";
const QString ShortcutManager::CategoryFormat = "format";
const QString ShortcutManager::CategoryTools = "tools";
const QString ShortcutManager::CategoryHelp = "help";
const QString ShortcutManager::CategoryCustom = "custom";

// 函数说明：构造 ShortcutManager 对象，初始化本模块需要的状态、界面和资源。
ShortcutManager::ShortcutManager(QObject *parent)
    : QObject(parent)
    , m_currentScheme("default")
{
    loadSettings();
}

// 函数说明：销毁 ShortcutManager 对象，释放本模块持有的资源。
ShortcutManager::~ShortcutManager()
{
    saveSettings();
}

// 函数说明：实现 ShortcutManager::registerAction 的核心逻辑，供当前模块调用。
void ShortcutManager::registerAction(const QString &id, QAction *action,
                                    const QString &category,
                                    const QString &description)
{
    if (!action) return;

    ShortcutInfo info;
    info.id = id;
    info.name = action->text().remove('&'); // 移除快捷键提示符
    info.description = description.isEmpty() ? action->toolTip() : description;
    info.category = category;
    info.defaultKey = action->shortcut();
    info.currentKey = action->shortcut();
    info.action = action;
    info.isCustom = false;

    m_shortcuts[id] = info;

    // 如果已有自定义设置，应用它
    QSettings settings;
    settings.beginGroup("Shortcuts");
    if (settings.contains(id)) {
        QKeySequence key(settings.value(id).toString());
        info.currentKey = key;
        action->setShortcut(key);
        m_shortcuts[id] = info;
    }
    settings.endGroup();
}

// 函数说明：实现 ShortcutManager::registerShortcut 的核心逻辑，供当前模块调用。
void ShortcutManager::registerShortcut(const QString &id, const QString &name,
                                      const QKeySequence &defaultKey,
                                      const QString &category,
                                      const QString &description)
{
    ShortcutInfo info;
    info.id = id;
    info.name = name;
    info.description = description;
    info.category = category;
    info.defaultKey = defaultKey;
    info.currentKey = defaultKey;
    info.action = nullptr;
    info.isCustom = true;

    m_shortcuts[id] = info;

    // 如果已有自定义设置，应用它
    QSettings settings;
    settings.beginGroup("Shortcuts");
    if (settings.contains(id)) {
        QKeySequence key(settings.value(id).toString());
        m_shortcuts[id].currentKey = key;
    }
    settings.endGroup();
}

// 函数说明：实现 ShortcutManager::unregisterShortcut 的核心逻辑，供当前模块调用。
void ShortcutManager::unregisterShortcut(const QString &id)
{
    m_shortcuts.remove(id);
}

// 函数说明：实现 ShortcutManager::allShortcutIds 的核心逻辑，供当前模块调用。
QStringList ShortcutManager::allShortcutIds() const
{
    return m_shortcuts.keys();
}

// 函数说明：实现 ShortcutManager::shortcutIdsByCategory 的核心逻辑，供当前模块调用。
QStringList ShortcutManager::shortcutIdsByCategory(const QString &category) const
{
    QStringList result;
    for (auto it = m_shortcuts.begin(); it != m_shortcuts.end(); ++it) {
        if (it->category == category) {
            result.append(it.key());
        }
    }
    return result;
}

// 函数说明：实现 ShortcutManager::categories 的核心逻辑，供当前模块调用。
QStringList ShortcutManager::categories() const
{
    QSet<QString> cats;
    for (const auto &info : m_shortcuts) {
        cats.insert(info.category);
    }
    return cats.values();
}

// 函数说明：实现 ShortcutManager::shortcutInfo 的核心逻辑，供当前模块调用。
ShortcutInfo ShortcutManager::shortcutInfo(const QString &id) const
{
    return m_shortcuts.value(id);
}

// 函数说明：检查 ShortcutManager 是否具备对应的数据或能力。
bool ShortcutManager::hasShortcut(const QString &id) const
{
    return m_shortcuts.contains(id);
}

// 函数说明：实现 ShortcutManager::shortcut 的核心逻辑，供当前模块调用。
QKeySequence ShortcutManager::shortcut(const QString &id) const
{
    return m_shortcuts.value(id).currentKey;
}

// 函数说明：设置 ShortcutManager 的运行参数，并触发必要的界面或数据刷新。
void ShortcutManager::setShortcut(const QString &id, const QKeySequence &key)
{
    if (!m_shortcuts.contains(id)) return;

    // 检查冲突
    if (hasConflict(key, id)) {
        QString conflictId = findShortcutById(key);
        emit conflictDetected(id, conflictId, key);
    }

    ShortcutInfo &info = m_shortcuts[id];
    info.currentKey = key;

    // 更新 QAction
    if (info.action) {
        info.action->setShortcut(key);
    }

    saveSettings();
    emit shortcutChanged(id, key);
}

// 函数说明：实现 ShortcutManager::resetShortcut 的核心逻辑，供当前模块调用。
void ShortcutManager::resetShortcut(const QString &id)
{
    if (!m_shortcuts.contains(id)) return;

    ShortcutInfo &info = m_shortcuts[id];
    info.currentKey = info.defaultKey;

    if (info.action) {
        info.action->setShortcut(info.defaultKey);
    }

    saveSettings();
    emit shortcutChanged(id, info.defaultKey);
}

// 函数说明：实现 ShortcutManager::resetAllShortcuts 的核心逻辑，供当前模块调用。
void ShortcutManager::resetAllShortcuts()
{
    for (auto it = m_shortcuts.begin(); it != m_shortcuts.end(); ++it) {
        it->currentKey = it->defaultKey;
        if (it->action) {
            it->action->setShortcut(it->defaultKey);
        }
        emit shortcutChanged(it.key(), it->defaultKey);
    }
    saveSettings();
}

// 函数说明：实现 ShortcutManager::findConflicts 的核心逻辑，供当前模块调用。
QList<ShortcutConflict> ShortcutManager::findConflicts() const
{
    QList<ShortcutConflict> conflicts;
    QMap<QString, QString> keyToId; // key string -> first id

    for (auto it = m_shortcuts.begin(); it != m_shortcuts.end(); ++it) {
        if (it->currentKey.isEmpty()) continue;

        QString keyStr = it->currentKey.toString();
        if (keyToId.contains(keyStr)) {
            ShortcutConflict conflict;
            conflict.id1 = keyToId[keyStr];
            conflict.id2 = it.key();
            conflict.key = it->currentKey;
            conflicts.append(conflict);
        } else {
            keyToId[keyStr] = it.key();
        }
    }

    return conflicts;
}

// 函数说明：检查 ShortcutManager 是否具备对应的数据或能力。
bool ShortcutManager::hasConflict(const QKeySequence &key, const QString &excludeId) const
{
    if (key.isEmpty()) return false;

    for (auto it = m_shortcuts.begin(); it != m_shortcuts.end(); ++it) {
        if (it.key() == excludeId) continue;
        if (it->currentKey == key) {
            return true;
        }
    }
    return false;
}

// 函数说明：实现 ShortcutManager::findShortcutById 的核心逻辑，供当前模块调用。
QString ShortcutManager::findShortcutById(const QKeySequence &key) const
{
    for (auto it = m_shortcuts.begin(); it != m_shortcuts.end(); ++it) {
        if (it->currentKey == key) {
            return it.key();
        }
    }
    return QString();
}

// 函数说明：实现 ShortcutManager::schemes 的核心逻辑，供当前模块调用。
QStringList ShortcutManager::schemes() const
{
    QStringList result;
    result << "default" << "vim" << "emacs";

    // 添加用户自定义方案
    QDir dir(schemesDir());
    for (const QString &file : dir.entryList({"*.json"}, QDir::Files)) {
        result << QFileInfo(file).baseName();
    }

    return result;
}

// 函数说明：实现 ShortcutManager::currentScheme 的核心逻辑，供当前模块调用。
QString ShortcutManager::currentScheme() const
{
    return m_currentScheme;
}

// 函数说明：设置 ShortcutManager 的运行参数，并触发必要的界面或数据刷新。
void ShortcutManager::setCurrentScheme(const QString &scheme)
{
    if (scheme == "default") {
        applyDefaultScheme();
    } else if (scheme == "vim") {
        applyVimScheme();
    } else if (scheme == "emacs") {
        applyEmacsScheme();
    } else {
        loadScheme(scheme);
    }

    m_currentScheme = scheme;
    emit schemeChanged(scheme);
    saveSettings();
}

// 函数说明：保存 ShortcutManager 当前状态，保证用户修改可以持久化。
void ShortcutManager::saveAsScheme(const QString &name)
{
    QJsonObject root;
    QJsonObject shortcuts;

    for (auto it = m_shortcuts.begin(); it != m_shortcuts.end(); ++it) {
        shortcuts[it.key()] = it->currentKey.toString();
    }

    root["shortcuts"] = shortcuts;

    QString filePath = schemesDir() + "/" + name + ".json";
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson());
    }
}

// 函数说明：删除 ShortcutManager 管理的指定数据或资源。
void ShortcutManager::deleteScheme(const QString &name)
{
    QString filePath = schemesDir() + "/" + name + ".json";
    QFile::remove(filePath);
}

// 函数说明：实现 ShortcutManager::importScheme 的核心逻辑，供当前模块调用。
void ShortcutManager::importScheme(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject root = doc.object();
    QJsonObject shortcuts = root["shortcuts"].toObject();

    for (auto it = shortcuts.begin(); it != shortcuts.end(); ++it) {
        if (m_shortcuts.contains(it.key())) {
            setShortcut(it.key(), QKeySequence(it.value().toString()));
        }
    }
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
void ShortcutManager::exportScheme(const QString &filePath)
{
    QJsonObject root;
    QJsonObject shortcuts;

    for (auto it = m_shortcuts.begin(); it != m_shortcuts.end(); ++it) {
        shortcuts[it.key()] = it->currentKey.toString();
    }

    root["shortcuts"] = shortcuts;
    root["name"] = m_currentScheme;

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    }
}

// 函数说明：应用 ShortcutManager 当前配置，让编辑器或预览立即生效。
void ShortcutManager::applyDefaultScheme()
{
    resetAllShortcuts();
}

// 函数说明：应用 ShortcutManager 当前配置，让编辑器或预览立即生效。
void ShortcutManager::applyVimScheme()
{
    // Vim 风格快捷键
    // 这里只设置一些示例，实际应该更完整
    QMap<QString, QString> vimKeys = {
        {"fileNew", ""},
        {"fileOpen", ":e"},
        {"fileSave", ":w"},
        {"fileSaveAs", ""},
        {"editUndo", "u"},
        {"editRedo", "Ctrl+R"},
        {"editCut", "d"},
        {"editCopy", "y"},
        {"editPaste", "p"},
        {"editSelectAll", "ggVG"},
        {"editFind", "/"},
        {"editReplace", ":%s"},
        {"viewFullScreen", ""},
    };

    for (auto it = vimKeys.begin(); it != vimKeys.end(); ++it) {
        if (m_shortcuts.contains(it.key())) {
            setShortcut(it.key(), QKeySequence(it.value()));
        }
    }
}

// 函数说明：应用 ShortcutManager 当前配置，让编辑器或预览立即生效。
void ShortcutManager::applyEmacsScheme()
{
    // Emacs 风格快捷键
    QMap<QString, QString> emacsKeys = {
        {"fileNew", "Ctrl+X, Ctrl+N"},
        {"fileOpen", "Ctrl+X, Ctrl+F"},
        {"fileSave", "Ctrl+X, Ctrl+S"},
        {"fileSaveAs", "Ctrl+X, Ctrl+W"},
        {"editUndo", "Ctrl+/"},
        {"editRedo", "Ctrl+Shift+/"},
        {"editCut", "Ctrl+W"},
        {"editCopy", "Alt+W"},
        {"editPaste", "Ctrl+Y"},
        {"editSelectAll", "Ctrl+X, H"},
        {"editFind", "Ctrl+S"},
        {"editReplace", "Alt+%"},
    };

    for (auto it = emacsKeys.begin(); it != emacsKeys.end(); ++it) {
        if (m_shortcuts.contains(it.key())) {
            setShortcut(it.key(), QKeySequence(it.value()));
        }
    }
}

// 函数说明：加载 ShortcutManager 需要的数据、配置或外部资源。
void ShortcutManager::loadSettings()
{
    QSettings settings;
    settings.beginGroup("Shortcuts");

    m_currentScheme = settings.value("scheme", "default").toString();

    // 加载自定义快捷键
    for (const QString &key : settings.childKeys()) {
        if (key == "scheme") continue;
        if (m_shortcuts.contains(key)) {
            m_shortcuts[key].currentKey = QKeySequence(settings.value(key).toString());
            if (m_shortcuts[key].action) {
                m_shortcuts[key].action->setShortcut(m_shortcuts[key].currentKey);
            }
        }
    }

    settings.endGroup();
}

// 函数说明：保存 ShortcutManager 当前状态，保证用户修改可以持久化。
void ShortcutManager::saveSettings()
{
    QSettings settings;
    settings.beginGroup("Shortcuts");

    settings.setValue("scheme", m_currentScheme);

    // 只保存与默认不同的快捷键
    for (auto it = m_shortcuts.begin(); it != m_shortcuts.end(); ++it) {
        if (it->currentKey != it->defaultKey) {
            settings.setValue(it.key(), it->currentKey.toString());
        } else {
            settings.remove(it.key());
        }
    }

    settings.endGroup();
}

// 函数说明：加载 ShortcutManager 需要的数据、配置或外部资源。
void ShortcutManager::loadScheme(const QString &scheme)
{
    QString filePath = schemesDir() + "/" + scheme + ".json";
    importScheme(filePath);
}

// 函数说明：保存 ShortcutManager 当前状态，保证用户修改可以持久化。
void ShortcutManager::saveScheme(const QString &scheme)
{
    saveAsScheme(scheme);
}

// 函数说明：实现 ShortcutManager::schemesDir 的核心逻辑，供当前模块调用。
QString ShortcutManager::schemesDir() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                  + "/shortcut_schemes";
    QDir().mkpath(dir);
    return dir;
}

// ===================== ShortcutEditor =====================

ShortcutEditor::ShortcutEditor(ShortcutManager *manager, QWidget *parent)
    : QDialog(parent)
    , m_manager(manager)
    , m_currentItem(nullptr)
{
    setupUi();
    populateTree();

    setWindowTitle(tr("快捷键设置"));
    resize(600, 500);
}

// 函数说明：销毁 ShortcutEditor 对象，释放本模块持有的资源。
ShortcutEditor::~ShortcutEditor()
{
}

// 函数说明：初始化 ShortcutEditor 的 setupUi 相关界面、动作或服务连接。
void ShortcutEditor::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 方案选择
    QHBoxLayout *schemeLayout = new QHBoxLayout();
    schemeLayout->addWidget(new QLabel(tr("快捷键方案:"), this));

    m_schemeCombo = new QComboBox(this);
    m_schemeCombo->addItems(m_manager->schemes());
    m_schemeCombo->setCurrentText(m_manager->currentScheme());
    connect(m_schemeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ShortcutEditor::onSchemeChanged);
    schemeLayout->addWidget(m_schemeCombo);

    QPushButton *saveSchemeBtn = new QPushButton(tr("另存为..."), this);
    connect(saveSchemeBtn, &QPushButton::clicked, this, &ShortcutEditor::onSaveSchemeClicked);
    schemeLayout->addWidget(saveSchemeBtn);

    QPushButton *importBtn = new QPushButton(tr("导入"), this);
    connect(importBtn, &QPushButton::clicked, this, &ShortcutEditor::onImportClicked);
    schemeLayout->addWidget(importBtn);

    QPushButton *exportBtn = new QPushButton(tr("导出"), this);
    connect(exportBtn, &QPushButton::clicked, this, &ShortcutEditor::onExportClicked);
    schemeLayout->addWidget(exportBtn);

    schemeLayout->addStretch();
    mainLayout->addLayout(schemeLayout);

    // 搜索框
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("搜索快捷键..."));
    m_searchEdit->setClearButtonEnabled(true);
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &ShortcutEditor::onSearchTextChanged);
    mainLayout->addWidget(m_searchEdit);

    // 快捷键列表
    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setHeaderLabels({tr("命令"), tr("快捷键"), tr("说明")});
    m_treeWidget->setColumnWidth(0, 200);
    m_treeWidget->setColumnWidth(1, 150);
    m_treeWidget->setRootIsDecorated(true);
    m_treeWidget->setAlternatingRowColors(true);
    connect(m_treeWidget, &QTreeWidget::itemClicked,
            this, &ShortcutEditor::onItemClicked);
    mainLayout->addWidget(m_treeWidget, 1);

    // 快捷键编辑区域
    QGroupBox *editGroup = new QGroupBox(tr("编辑快捷键"), this);
    QFormLayout *editLayout = new QFormLayout(editGroup);

    m_keyEdit = new QKeySequenceEdit(this);
    connect(m_keyEdit, &QKeySequenceEdit::keySequenceChanged,
            this, &ShortcutEditor::onKeySequenceChanged);
    editLayout->addRow(tr("快捷键:"), m_keyEdit);

    m_conflictLabel = new QLabel(this);
    m_conflictLabel->setStyleSheet("color: red;");
    editLayout->addRow("", m_conflictLabel);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_resetBtn = new QPushButton(tr("重置"), this);
    connect(m_resetBtn, &QPushButton::clicked, this, &ShortcutEditor::onResetClicked);
    btnLayout->addWidget(m_resetBtn);

    m_resetAllBtn = new QPushButton(tr("全部重置"), this);
    connect(m_resetAllBtn, &QPushButton::clicked, this, &ShortcutEditor::onResetAllClicked);
    btnLayout->addWidget(m_resetAllBtn);

    btnLayout->addStretch();
    editLayout->addRow("", btnLayout);

    mainLayout->addWidget(editGroup);

    // 底部按钮
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->addStretch();

    QPushButton *closeBtn = new QPushButton(tr("关闭"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    bottomLayout->addWidget(closeBtn);

    mainLayout->addLayout(bottomLayout);
}

// 函数说明：实现 ShortcutEditor::populateTree 的核心逻辑，供当前模块调用。
void ShortcutEditor::populateTree()
{
    m_treeWidget->clear();

    // 分类名称映射
    QMap<QString, QString> categoryNames = {
        {ShortcutManager::CategoryFile, tr("文件")},
        {ShortcutManager::CategoryEdit, tr("编辑")},
        {ShortcutManager::CategoryView, tr("视图")},
        {ShortcutManager::CategoryFormat, tr("格式")},
        {ShortcutManager::CategoryTools, tr("工具")},
        {ShortcutManager::CategoryHelp, tr("帮助")},
        {ShortcutManager::CategoryCustom, tr("自定义")},
    };

    // 按分类创建节点
    QMap<QString, QTreeWidgetItem*> categoryItems;

    for (const QString &category : m_manager->categories()) {
        QTreeWidgetItem *catItem = new QTreeWidgetItem(m_treeWidget);
        catItem->setText(0, categoryNames.value(category, category));
        catItem->setExpanded(true);
        categoryItems[category] = catItem;
    }

    // 添加快捷键项
    for (const QString &id : m_manager->allShortcutIds()) {
        ShortcutInfo info = m_manager->shortcutInfo(id);

        QTreeWidgetItem *parent = categoryItems.value(info.category);
        if (!parent) continue;

        QTreeWidgetItem *item = new QTreeWidgetItem(parent);
        item->setText(0, info.name);
        item->setText(1, info.currentKey.toString(QKeySequence::NativeText));
        item->setText(2, info.description);
        item->setData(0, Qt::UserRole, id);

        // 高亮自定义的快捷键
        if (info.currentKey != info.defaultKey) {
            item->setForeground(1, QBrush(Qt::blue));
        }
    }

    highlightConflicts();
}

// 函数说明：刷新 ShortcutEditor 的内部状态，并同步到相关界面。
void ShortcutEditor::updateCurrentItem()
{
    if (!m_currentItem || m_currentId.isEmpty()) return;

    ShortcutInfo info = m_manager->shortcutInfo(m_currentId);
    m_currentItem->setText(1, info.currentKey.toString(QKeySequence::NativeText));

    // 高亮自定义的快捷键
    if (info.currentKey != info.defaultKey) {
        m_currentItem->setForeground(1, QBrush(Qt::blue));
    } else {
        m_currentItem->setForeground(1, QBrush());
    }

    highlightConflicts();
}

// 函数说明：实现 ShortcutEditor::filterItems 的核心逻辑，供当前模块调用。
void ShortcutEditor::filterItems(const QString &filter)
{
    QTreeWidgetItemIterator it(m_treeWidget);
    while (*it) {
        QTreeWidgetItem *item = *it;

        // 跳过分类节点
        if (item->parent() == nullptr) {
            ++it;
            continue;
        }

        bool matches = filter.isEmpty() ||
                      item->text(0).contains(filter, Qt::CaseInsensitive) ||
                      item->text(1).contains(filter, Qt::CaseInsensitive) ||
                      item->text(2).contains(filter, Qt::CaseInsensitive);

        item->setHidden(!matches);
        ++it;
    }

    // 隐藏空的分类
    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem *catItem = m_treeWidget->topLevelItem(i);
        bool hasVisible = false;
        for (int j = 0; j < catItem->childCount(); ++j) {
            if (!catItem->child(j)->isHidden()) {
                hasVisible = true;
                break;
            }
        }
        catItem->setHidden(!hasVisible);
    }
}

// 函数说明：实现 ShortcutEditor::highlightConflicts 的核心逻辑，供当前模块调用。
void ShortcutEditor::highlightConflicts()
{
    // 清除之前的高亮
    QTreeWidgetItemIterator it(m_treeWidget);
    while (*it) {
        (*it)->setBackground(1, QBrush());
        ++it;
    }

    // 高亮冲突
    QList<ShortcutConflict> conflicts = m_manager->findConflicts();
    for (const auto &conflict : conflicts) {
        QTreeWidgetItemIterator it(m_treeWidget);
        while (*it) {
            QString id = (*it)->data(0, Qt::UserRole).toString();
            if (id == conflict.id1 || id == conflict.id2) {
                (*it)->setBackground(1, QBrush(QColor(255, 200, 200)));
            }
            ++it;
        }
    }
}

// 函数说明：响应 ShortcutEditor 收到的信号或异步回调，并更新界面状态。
void ShortcutEditor::onItemClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column)

    QString id = item->data(0, Qt::UserRole).toString();
    if (id.isEmpty()) return; // 分类节点

    m_currentItem = item;
    m_currentId = id;

    ShortcutInfo info = m_manager->shortcutInfo(id);
    m_keyEdit->setKeySequence(info.currentKey);
    m_conflictLabel->clear();
}

// 函数说明：响应 ShortcutEditor 收到的信号或异步回调，并更新界面状态。
void ShortcutEditor::onSearchTextChanged(const QString &text)
{
    filterItems(text);
}

// 函数说明：响应 ShortcutEditor 收到的信号或异步回调，并更新界面状态。
void ShortcutEditor::onKeySequenceChanged(const QKeySequence &key)
{
    if (m_currentId.isEmpty()) return;

    // 检查冲突
    if (m_manager->hasConflict(key, m_currentId)) {
        QString conflictId = m_manager->findShortcutById(key);
        ShortcutInfo conflictInfo = m_manager->shortcutInfo(conflictId);
        m_conflictLabel->setText(tr("冲突: %1").arg(conflictInfo.name));
    } else {
        m_conflictLabel->clear();
    }

    m_manager->setShortcut(m_currentId, key);
    updateCurrentItem();
}

// 函数说明：响应 ShortcutEditor 收到的信号或异步回调，并更新界面状态。
void ShortcutEditor::onResetClicked()
{
    if (m_currentId.isEmpty()) return;

    m_manager->resetShortcut(m_currentId);

    ShortcutInfo info = m_manager->shortcutInfo(m_currentId);
    m_keyEdit->setKeySequence(info.currentKey);
    m_conflictLabel->clear();

    updateCurrentItem();
}

// 函数说明：响应 ShortcutEditor 收到的信号或异步回调，并更新界面状态。
void ShortcutEditor::onResetAllClicked()
{
    if (QMessageBox::question(this, tr("确认"),
        tr("确定要重置所有快捷键吗？")) != QMessageBox::Yes) {
        return;
    }

    m_manager->resetAllShortcuts();
    populateTree();

    if (!m_currentId.isEmpty()) {
        ShortcutInfo info = m_manager->shortcutInfo(m_currentId);
        m_keyEdit->setKeySequence(info.currentKey);
    }
}

// 函数说明：响应 ShortcutEditor 收到的信号或异步回调，并更新界面状态。
void ShortcutEditor::onSchemeChanged(int index)
{
    QString scheme = m_schemeCombo->itemText(index);
    m_manager->setCurrentScheme(scheme);
    populateTree();
}

// 函数说明：响应 ShortcutEditor 收到的信号或异步回调，并更新界面状态。
void ShortcutEditor::onSaveSchemeClicked()
{
    bool ok;
    QString name = QInputDialog::getText(this, tr("保存方案"),
        tr("方案名称:"), QLineEdit::Normal, "", &ok);

    if (ok && !name.isEmpty()) {
        m_manager->saveAsScheme(name);
        m_schemeCombo->addItem(name);
        m_schemeCombo->setCurrentText(name);
    }
}

// 函数说明：响应 ShortcutEditor 收到的信号或异步回调，并更新界面状态。
void ShortcutEditor::onImportClicked()
{
    QString filePath = QFileDialog::getOpenFileName(this,
        tr("导入快捷键方案"), QString(),
        tr("JSON 文件 (*.json)"));

    if (!filePath.isEmpty()) {
        m_manager->importScheme(filePath);
        populateTree();
    }
}

// 函数说明：响应 ShortcutEditor 收到的信号或异步回调，并更新界面状态。
void ShortcutEditor::onExportClicked()
{
    QString filePath = QFileDialog::getSaveFileName(this,
        tr("导出快捷键方案"),
        m_manager->currentScheme() + ".json",
        tr("JSON 文件 (*.json)"));

    if (!filePath.isEmpty()) {
        m_manager->exportScheme(filePath);
    }
}

