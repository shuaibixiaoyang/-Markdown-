// 文件说明：app-static\extension\editortheme.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef EDITORTHEME_H
#define EDITORTHEME_H

#include <QObject>
#include <QColor>
#include <QFont>
#include <QMap>
#include <QString>
#include <QDialog>
#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QFontComboBox>
#include <QLabel>
#include <QGroupBox>
#include <QPlainTextEdit>

class QTextCharFormat;

/**
 * @brief 语法高亮颜色配置
 */
struct SyntaxColors {
    QColor heading1;        // H1 标题
    QColor heading2;        // H2 标题
    QColor heading3;        // H3 标题
    QColor heading4;        // H4-H6 标题
    QColor bold;            // 粗体
    QColor italic;          // 斜体
    QColor strikethrough;   // 删除线
    QColor code;            // 行内代码
    QColor codeBlock;       // 代码块
    QColor link;            // 链接
    QColor linkUrl;         // 链接 URL
    QColor image;           // 图片
    QColor blockquote;      // 引用
    QColor listMarker;      // 列表标记
    QColor horizontalRule;  // 水平线
    QColor comment;         // HTML 注释
    QColor htmlTag;         // HTML 标签
    QColor escape;          // 转义字符

    SyntaxColors();
};

/**
 * @brief 编辑器主题配置
 */
struct EditorTheme {
    QString id;             // 唯一标识符
    QString name;           // 显示名称
    QString author;         // 作者
    QString description;    // 描述
    bool isBuiltin;         // 是否内置

    // 编辑器颜色
    QColor background;      // 背景色
    QColor foreground;      // 前景色（普通文本）
    QColor selection;       // 选中背景色
    QColor selectionText;   // 选中文本色
    QColor currentLine;     // 当前行高亮色
    QColor lineNumber;      // 行号颜色
    QColor lineNumberBg;    // 行号背景色
    QColor cursor;          // 光标颜色
    QColor matchBracket;    // 匹配括号颜色
    QColor searchHighlight; // 搜索高亮色

    // 语法高亮颜色
    SyntaxColors syntax;

    // 字体设置
    QString fontFamily;
    int fontSize;
    int lineSpacing;        // 行间距（百分比）
    int letterSpacing;      // 字间距

    // 边距设置
    int marginLeft;
    int marginRight;
    int marginTop;
    int marginBottom;

    EditorTheme();

    // 序列化
    QJsonObject toJson() const;
    static EditorTheme fromJson(const QJsonObject &json);
};

/**
 * @brief 编辑器主题管理器
 *
 * 功能：
 * - 管理编辑器配色主题
 * - 应用主题到编辑器
 * - 支持自定义主题
 * - 主题导入/导出
 */
class EditorThemeManager : public QObject
{
    Q_OBJECT

public:
    explicit EditorThemeManager(QObject *parent = nullptr);
    ~EditorThemeManager();

    // 设置编辑器
    void setEditor(QPlainTextEdit *editor);

    // 主题管理
    QStringList availableThemes() const;
    EditorTheme theme(const QString &id) const;
    QString currentThemeId() const;

    // 应用主题
    void applyTheme(const QString &id);
    void applyTheme(const EditorTheme &theme);

    // 自定义主题
    void addTheme(const EditorTheme &theme);
    void updateTheme(const EditorTheme &theme);
    void removeTheme(const QString &id);

    // 导入/导出
    bool importTheme(const QString &filePath);
    bool exportTheme(const QString &id, const QString &filePath);

    // 获取当前主题的语法格式
    QTextCharFormat formatForElement(const QString &element) const;

signals:
    void themeChanged(const QString &id);
    void themeAdded(const QString &id);
    void themeRemoved(const QString &id);

private:
    void loadBuiltinThemes();
    void loadUserThemes();
    void saveUserThemes();
    void applyToEditor();
    QString userThemesDir() const;

    QPlainTextEdit *m_editor;
    QMap<QString, EditorTheme> m_themes;
    QString m_currentThemeId;
    EditorTheme m_currentTheme;
};

/**
 * @brief 主题编辑器对话框
 */
class ColorButton : public QPushButton
{
    Q_OBJECT

public:
    explicit ColorButton(QWidget *parent = nullptr);

    QColor color() const;
    void setColor(const QColor &color);

signals:
    void colorChanged(const QColor &color);

private slots:
    void chooseColor();

private:
    void updateButtonColor();
    QColor m_color;
};

class EditorThemeEditor : public QDialog
{
    Q_OBJECT

public:
    explicit EditorThemeEditor(EditorThemeManager *manager, QWidget *parent = nullptr);
    ~EditorThemeEditor();

private slots:
    void onThemeSelected(int index);
    void onNewTheme();
    void onDeleteTheme();
    void onImportTheme();
    void onExportTheme();
    void onApply();
    void onColorChanged();
    void onFontChanged();
    void onPreview();

private:
    void setupUi();
    void loadThemeToUi(const EditorTheme &theme);
    EditorTheme uiToTheme();
    void updatePreview();

    EditorThemeManager *m_manager;

    QComboBox *m_themeCombo;
    QLineEdit *m_nameEdit;
    QLineEdit *m_authorEdit;
    QLineEdit *m_descriptionEdit;

    // 颜色按钮
    ColorButton *m_bgColorBtn;
    ColorButton *m_fgColorBtn;
    ColorButton *m_selectionColorBtn;
    ColorButton *m_currentLineColorBtn;
    ColorButton *m_lineNumberColorBtn;
    ColorButton *m_cursorColorBtn;

    // 语法颜色
    ColorButton *m_heading1ColorBtn;
    ColorButton *m_heading2ColorBtn;
    ColorButton *m_boldColorBtn;
    ColorButton *m_italicColorBtn;
    ColorButton *m_codeColorBtn;
    ColorButton *m_linkColorBtn;
    ColorButton *m_blockquoteColorBtn;

    // 字体设置
    QFontComboBox *m_fontCombo;
    QSpinBox *m_fontSizeSpin;
    QSpinBox *m_lineSpacingSpin;

    // 预览
    QPlainTextEdit *m_previewEdit;

    QString m_currentEditingId;
    bool m_isModified;
};

#endif // EDITORTHEME_H

