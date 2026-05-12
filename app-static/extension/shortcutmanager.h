// 文件说明：app-static\extension\shortcutmanager.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef SHORTCUTMANAGER_H
#define SHORTCUTMANAGER_H

#include <QObject>
#include <QMap>
#include <QKeySequence>
#include <QAction>
#include <QDialog>
#include <QTreeWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QComboBox>

class QWidget;

/**
 * @brief 快捷键信息
 */
struct ShortcutInfo {
    QString id;              // 唯一标识符
    QString name;            // 显示名称
    QString description;     // 描述
    QString category;        // 分类
    QKeySequence defaultKey; // 默认快捷键
    QKeySequence currentKey; // 当前快捷键
    QAction *action;         // 关联的 QAction
    bool isCustom;           // 是否为自定义（非内置）

    ShortcutInfo()
        : action(nullptr)
        , isCustom(false) {}
};

/**
 * @brief 快捷键冲突信息
 */
struct ShortcutConflict {
    QString id1;
    QString id2;
    QKeySequence key;
};

/**
 * @brief 快捷键管理器
 *
 * 功能：
 * - 统一管理所有快捷键
 * - 允许用户自定义快捷键
 * - 检测快捷键冲突
 * - 导入/导出快捷键配置
 * - 支持多套快捷键方案
 */
class ShortcutManager : public QObject
{
    Q_OBJECT

public:
    explicit ShortcutManager(QObject *parent = nullptr);
    ~ShortcutManager();

    // 快捷键分类
    static const QString CategoryFile;
    static const QString CategoryEdit;
    static const QString CategoryView;
    static const QString CategoryFormat;
    static const QString CategoryTools;
    static const QString CategoryHelp;
    static const QString CategoryCustom;

    // 注册快捷键
    void registerAction(const QString &id, QAction *action,
                       const QString &category = CategoryCustom,
                       const QString &description = QString());

    void registerShortcut(const QString &id, const QString &name,
                         const QKeySequence &defaultKey,
                         const QString &category = CategoryCustom,
                         const QString &description = QString());

    void unregisterShortcut(const QString &id);

    // 查询
    QStringList allShortcutIds() const;
    QStringList shortcutIdsByCategory(const QString &category) const;
    QStringList categories() const;
    ShortcutInfo shortcutInfo(const QString &id) const;
    bool hasShortcut(const QString &id) const;

    // 获取/设置快捷键
    QKeySequence shortcut(const QString &id) const;
    void setShortcut(const QString &id, const QKeySequence &key);
    void resetShortcut(const QString &id);
    void resetAllShortcuts();

    // 冲突检测
    QList<ShortcutConflict> findConflicts() const;
    bool hasConflict(const QKeySequence &key, const QString &excludeId = QString()) const;
    QString findShortcutById(const QKeySequence &key) const;

    // 快捷键方案
    QStringList schemes() const;
    QString currentScheme() const;
    void setCurrentScheme(const QString &scheme);
    void saveAsScheme(const QString &name);
    void deleteScheme(const QString &name);
    void importScheme(const QString &filePath);
    void exportScheme(const QString &filePath);

    // 内置方案
    void applyDefaultScheme();
    void applyVimScheme();
    void applyEmacsScheme();

signals:
    void shortcutChanged(const QString &id, const QKeySequence &key);
    void schemeChanged(const QString &scheme);
    void conflictDetected(const QString &id1, const QString &id2, const QKeySequence &key);

private:
    void loadSettings();
    void saveSettings();
    void loadScheme(const QString &scheme);
    void saveScheme(const QString &scheme);
    QString schemesDir() const;

    QMap<QString, ShortcutInfo> m_shortcuts;
    QString m_currentScheme;
};

/**
 * @brief 快捷键编辑器（用于设置对话框）
 */
class ShortcutEditor : public QDialog
{
    Q_OBJECT

public:
    explicit ShortcutEditor(ShortcutManager *manager, QWidget *parent = nullptr);
    ~ShortcutEditor();

private slots:
    void onItemClicked(QTreeWidgetItem *item, int column);
    void onSearchTextChanged(const QString &text);
    void onKeySequenceChanged(const QKeySequence &key);
    void onResetClicked();
    void onResetAllClicked();
    void onSchemeChanged(int index);
    void onSaveSchemeClicked();
    void onImportClicked();
    void onExportClicked();

private:
    void setupUi();
    void populateTree();
    void updateCurrentItem();
    void filterItems(const QString &filter);
    void highlightConflicts();

    ShortcutManager *m_manager;

    QTreeWidget *m_treeWidget;
    QLineEdit *m_searchEdit;
    QKeySequenceEdit *m_keyEdit;
    QLabel *m_conflictLabel;
    QPushButton *m_resetBtn;
    QPushButton *m_resetAllBtn;
    QComboBox *m_schemeCombo;

    QTreeWidgetItem *m_currentItem;
    QString m_currentId;
};

#endif // SHORTCUTMANAGER_H

