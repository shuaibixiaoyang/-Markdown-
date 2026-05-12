// 文件说明：app-static\extension\editortheme.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "editortheme.h"

#include <QPlainTextEdit>
#include <QTextCharFormat>
#include <QScrollBar>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QTabWidget>
#include <QColorDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QDebug>

// ===================== SyntaxColors =====================

SyntaxColors::SyntaxColors()
    : heading1("#1a73e8")
    , heading2("#188038")
    , heading3("#a142f4")
    , heading4("#e37400")
    , bold("#000000")
    , italic("#666666")
    , strikethrough("#999999")
    , code("#d93025")
    , codeBlock("#1a73e8")
    , link("#1a73e8")
    , linkUrl("#888888")
    , image("#188038")
    , blockquote("#666666")
    , listMarker("#1a73e8")
    , horizontalRule("#cccccc")
    , comment("#999999")
    , htmlTag("#a142f4")
    , escape("#e37400")
{
}

// ===================== EditorTheme =====================

EditorTheme::EditorTheme()
    : isBuiltin(false)
    , background("#ffffff")
    , foreground("#333333")
    , selection("#b4d7ff")
    , selectionText("#000000")
    , currentLine("#f5f5f5")
    , lineNumber("#999999")
    , lineNumberBg("#f5f5f5")
    , cursor("#000000")
    , matchBracket("#ffeb3b")
    , searchHighlight("#ffff00")
    , fontFamily("SF Mono")
    , fontSize(14)
    , lineSpacing(140)
    , letterSpacing(0)
    , marginLeft(10)
    , marginRight(10)
    , marginTop(10)
    , marginBottom(10)
{
}

// 函数说明：实现 EditorTheme::toJson 的核心逻辑，供当前模块调用。
QJsonObject EditorTheme::toJson() const
{
    QJsonObject json;

    json["id"] = id;
    json["name"] = name;
    json["author"] = author;
    json["description"] = description;

    // 编辑器颜色
    QJsonObject colors;
    colors["background"] = background.name();
    colors["foreground"] = foreground.name();
    colors["selection"] = selection.name();
    colors["selectionText"] = selectionText.name();
    colors["currentLine"] = currentLine.name();
    colors["lineNumber"] = lineNumber.name();
    colors["lineNumberBg"] = lineNumberBg.name();
    colors["cursor"] = cursor.name();
    colors["matchBracket"] = matchBracket.name();
    colors["searchHighlight"] = searchHighlight.name();
    json["colors"] = colors;

    // 语法颜色
    QJsonObject syntaxJson;
    syntaxJson["heading1"] = syntax.heading1.name();
    syntaxJson["heading2"] = syntax.heading2.name();
    syntaxJson["heading3"] = syntax.heading3.name();
    syntaxJson["heading4"] = syntax.heading4.name();
    syntaxJson["bold"] = syntax.bold.name();
    syntaxJson["italic"] = syntax.italic.name();
    syntaxJson["strikethrough"] = syntax.strikethrough.name();
    syntaxJson["code"] = syntax.code.name();
    syntaxJson["codeBlock"] = syntax.codeBlock.name();
    syntaxJson["link"] = syntax.link.name();
    syntaxJson["linkUrl"] = syntax.linkUrl.name();
    syntaxJson["image"] = syntax.image.name();
    syntaxJson["blockquote"] = syntax.blockquote.name();
    syntaxJson["listMarker"] = syntax.listMarker.name();
    syntaxJson["horizontalRule"] = syntax.horizontalRule.name();
    syntaxJson["comment"] = syntax.comment.name();
    syntaxJson["htmlTag"] = syntax.htmlTag.name();
    syntaxJson["escape"] = syntax.escape.name();
    json["syntax"] = syntaxJson;

    // 字体
    QJsonObject font;
    font["family"] = fontFamily;
    font["size"] = fontSize;
    font["lineSpacing"] = lineSpacing;
    font["letterSpacing"] = letterSpacing;
    json["font"] = font;

    // 边距
    QJsonObject margins;
    margins["left"] = marginLeft;
    margins["right"] = marginRight;
    margins["top"] = marginTop;
    margins["bottom"] = marginBottom;
    json["margins"] = margins;

    return json;
}

// 函数说明：实现 EditorTheme::fromJson 的核心逻辑，供当前模块调用。
EditorTheme EditorTheme::fromJson(const QJsonObject &json)
{
    EditorTheme theme;

    theme.id = json["id"].toString();
    theme.name = json["name"].toString();
    theme.author = json["author"].toString();
    theme.description = json["description"].toString();

    // 编辑器颜色
    QJsonObject colors = json["colors"].toObject();
    theme.background = QColor(colors["background"].toString());
    theme.foreground = QColor(colors["foreground"].toString());
    theme.selection = QColor(colors["selection"].toString());
    theme.selectionText = QColor(colors["selectionText"].toString());
    theme.currentLine = QColor(colors["currentLine"].toString());
    theme.lineNumber = QColor(colors["lineNumber"].toString());
    theme.lineNumberBg = QColor(colors["lineNumberBg"].toString());
    theme.cursor = QColor(colors["cursor"].toString());
    theme.matchBracket = QColor(colors["matchBracket"].toString());
    theme.searchHighlight = QColor(colors["searchHighlight"].toString());

    // 语法颜色
    QJsonObject syntaxJson = json["syntax"].toObject();
    theme.syntax.heading1 = QColor(syntaxJson["heading1"].toString());
    theme.syntax.heading2 = QColor(syntaxJson["heading2"].toString());
    theme.syntax.heading3 = QColor(syntaxJson["heading3"].toString());
    theme.syntax.heading4 = QColor(syntaxJson["heading4"].toString());
    theme.syntax.bold = QColor(syntaxJson["bold"].toString());
    theme.syntax.italic = QColor(syntaxJson["italic"].toString());
    theme.syntax.strikethrough = QColor(syntaxJson["strikethrough"].toString());
    theme.syntax.code = QColor(syntaxJson["code"].toString());
    theme.syntax.codeBlock = QColor(syntaxJson["codeBlock"].toString());
    theme.syntax.link = QColor(syntaxJson["link"].toString());
    theme.syntax.linkUrl = QColor(syntaxJson["linkUrl"].toString());
    theme.syntax.image = QColor(syntaxJson["image"].toString());
    theme.syntax.blockquote = QColor(syntaxJson["blockquote"].toString());
    theme.syntax.listMarker = QColor(syntaxJson["listMarker"].toString());
    theme.syntax.horizontalRule = QColor(syntaxJson["horizontalRule"].toString());
    theme.syntax.comment = QColor(syntaxJson["comment"].toString());
    theme.syntax.htmlTag = QColor(syntaxJson["htmlTag"].toString());
    theme.syntax.escape = QColor(syntaxJson["escape"].toString());

    // 字体
    QJsonObject font = json["font"].toObject();
    theme.fontFamily = font["family"].toString();
    theme.fontSize = font["size"].toInt();
    theme.lineSpacing = font["lineSpacing"].toInt();
    theme.letterSpacing = font["letterSpacing"].toInt();

    // 边距
    QJsonObject margins = json["margins"].toObject();
    theme.marginLeft = margins["left"].toInt();
    theme.marginRight = margins["right"].toInt();
    theme.marginTop = margins["top"].toInt();
    theme.marginBottom = margins["bottom"].toInt();

    return theme;
}

// ===================== EditorThemeManager =====================

EditorThemeManager::EditorThemeManager(QObject *parent)
    : QObject(parent)
    , m_editor(nullptr)
    , m_currentThemeId("light")
{
    loadBuiltinThemes();
    loadUserThemes();

    // 加载上次使用的主题
    QSettings settings;
    m_currentThemeId = settings.value("Editor/theme", "light").toString();
    if (m_themes.contains(m_currentThemeId)) {
        m_currentTheme = m_themes[m_currentThemeId];
    }
}

// 函数说明：销毁 EditorThemeManager 对象，释放本模块持有的资源。
EditorThemeManager::~EditorThemeManager()
{
    saveUserThemes();
}

// 函数说明：设置 EditorThemeManager 的运行参数，并触发必要的界面或数据刷新。
void EditorThemeManager::setEditor(QPlainTextEdit *editor)
{
    m_editor = editor;
    applyToEditor();
}

// 函数说明：实现 EditorThemeManager::availableThemes 的核心逻辑，供当前模块调用。
QStringList EditorThemeManager::availableThemes() const
{
    return m_themes.keys();
}

// 函数说明：实现 EditorThemeManager::theme 的核心逻辑，供当前模块调用。
EditorTheme EditorThemeManager::theme(const QString &id) const
{
    return m_themes.value(id);
}

// 函数说明：实现 EditorThemeManager::currentThemeId 的核心逻辑，供当前模块调用。
QString EditorThemeManager::currentThemeId() const
{
    return m_currentThemeId;
}

// 函数说明：应用 EditorThemeManager 当前配置，让编辑器或预览立即生效。
void EditorThemeManager::applyTheme(const QString &id)
{
    if (!m_themes.contains(id)) return;

    m_currentThemeId = id;
    m_currentTheme = m_themes[id];
    applyToEditor();

    QSettings settings;
    settings.setValue("Editor/theme", id);

    emit themeChanged(id);
}

// 函数说明：应用 EditorThemeManager 当前配置，让编辑器或预览立即生效。
void EditorThemeManager::applyTheme(const EditorTheme &theme)
{
    m_currentTheme = theme;
    applyToEditor();
}

// 函数说明：向 EditorThemeManager 管理的数据集合中添加一项内容。
void EditorThemeManager::addTheme(const EditorTheme &theme)
{
    m_themes[theme.id] = theme;
    saveUserThemes();
    emit themeAdded(theme.id);
}

// 函数说明：刷新 EditorThemeManager 的内部状态，并同步到相关界面。
void EditorThemeManager::updateTheme(const EditorTheme &theme)
{
    if (!m_themes.contains(theme.id)) return;

    m_themes[theme.id] = theme;
    saveUserThemes();

    if (m_currentThemeId == theme.id) {
        m_currentTheme = theme;
        applyToEditor();
    }
}

// 函数说明：从 EditorThemeManager 管理的数据集合中移除指定内容。
void EditorThemeManager::removeTheme(const QString &id)
{
    if (!m_themes.contains(id)) return;
    if (m_themes[id].isBuiltin) return; // 不能删除内置主题

    m_themes.remove(id);
    saveUserThemes();
    emit themeRemoved(id);

    if (m_currentThemeId == id) {
        applyTheme("light");
    }
}

// 函数说明：实现 EditorThemeManager::importTheme 的核心逻辑，供当前模块调用。
bool EditorThemeManager::importTheme(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull()) {
        return false;
    }

    EditorTheme theme = EditorTheme::fromJson(doc.object());
    if (theme.id.isEmpty()) {
        return false;
    }

    theme.isBuiltin = false;
    addTheme(theme);
    return true;
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
bool EditorThemeManager::exportTheme(const QString &id, const QString &filePath)
{
    if (!m_themes.contains(id)) return false;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QJsonDocument doc(m_themes[id].toJson());
    file.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

// 函数说明：实现 EditorThemeManager::formatForElement 的核心逻辑，供当前模块调用。
QTextCharFormat EditorThemeManager::formatForElement(const QString &element) const
{
    QTextCharFormat format;

    if (element == "heading1") {
        format.setForeground(m_currentTheme.syntax.heading1);
        format.setFontWeight(QFont::Bold);
        format.setFontPointSize(m_currentTheme.fontSize * 1.5);
    } else if (element == "heading2") {
        format.setForeground(m_currentTheme.syntax.heading2);
        format.setFontWeight(QFont::Bold);
        format.setFontPointSize(m_currentTheme.fontSize * 1.3);
    } else if (element == "heading3") {
        format.setForeground(m_currentTheme.syntax.heading3);
        format.setFontWeight(QFont::Bold);
        format.setFontPointSize(m_currentTheme.fontSize * 1.1);
    } else if (element == "heading4") {
        format.setForeground(m_currentTheme.syntax.heading4);
        format.setFontWeight(QFont::Bold);
    } else if (element == "bold") {
        format.setForeground(m_currentTheme.syntax.bold);
        format.setFontWeight(QFont::Bold);
    } else if (element == "italic") {
        format.setForeground(m_currentTheme.syntax.italic);
        format.setFontItalic(true);
    } else if (element == "strikethrough") {
        format.setForeground(m_currentTheme.syntax.strikethrough);
        format.setFontStrikeOut(true);
    } else if (element == "code") {
        format.setForeground(m_currentTheme.syntax.code);
        format.setFontFamily("monospace");
    } else if (element == "codeBlock") {
        format.setForeground(m_currentTheme.syntax.codeBlock);
        format.setFontFamily("monospace");
    } else if (element == "link") {
        format.setForeground(m_currentTheme.syntax.link);
        format.setFontUnderline(true);
    } else if (element == "linkUrl") {
        format.setForeground(m_currentTheme.syntax.linkUrl);
    } else if (element == "image") {
        format.setForeground(m_currentTheme.syntax.image);
    } else if (element == "blockquote") {
        format.setForeground(m_currentTheme.syntax.blockquote);
        format.setFontItalic(true);
    } else if (element == "listMarker") {
        format.setForeground(m_currentTheme.syntax.listMarker);
        format.setFontWeight(QFont::Bold);
    } else if (element == "horizontalRule") {
        format.setForeground(m_currentTheme.syntax.horizontalRule);
    } else if (element == "comment") {
        format.setForeground(m_currentTheme.syntax.comment);
        format.setFontItalic(true);
    } else if (element == "htmlTag") {
        format.setForeground(m_currentTheme.syntax.htmlTag);
    } else if (element == "escape") {
        format.setForeground(m_currentTheme.syntax.escape);
    }

    return format;
}

// 函数说明：加载 EditorThemeManager 需要的数据、配置或外部资源。
void EditorThemeManager::loadBuiltinThemes()
{
    // Light 主题
    EditorTheme light;
    light.id = "light";
    light.name = tr("明亮");
    light.author = "CuteMarkEd";
    light.description = tr("默认明亮主题");
    light.isBuiltin = true;
    m_themes["light"] = light;

    // Dark 主题
    EditorTheme dark;
    dark.id = "dark";
    dark.name = tr("暗黑");
    dark.author = "CuteMarkEd";
    dark.description = tr("暗黑主题");
    dark.isBuiltin = true;
    dark.background = QColor("#1e1e1e");
    dark.foreground = QColor("#d4d4d4");
    dark.selection = QColor("#264f78");
    dark.selectionText = QColor("#ffffff");
    dark.currentLine = QColor("#2d2d2d");
    dark.lineNumber = QColor("#858585");
    dark.lineNumberBg = QColor("#1e1e1e");
    dark.cursor = QColor("#ffffff");
    dark.syntax.heading1 = QColor("#4fc1ff");
    dark.syntax.heading2 = QColor("#4ec9b0");
    dark.syntax.heading3 = QColor("#c586c0");
    dark.syntax.heading4 = QColor("#ce9178");
    dark.syntax.bold = QColor("#d4d4d4");
    dark.syntax.italic = QColor("#9cdcfe");
    dark.syntax.code = QColor("#ce9178");
    dark.syntax.codeBlock = QColor("#4fc1ff");
    dark.syntax.link = QColor("#4fc1ff");
    dark.syntax.blockquote = QColor("#6a9955");
    m_themes["dark"] = dark;

    // Sepia 主题
    EditorTheme sepia;
    sepia.id = "sepia";
    sepia.name = tr("护眼");
    sepia.author = "CuteMarkEd";
    sepia.description = tr("温暖护眼主题");
    sepia.isBuiltin = true;
    sepia.background = QColor("#f4ecd8");
    sepia.foreground = QColor("#5b4636");
    sepia.selection = QColor("#d4c4a8");
    sepia.currentLine = QColor("#e8dcc8");
    sepia.lineNumber = QColor("#a89880");
    sepia.lineNumberBg = QColor("#f4ecd8");
    sepia.syntax.heading1 = QColor("#8b4513");
    sepia.syntax.heading2 = QColor("#2e8b57");
    sepia.syntax.code = QColor("#a0522d");
    sepia.syntax.link = QColor("#4169e1");
    m_themes["sepia"] = sepia;

    // Monokai 主题
    EditorTheme monokai;
    monokai.id = "monokai";
    monokai.name = tr("Monokai");
    monokai.author = "CuteMarkEd";
    monokai.description = tr("经典 Monokai 配色");
    monokai.isBuiltin = true;
    monokai.background = QColor("#272822");
    monokai.foreground = QColor("#f8f8f2");
    monokai.selection = QColor("#49483e");
    monokai.currentLine = QColor("#3e3d32");
    monokai.lineNumber = QColor("#75715e");
    monokai.lineNumberBg = QColor("#272822");
    monokai.cursor = QColor("#f8f8f0");
    monokai.syntax.heading1 = QColor("#f92672");
    monokai.syntax.heading2 = QColor("#66d9ef");
    monokai.syntax.heading3 = QColor("#a6e22e");
    monokai.syntax.heading4 = QColor("#fd971f");
    monokai.syntax.bold = QColor("#f8f8f2");
    monokai.syntax.italic = QColor("#e6db74");
    monokai.syntax.code = QColor("#e6db74");
    monokai.syntax.link = QColor("#66d9ef");
    monokai.syntax.blockquote = QColor("#75715e");
    m_themes["monokai"] = monokai;

    // Solarized Light
    EditorTheme solarizedLight;
    solarizedLight.id = "solarized-light";
    solarizedLight.name = tr("Solarized Light");
    solarizedLight.author = "Ethan Schoonover";
    solarizedLight.description = tr("Solarized 明亮主题");
    solarizedLight.isBuiltin = true;
    solarizedLight.background = QColor("#fdf6e3");
    solarizedLight.foreground = QColor("#657b83");
    solarizedLight.selection = QColor("#eee8d5");
    solarizedLight.currentLine = QColor("#eee8d5");
    solarizedLight.lineNumber = QColor("#93a1a1");
    solarizedLight.syntax.heading1 = QColor("#cb4b16");
    solarizedLight.syntax.heading2 = QColor("#859900");
    solarizedLight.syntax.code = QColor("#d33682");
    solarizedLight.syntax.link = QColor("#268bd2");
    m_themes["solarized-light"] = solarizedLight;

    // Solarized Dark
    EditorTheme solarizedDark;
    solarizedDark.id = "solarized-dark";
    solarizedDark.name = tr("Solarized Dark");
    solarizedDark.author = "Ethan Schoonover";
    solarizedDark.description = tr("Solarized 暗黑主题");
    solarizedDark.isBuiltin = true;
    solarizedDark.background = QColor("#002b36");
    solarizedDark.foreground = QColor("#839496");
    solarizedDark.selection = QColor("#073642");
    solarizedDark.currentLine = QColor("#073642");
    solarizedDark.lineNumber = QColor("#586e75");
    solarizedDark.syntax.heading1 = QColor("#cb4b16");
    solarizedDark.syntax.heading2 = QColor("#859900");
    solarizedDark.syntax.code = QColor("#d33682");
    solarizedDark.syntax.link = QColor("#268bd2");
    m_themes["solarized-dark"] = solarizedDark;
}

// 函数说明：加载 EditorThemeManager 需要的数据、配置或外部资源。
void EditorThemeManager::loadUserThemes()
{
    QDir dir(userThemesDir());
    for (const QString &file : dir.entryList({"*.json"}, QDir::Files)) {
        QString filePath = dir.absoluteFilePath(file);
        QFile f(filePath);
        if (f.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
            if (!doc.isNull()) {
                EditorTheme theme = EditorTheme::fromJson(doc.object());
                theme.isBuiltin = false;
                if (!theme.id.isEmpty()) {
                    m_themes[theme.id] = theme;
                }
            }
        }
    }
}

// 函数说明：保存 EditorThemeManager 当前状态，保证用户修改可以持久化。
void EditorThemeManager::saveUserThemes()
{
    QDir dir(userThemesDir());
    dir.mkpath(".");

    for (auto it = m_themes.begin(); it != m_themes.end(); ++it) {
        if (!it->isBuiltin) {
            QString filePath = dir.absoluteFilePath(it->id + ".json");
            QFile file(filePath);
            if (file.open(QIODevice::WriteOnly)) {
                QJsonDocument doc(it->toJson());
                file.write(doc.toJson(QJsonDocument::Indented));
            }
        }
    }
}

// 函数说明：应用 EditorThemeManager 当前配置，让编辑器或预览立即生效。
void EditorThemeManager::applyToEditor()
{
    if (!m_editor) return;

    // 设置样式表
    QString styleSheet = QString(R"(
        QPlainTextEdit {
            background-color: %1;
            color: %2;
            selection-background-color: %3;
            selection-color: %4;
            font-family: %5;
            font-size: %6px;
        }
    )")
    .arg(m_currentTheme.background.name())
    .arg(m_currentTheme.foreground.name())
    .arg(m_currentTheme.selection.name())
    .arg(m_currentTheme.selectionText.name())
    .arg(m_currentTheme.fontFamily)
    .arg(m_currentTheme.fontSize);

    m_editor->setStyleSheet(styleSheet);

    // 设置字体
    QFont font(m_currentTheme.fontFamily, m_currentTheme.fontSize);
    m_editor->setFont(font);

    // 设置边距 (使用 document margin，因为 setViewportMargins 是 protected 的)
    qreal avgMargin = (m_currentTheme.marginLeft + m_currentTheme.marginRight +
                       m_currentTheme.marginTop + m_currentTheme.marginBottom) / 4.0;
    m_editor->document()->setDocumentMargin(avgMargin);
}

// 函数说明：实现 EditorThemeManager::userThemesDir 的核心逻辑，供当前模块调用。
QString EditorThemeManager::userThemesDir() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                  + "/editor_themes";
    QDir().mkpath(dir);
    return dir;
}

// ===================== ColorButton =====================

ColorButton::ColorButton(QWidget *parent)
    : QPushButton(parent)
    , m_color(Qt::white)
{
    setFixedSize(40, 25);
    updateButtonColor();
    connect(this, &QPushButton::clicked, this, &ColorButton::chooseColor);
}

// 函数说明：实现 ColorButton::color 的核心逻辑，供当前模块调用。
QColor ColorButton::color() const
{
    return m_color;
}

// 函数说明：设置 ColorButton 的运行参数，并触发必要的界面或数据刷新。
void ColorButton::setColor(const QColor &color)
{
    m_color = color;
    updateButtonColor();
}

// 函数说明：实现 ColorButton::chooseColor 的核心逻辑，供当前模块调用。
void ColorButton::chooseColor()
{
    QColor newColor = QColorDialog::getColor(m_color, this, tr("选择颜色"));
    if (newColor.isValid()) {
        m_color = newColor;
        updateButtonColor();
        emit colorChanged(m_color);
    }
}

// 函数说明：刷新 ColorButton 的内部状态，并同步到相关界面。
void ColorButton::updateButtonColor()
{
    setStyleSheet(QString("background-color: %1; border: 1px solid #999;").arg(m_color.name()));
}

// ===================== EditorThemeEditor =====================

EditorThemeEditor::EditorThemeEditor(EditorThemeManager *manager, QWidget *parent)
    : QDialog(parent)
    , m_manager(manager)
    , m_isModified(false)
{
    setupUi();

    setWindowTitle(tr("编辑器主题设置"));
    resize(700, 600);

    // 加载当前主题
    m_themeCombo->setCurrentText(m_manager->theme(m_manager->currentThemeId()).name);
}

// 函数说明：销毁 EditorThemeEditor 对象，释放本模块持有的资源。
EditorThemeEditor::~EditorThemeEditor()
{
}

// 函数说明：初始化 EditorThemeEditor 的 setupUi 相关界面、动作或服务连接。
void EditorThemeEditor::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 主题选择
    QHBoxLayout *themeLayout = new QHBoxLayout();
    themeLayout->addWidget(new QLabel(tr("主题:"), this));

    m_themeCombo = new QComboBox(this);
    for (const QString &id : m_manager->availableThemes()) {
        EditorTheme t = m_manager->theme(id);
        m_themeCombo->addItem(t.name, id);
    }
    connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &EditorThemeEditor::onThemeSelected);
    themeLayout->addWidget(m_themeCombo, 1);

    QPushButton *newBtn = new QPushButton(tr("新建"), this);
    connect(newBtn, &QPushButton::clicked, this, &EditorThemeEditor::onNewTheme);
    themeLayout->addWidget(newBtn);

    QPushButton *deleteBtn = new QPushButton(tr("删除"), this);
    connect(deleteBtn, &QPushButton::clicked, this, &EditorThemeEditor::onDeleteTheme);
    themeLayout->addWidget(deleteBtn);

    QPushButton *importBtn = new QPushButton(tr("导入"), this);
    connect(importBtn, &QPushButton::clicked, this, &EditorThemeEditor::onImportTheme);
    themeLayout->addWidget(importBtn);

    QPushButton *exportBtn = new QPushButton(tr("导出"), this);
    connect(exportBtn, &QPushButton::clicked, this, &EditorThemeEditor::onExportTheme);
    themeLayout->addWidget(exportBtn);

    mainLayout->addLayout(themeLayout);

    // 选项卡
    QTabWidget *tabWidget = new QTabWidget(this);

    // 基本信息选项卡
    QWidget *infoTab = new QWidget(this);
    QFormLayout *infoLayout = new QFormLayout(infoTab);

    m_nameEdit = new QLineEdit(this);
    infoLayout->addRow(tr("名称:"), m_nameEdit);

    m_authorEdit = new QLineEdit(this);
    infoLayout->addRow(tr("作者:"), m_authorEdit);

    m_descriptionEdit = new QLineEdit(this);
    infoLayout->addRow(tr("描述:"), m_descriptionEdit);

    tabWidget->addTab(infoTab, tr("基本信息"));

    // 颜色选项卡
    QWidget *colorTab = new QWidget(this);
    QVBoxLayout *colorLayout = new QVBoxLayout(colorTab);

    QGroupBox *editorGroup = new QGroupBox(tr("编辑器颜色"), this);
    QFormLayout *editorColorLayout = new QFormLayout(editorGroup);

    m_bgColorBtn = new ColorButton(this);
    connect(m_bgColorBtn, &ColorButton::colorChanged, this, &EditorThemeEditor::onColorChanged);
    editorColorLayout->addRow(tr("背景色:"), m_bgColorBtn);

    m_fgColorBtn = new ColorButton(this);
    connect(m_fgColorBtn, &ColorButton::colorChanged, this, &EditorThemeEditor::onColorChanged);
    editorColorLayout->addRow(tr("文本色:"), m_fgColorBtn);

    m_selectionColorBtn = new ColorButton(this);
    connect(m_selectionColorBtn, &ColorButton::colorChanged, this, &EditorThemeEditor::onColorChanged);
    editorColorLayout->addRow(tr("选中色:"), m_selectionColorBtn);

    m_currentLineColorBtn = new ColorButton(this);
    connect(m_currentLineColorBtn, &ColorButton::colorChanged, this, &EditorThemeEditor::onColorChanged);
    editorColorLayout->addRow(tr("当前行:"), m_currentLineColorBtn);

    m_lineNumberColorBtn = new ColorButton(this);
    connect(m_lineNumberColorBtn, &ColorButton::colorChanged, this, &EditorThemeEditor::onColorChanged);
    editorColorLayout->addRow(tr("行号:"), m_lineNumberColorBtn);

    m_cursorColorBtn = new ColorButton(this);
    connect(m_cursorColorBtn, &ColorButton::colorChanged, this, &EditorThemeEditor::onColorChanged);
    editorColorLayout->addRow(tr("光标:"), m_cursorColorBtn);

    colorLayout->addWidget(editorGroup);

    QGroupBox *syntaxGroup = new QGroupBox(tr("语法高亮"), this);
    QFormLayout *syntaxLayout = new QFormLayout(syntaxGroup);

    m_heading1ColorBtn = new ColorButton(this);
    connect(m_heading1ColorBtn, &ColorButton::colorChanged, this, &EditorThemeEditor::onColorChanged);
    syntaxLayout->addRow(tr("标题 1:"), m_heading1ColorBtn);

    m_heading2ColorBtn = new ColorButton(this);
    connect(m_heading2ColorBtn, &ColorButton::colorChanged, this, &EditorThemeEditor::onColorChanged);
    syntaxLayout->addRow(tr("标题 2:"), m_heading2ColorBtn);

    m_boldColorBtn = new ColorButton(this);
    connect(m_boldColorBtn, &ColorButton::colorChanged, this, &EditorThemeEditor::onColorChanged);
    syntaxLayout->addRow(tr("粗体:"), m_boldColorBtn);

    m_italicColorBtn = new ColorButton(this);
    connect(m_italicColorBtn, &ColorButton::colorChanged, this, &EditorThemeEditor::onColorChanged);
    syntaxLayout->addRow(tr("斜体:"), m_italicColorBtn);

    m_codeColorBtn = new ColorButton(this);
    connect(m_codeColorBtn, &ColorButton::colorChanged, this, &EditorThemeEditor::onColorChanged);
    syntaxLayout->addRow(tr("代码:"), m_codeColorBtn);

    m_linkColorBtn = new ColorButton(this);
    connect(m_linkColorBtn, &ColorButton::colorChanged, this, &EditorThemeEditor::onColorChanged);
    syntaxLayout->addRow(tr("链接:"), m_linkColorBtn);

    m_blockquoteColorBtn = new ColorButton(this);
    connect(m_blockquoteColorBtn, &ColorButton::colorChanged, this, &EditorThemeEditor::onColorChanged);
    syntaxLayout->addRow(tr("引用:"), m_blockquoteColorBtn);

    colorLayout->addWidget(syntaxGroup);

    tabWidget->addTab(colorTab, tr("颜色"));

    // 字体选项卡
    QWidget *fontTab = new QWidget(this);
    QFormLayout *fontLayout = new QFormLayout(fontTab);

    m_fontCombo = new QFontComboBox(this);
    connect(m_fontCombo, &QFontComboBox::currentFontChanged, this, &EditorThemeEditor::onFontChanged);
    fontLayout->addRow(tr("字体:"), m_fontCombo);

    m_fontSizeSpin = new QSpinBox(this);
    m_fontSizeSpin->setRange(8, 48);
    m_fontSizeSpin->setValue(14);
    connect(m_fontSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &EditorThemeEditor::onFontChanged);
    fontLayout->addRow(tr("字号:"), m_fontSizeSpin);

    m_lineSpacingSpin = new QSpinBox(this);
    m_lineSpacingSpin->setRange(100, 200);
    m_lineSpacingSpin->setValue(140);
    m_lineSpacingSpin->setSuffix("%");
    fontLayout->addRow(tr("行距:"), m_lineSpacingSpin);

    tabWidget->addTab(fontTab, tr("字体"));

    mainLayout->addWidget(tabWidget);

    // 预览
    QGroupBox *previewGroup = new QGroupBox(tr("预览"), this);
    QVBoxLayout *previewLayout = new QVBoxLayout(previewGroup);

    m_previewEdit = new QPlainTextEdit(this);
    m_previewEdit->setPlainText(
        "# 标题 1\n"
        "## 标题 2\n"
        "这是**粗体**和*斜体*文本。\n"
        "`行内代码` 和 [链接](https://example.com)\n"
        "> 引用文本\n"
        "```\n代码块\n```"
    );
    m_previewEdit->setReadOnly(true);
    m_previewEdit->setMaximumHeight(150);
    previewLayout->addWidget(m_previewEdit);

    mainLayout->addWidget(previewGroup);

    // 底部按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    QPushButton *applyBtn = new QPushButton(tr("应用"), this);
    connect(applyBtn, &QPushButton::clicked, this, &EditorThemeEditor::onApply);
    btnLayout->addWidget(applyBtn);

    QPushButton *closeBtn = new QPushButton(tr("关闭"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);

    mainLayout->addLayout(btnLayout);

    // 加载第一个主题
    if (m_themeCombo->count() > 0) {
        onThemeSelected(0);
    }
}

// 函数说明：加载 EditorThemeEditor 需要的数据、配置或外部资源。
void EditorThemeEditor::loadThemeToUi(const EditorTheme &theme)
{
    m_currentEditingId = theme.id;

    m_nameEdit->setText(theme.name);
    m_authorEdit->setText(theme.author);
    m_descriptionEdit->setText(theme.description);

    m_bgColorBtn->setColor(theme.background);
    m_fgColorBtn->setColor(theme.foreground);
    m_selectionColorBtn->setColor(theme.selection);
    m_currentLineColorBtn->setColor(theme.currentLine);
    m_lineNumberColorBtn->setColor(theme.lineNumber);
    m_cursorColorBtn->setColor(theme.cursor);

    m_heading1ColorBtn->setColor(theme.syntax.heading1);
    m_heading2ColorBtn->setColor(theme.syntax.heading2);
    m_boldColorBtn->setColor(theme.syntax.bold);
    m_italicColorBtn->setColor(theme.syntax.italic);
    m_codeColorBtn->setColor(theme.syntax.code);
    m_linkColorBtn->setColor(theme.syntax.link);
    m_blockquoteColorBtn->setColor(theme.syntax.blockquote);

    m_fontCombo->setCurrentFont(QFont(theme.fontFamily));
    m_fontSizeSpin->setValue(theme.fontSize);
    m_lineSpacingSpin->setValue(theme.lineSpacing);

    updatePreview();
}

// 函数说明：实现 EditorThemeEditor::uiToTheme 的核心逻辑，供当前模块调用。
EditorTheme EditorThemeEditor::uiToTheme()
{
    EditorTheme theme;

    theme.id = m_currentEditingId;
    theme.name = m_nameEdit->text();
    theme.author = m_authorEdit->text();
    theme.description = m_descriptionEdit->text();

    theme.background = m_bgColorBtn->color();
    theme.foreground = m_fgColorBtn->color();
    theme.selection = m_selectionColorBtn->color();
    theme.currentLine = m_currentLineColorBtn->color();
    theme.lineNumber = m_lineNumberColorBtn->color();
    theme.cursor = m_cursorColorBtn->color();

    theme.syntax.heading1 = m_heading1ColorBtn->color();
    theme.syntax.heading2 = m_heading2ColorBtn->color();
    theme.syntax.bold = m_boldColorBtn->color();
    theme.syntax.italic = m_italicColorBtn->color();
    theme.syntax.code = m_codeColorBtn->color();
    theme.syntax.link = m_linkColorBtn->color();
    theme.syntax.blockquote = m_blockquoteColorBtn->color();

    theme.fontFamily = m_fontCombo->currentFont().family();
    theme.fontSize = m_fontSizeSpin->value();
    theme.lineSpacing = m_lineSpacingSpin->value();

    return theme;
}

// 函数说明：刷新 EditorThemeEditor 的内部状态，并同步到相关界面。
void EditorThemeEditor::updatePreview()
{
    EditorTheme theme = uiToTheme();

    QString styleSheet = QString(R"(
        QPlainTextEdit {
            background-color: %1;
            color: %2;
            selection-background-color: %3;
            font-family: %4;
            font-size: %5px;
        }
    )")
    .arg(theme.background.name())
    .arg(theme.foreground.name())
    .arg(theme.selection.name())
    .arg(theme.fontFamily)
    .arg(theme.fontSize);

    m_previewEdit->setStyleSheet(styleSheet);
}

// 函数说明：响应 EditorThemeEditor 收到的信号或异步回调，并更新界面状态。
void EditorThemeEditor::onThemeSelected(int index)
{
    QString id = m_themeCombo->itemData(index).toString();
    EditorTheme theme = m_manager->theme(id);
    loadThemeToUi(theme);
}

// 函数说明：响应 EditorThemeEditor 收到的信号或异步回调，并更新界面状态。
void EditorThemeEditor::onNewTheme()
{
    bool ok;
    QString name = QInputDialog::getText(this, tr("新建主题"),
        tr("主题名称:"), QLineEdit::Normal, "", &ok);

    if (ok && !name.isEmpty()) {
        EditorTheme theme = uiToTheme();
        theme.id = name.toLower().replace(" ", "-");
        theme.name = name;
        theme.isBuiltin = false;

        m_manager->addTheme(theme);
        m_themeCombo->addItem(name, theme.id);
        m_themeCombo->setCurrentIndex(m_themeCombo->count() - 1);
    }
}

// 函数说明：响应 EditorThemeEditor 收到的信号或异步回调，并更新界面状态。
void EditorThemeEditor::onDeleteTheme()
{
    QString id = m_themeCombo->currentData().toString();
    EditorTheme theme = m_manager->theme(id);

    if (theme.isBuiltin) {
        QMessageBox::warning(this, tr("错误"), tr("不能删除内置主题"));
        return;
    }

    if (QMessageBox::question(this, tr("确认"),
        tr("确定要删除主题 \"%1\" 吗？").arg(theme.name)) == QMessageBox::Yes) {
        m_manager->removeTheme(id);
        m_themeCombo->removeItem(m_themeCombo->currentIndex());
    }
}

// 函数说明：响应 EditorThemeEditor 收到的信号或异步回调，并更新界面状态。
void EditorThemeEditor::onImportTheme()
{
    QString filePath = QFileDialog::getOpenFileName(this,
        tr("导入主题"), QString(),
        tr("JSON 文件 (*.json)"));

    if (!filePath.isEmpty()) {
        if (m_manager->importTheme(filePath)) {
            // 刷新列表
            m_themeCombo->clear();
            for (const QString &id : m_manager->availableThemes()) {
                EditorTheme t = m_manager->theme(id);
                m_themeCombo->addItem(t.name, id);
            }
        } else {
            QMessageBox::warning(this, tr("错误"), tr("导入主题失败"));
        }
    }
}

// 函数说明：响应 EditorThemeEditor 收到的信号或异步回调，并更新界面状态。
void EditorThemeEditor::onExportTheme()
{
    QString id = m_themeCombo->currentData().toString();
    EditorTheme theme = m_manager->theme(id);

    QString filePath = QFileDialog::getSaveFileName(this,
        tr("导出主题"), theme.name + ".json",
        tr("JSON 文件 (*.json)"));

    if (!filePath.isEmpty()) {
        if (!m_manager->exportTheme(id, filePath)) {
            QMessageBox::warning(this, tr("错误"), tr("导出主题失败"));
        }
    }
}

// 函数说明：响应 EditorThemeEditor 收到的信号或异步回调，并更新界面状态。
void EditorThemeEditor::onApply()
{
    EditorTheme theme = uiToTheme();
    m_manager->updateTheme(theme);
    m_manager->applyTheme(theme.id);
}

// 函数说明：响应 EditorThemeEditor 收到的信号或异步回调，并更新界面状态。
void EditorThemeEditor::onColorChanged()
{
    m_isModified = true;
    updatePreview();
}

// 函数说明：响应 EditorThemeEditor 收到的信号或异步回调，并更新界面状态。
void EditorThemeEditor::onFontChanged()
{
    m_isModified = true;
    updatePreview();
}

// 函数说明：响应 EditorThemeEditor 收到的信号或异步回调，并更新界面状态。
void EditorThemeEditor::onPreview()
{
    updatePreview();
}

