// 文件说明：app\options.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "options.h"

// Qt WebEngine 不再提供 QWebSettings 类
// 字体设置需要通过 CSS 方式实现
// #include <QWebSettings>

/**********************************************************************
 * 配置项常量定义（存储在 QSettings 中的键值路径）
 * 采用 "分组/项名" 格式，便于配置文件结构化管理
 *********************************************************************/

// 通用配置
static const char* MARKDOWN_CONVERTER = "General/converter";           // Markdown 转换器类型
static const char* LAST_USED_THEME = "General/lastusedtheme";         // 最后使用的主题
static const char* THEME_DEFAULT = "Default";                         // 默认主题名称

// 编辑器字体默认值
static const char* FONT_FAMILY_DEFAULT = "Monospace";                 // 编辑器默认等宽字体

// 编辑器配置
static const char* FONT_FAMILY = "editor/font/family";                // 编辑器字体
static const char* FONT_SIZE = "editor/font/size";                    // 编辑器字号
static const char* TAB_WIDTH = "editor/tabwidth";                     // 制表符宽度
static const char* LINECOLUMN_ENABLED = "editor/linecolumn/enabled"; // 显示行号列号
static const char* RULER_ENABLED = "editor/ruler/enabled";           // 启用右侧参考线
static const char* RULER_POS = "editor/ruler/pos";                   // 参考线位置

// 预览窗口字体配置
static const char* PREVIEW_STANDARD_FONT = "preview/standardfont";    // 预览标准字体
static const char* PREVIEW_FIXED_FONT = "preview/fixedfont";          // 预览等宽字体
static const char* PREVIEW_SERIF_FONT = "preview/seriffont";          // 预览衬线字体
static const char* PREVIEW_SANSSERIF_FONT = "preview/sansseriffont";// 预览无衬线字体
static const char* PREVIEW_DEFAULT_FONT_SIZE = "preview/defaultfontsize";       // 预览默认字号
static const char* PREVIEW_DEFAULT_FIXED_FONT_SIZE = "preview/defaultfixedfontsize";// 预览默认等宽字号

// 网络代理配置
static const char* PROXY_MODE = "internet/proxy/mode";                // 代理模式
static const char* PROXY_HOST = "internet/proxy/host";                // 代理主机
static const char* PROXY_PORT = "internet/proxy/port";                // 代理端口
static const char* PROXY_USER = "internet/proxy/user";                // 代理用户名
static const char* PROXY_PASSWORD = "internet/proxy/password";        // 代理密码

// Markdown 扩展功能开关
static const char* AUTOLINK_ENABLED = "extensions/autolink";         // 自动链接
static const char* STRIKETHROUGH_ENABLED = "extensions/strikethrough";// 删除线
static const char* ALPHABETICLISTS_ENABLED = "extensions/alphabeticLists";// 字母列表
static const char* DEFINITIONSLISTS_ENABLED = "extensions/definitionLists";// 定义列表
static const char* SMARTYPANTS_ENABLED = "extensions/smartyPants";     // 智能标点
static const char* FOOTNOTES_ENABLED = "extensions/footnotes";       // 脚注
static const char* SUPERSCRIPT_ENABLED = "extensions/superscript";   // 上标

// 高级功能开关
static const char* MATHSUPPORT_ENABLED = "mathsupport/enabled";      // 数学公式支持
static const char* MATHINLINESUPPORT_ENABLED = "mathinlinesupport/enabled";// 行内公式
static const char* CODEHIGHLIGHT_ENABLED = "codehighlighting/enabled";// 代码高亮
static const char* SHOWSPECIALCHARACTERS_ENABLED = "specialchars/enabled";// 显示特殊字符
static const char* WORDWRAP_ENABLED = "wordwrap/enabled";           // 自动换行
static const char* SOURCEATSINGLESIZE_ENABLED = "sourceatsinglesize/enabled";// 源码等宽显示
static const char* SPELLINGCHECK_ENABLED = "spelling/enabled";      // 拼写检查
static const char* DICTIONARY_LANGUAGE = "spelling/language";      // 拼写检查语言
static const char* YAMLHEADERSUPPORT_ENABLED = "yamlheadersupport/enabled";// YAML 头支持
static const char* DIAGRAMSUPPORT_ENABLED = "diagramsupport/enabled";// 图表支持

// 已废弃配置项（用于兼容旧版本）
static const char* DEPRECATED__LAST_USED_STYLE = "general/lastusedstyle";

/**********************************************************************
 * 构造函数：初始化所有配置项的默认值
 *********************************************************************/
// 函数说明：构造 Options 对象，初始化本模块需要的状态、界面和资源。
Options::Options(QObject *parent) :
    QObject(parent),
    m_tabWidth(8),                                    // 默认制表符宽度 8
    m_proxyMode(NoProxy),                             // 默认无代理
    m_proxyPort(0),                                   // 默认代理端口 0
    m_autolinkEnabled(true),                          // 默认启用自动链接
    m_strikethroughEnabled(true),                     // 默认启用删除线
    m_alphabeticListsEnabled(true),                   // 默认启用字母列表
    m_definitionListsEnabled(true),                   // 默认启用定义列表
    m_smartyPantsEnabled(true),                       // 默认启用智能标点
    m_footnotesEnabled(true),                         // 默认启用脚注
    m_superscriptEnabled(true),                       // 默认启用上标
    m_mathSupportEnabled(false),                      // 默认关闭数学公式
    m_mathInlineSupportEnabled(false),                // 默认关闭行内公式
    m_codeHighlightingEnabled(false),                 // 默认关闭代码高亮
    m_showSpecialCharactersEnabled(false),            // 默认关闭特殊字符显示
    m_wordWrapEnabled(true),                          // 默认启用自动换行
    m_sourceAtSingleSizeEnabled(true),                // 默认启用源码等宽
    m_spellingCheckEnabled(true),                     // 默认启用拼写检查
    m_diagramSupportEnabled(false),                   // 默认关闭图表支持
    m_lineColumnEnabled(true),                        // 默认显示行列号
    m_rulerEnabled(false),                            // 默认关闭参考线
    m_rulerPos(80),                                   // 默认参考线位置 80
    m_markdownConverter(DiscountMarkdownConverter),   // 默认使用 Discount 转换器
    m_lastUsedTheme(THEME_DEFAULT)                    // 默认主题
{
}

/**********************************************************************
 * 应用配置：将配置生效并发送信号通知界面更新
 *********************************************************************/
// 函数说明：应用 Options 当前配置，让编辑器或预览立即生效。
void Options::apply()
{
    // Qt WebEngine 不支持 QWebSettings，字体设置需通过 CSS 注入实现
    // TODO: 使用 CSS 注入实现预览字体设置

    emit proxyConfigurationChanged();                 // 代理配置变更信号
    emit markdownConverterChanged();                  // Markdown 转换器变更信号
    emit lineColumnEnabledChanged(m_lineColumnEnabled); // 行列号显示信号
    emit rulerEnabledChanged(m_rulerEnabled);         // 参考线显示信号
    emit rulerPosChanged(m_rulerPos);                // 参考线位置信号
}

/**********************************************************************
 * 编辑器字体设置与获取
 *********************************************************************/
// 函数说明：处理编辑命令，将用户操作转换为 Markdown 文本变更。
QFont Options::editorFont() const
{
    return font;
}

// 函数说明：设置 Options 的运行参数，并触发必要的界面或数据刷新。
void Options::setEditorFont(const QFont &font)
{
    this->font = font;
    emit editorFontChanged(font);                    // 字体变更信号
}

/**********************************************************************
 * 编辑器制表符宽度设置与获取
 *********************************************************************/
// 函数说明：实现 Options::tabWidth 的核心逻辑，供当前模块调用。
int Options::tabWidth() const
{
    return m_tabWidth;
}

// 函数说明：设置 Options 的运行参数，并触发必要的界面或数据刷新。
void Options::setTabWidth(int width)
{
    m_tabWidth = width;
    emit tabWidthChanged(width);
}

/**********************************************************************
 * 行列号显示设置与获取
 *********************************************************************/
// 函数说明：判断 Options 当前是否满足指定状态。
bool Options::isLineColumnEnabled() const
{
    return m_lineColumnEnabled;
}

// 函数说明：设置 Options 的运行参数，并触发必要的界面或数据刷新。
void Options::setLineColumnEnabled(bool enabled)
{
    m_lineColumnEnabled = enabled;
    emit lineColumnEnabledChanged(enabled);
}

/**********************************************************************
 * 编辑器参考线显示开关设置与获取
 *********************************************************************/
// 函数说明：判断 Options 当前是否满足指定状态。
bool Options::isRulerEnabled() const
{
    return m_rulerEnabled;
}

// 函数说明：设置 Options 的运行参数，并触发必要的界面或数据刷新。
void Options::setRulerEnabled(bool enabled)
{
    m_rulerEnabled = enabled;
    emit rulerEnabledChanged(enabled);
}

/**********************************************************************
 * 编辑器参考线位置设置与获取
 *********************************************************************/
// 函数说明：实现 Options::rulerPos 的核心逻辑，供当前模块调用。
int Options::rulerPos() const
{
    return m_rulerPos;
}

// 函数说明：设置 Options 的运行参数，并触发必要的界面或数据刷新。
void Options::setRulerPos(int pos)
{
    m_rulerPos = pos;
    emit rulerPosChanged(pos);
}

/**********************************************************************
 * 预览窗口标准字体设置与获取
 *********************************************************************/
// 函数说明：实现 Options::standardFont 的核心逻辑，供当前模块调用。
QFont Options::standardFont() const
{
    return QFont(m_standardFontFamily);
}

// 函数说明：设置 Options 的运行参数，并触发必要的界面或数据刷新。
void Options::setStandardFont(const QFont &font)
{
    m_standardFontFamily = font.family();
}

/**********************************************************************
 * 预览窗口衬线字体设置与获取
 *********************************************************************/
// 函数说明：实现 Options::serifFont 的核心逻辑，供当前模块调用。
QFont Options::serifFont() const
{
    return QFont(m_serifFontFamily);
}

// 函数说明：设置 Options 的运行参数，并触发必要的界面或数据刷新。
void Options::setSerifFont(const QFont &font)
{
    m_serifFontFamily = font.family();
}

/**********************************************************************
 * 预览窗口无衬线字体设置与获取
 *********************************************************************/
// 函数说明：实现 Options::sansSerifFont 的核心逻辑，供当前模块调用。
QFont Options::sansSerifFont() const
{
    return QFont(m_sansSerifFontFamily);
}

// 函数说明：设置 Options 的运行参数，并触发必要的界面或数据刷新。
void Options::setSansSerifFont(const QFont &font)
{
    m_sansSerifFontFamily = font.family();
}

/**********************************************************************
 * 预览窗口等宽字体设置与获取
 *********************************************************************/
// 函数说明：实现 Options::fixedFont 的核心逻辑，供当前模块调用。
QFont Options::fixedFont() const
{
    return QFont(m_fixedFontFamily);
}

// 函数说明：设置 Options 的运行参数，并触发必要的界面或数据刷新。
void Options::setFixedFont(const QFont &font)
{
    m_fixedFontFamily = font.family();
}

/**********************************************************************
 * 预览窗口默认字号设置与获取
 *********************************************************************/
// 函数说明：实现 Options::defaultFontSize 的核心逻辑，供当前模块调用。
int Options::defaultFontSize() const
{
    return m_defaultFontSize;
}

// 函数说明：设置 Options 的运行参数，并触发必要的界面或数据刷新。
void Options::setDefaultFontSize(int size)
{
    m_defaultFontSize = size;
}

/**********************************************************************
 * 预览窗口默认等宽字号设置与获取
 *********************************************************************/
// 函数说明：实现 Options::defaultFixedFontSize 的核心逻辑，供当前模块调用。
int Options::defaultFixedFontSize() const
{
    return m_defaultFixedFontSize;
}

// 函数说明：设置 Options 的运行参数，并触发必要的界面或数据刷新。
void Options::setDefaultFixedFontSize(int size)
{
    m_defaultFixedFontSize = size;
}

/**********************************************************************
 * 代理模式设置与获取
 *********************************************************************/
// 函数说明：实现 Options::proxyMode 的核心逻辑，供当前模块调用。
Options::ProxyMode Options::proxyMode() const
{
    return m_proxyMode;
}

// 函数说明：设置 Options 的运行参数，并触发必要的界面或数据刷新。
void Options::setProxyMode(Options::ProxyMode mode)
{
    m_proxyMode = mode;
}

/**********************************************************************
 * 代理主机设置与获取
 *********************************************************************/
// 函数说明：实现 Options::proxyHost 的核心逻辑，供当前模块调用。
QString Options::proxyHost() const
{
    return m_proxyHost;
}

// 函数说明：设置 Options 的运行参数，并触发必要的界面或数据刷新。
void Options::setProxyHost(const QString &host)
{
    m_proxyHost = host;
}

/**********************************************************************
 * 代理端口设置与获取
 *********************************************************************/
// 函数说明：实现 Options::proxyPort 的核心逻辑，供当前模块调用。
quint16 Options::proxyPort() const
{
    return m_proxyPort;
}

// 函数说明：设置 Options 的运行参数，并触发必要的界面或数据刷新。
void Options::setProxyPort(quint16 port)
{
    m_proxyPort = port;
}

/**********************************************************************
 * 代理用户名设置与获取
 *********************************************************************/
// 函数说明：实现 Options::proxyUser 的核心逻辑，供当前模块调用。
QString Options::proxyUser() const
{
    return m_proxyUser;
}

// 函数说明：设置 Options 的运行参数，并触发必要的界面或数据刷新。
void Options::setProxyUser(const QString &user)
{
    m_proxyUser = user;
}

/**********************************************************************
 * 代理密码设置与获取
 *********************************************************************/
// 函数说明：实现 Options::proxyPassword 的核心逻辑，供当前模块调用。
QString Options::proxyPassword() const
{
    return m_proxyPassword;
}

// 函数说明：设置 Options 的运行参数，并触发必要的界面或数据刷新。
void Options::setProxyPassword(const QString &password)
{
    m_proxyPassword = password;
}

/**********************************************************************
 * 自定义快捷键管理：添加快捷键
 *********************************************************************/
// 函数说明：向 Options 管理的数据集合中添加一项内容。
void Options::addCustomShortcut(const QString &actionName, const QKeySequence &keySequence)
{
    if (actionName.isEmpty()) return;
    m_customShortcuts.insert(actionName, keySequence);
}

/**********************************************************************
 * 自定义快捷键管理：判断是否存在自定义快捷键
 *********************************************************************/
// 函数说明：检查 Options 是否具备对应的数据或能力。
bool Options::hasCustomShortcut(const QString &actionName) const
{
    return m_customShortcuts.contains(actionName);
}

/**********************************************************************
 * 自定义快捷键管理：获取自定义快捷键
 *********************************************************************/
// 函数说明：实现 Options::customShortcut 的核心逻辑，供当前模块调用。
QKeySequence Options::customShortcut(const QString &actionName) const
{
    return m_customShortcuts.value(actionName);
}

/**********************************************************************
 * 自动链接功能开关
 *********************************************************************/
// 函数说明：判断 Options 当前是否满足指定状态。
bool Options::isAutolinkEnabled() const
{
    return m_autolinkEnabled;
}

// 函数说明：设置 Options 的运行参数，并触发必要的界面或数据刷新。
void Options::setAutolinkEnabled(bool enabled)
{
    m_autolinkEnabled = enabled;
}

/**********************************************************************
 * 删除线功能开关
 *********************************************************************/
// 函数说明：判断 Options 当前是否满足指定状态。
bool Options::isStrikethroughEnabled() const
{
    return m_strikethroughEnabled;
}

// 函数说明：设置 Options 的运行参数，并触发必要的界面或数据刷新。
void Options::setStrikethroughEnabled(bool enabled)
{
    m_strikethroughEnabled = enabled;
}

/**********************************************************************
 * 字母列表功能开关
 *********************************************************************/
// 函数说明：判断 Options 当前是否满足指定状态。
bool Options::isAlphabeticListsEnabled() const
{
    return m_alphabeticListsEnabled;
}

// 函数说明：设置 Options 的运行参数，并触发必要的界面或数据刷新。
void Options::setAlphabeticListsEnabled(bool enabled)
{
    m_alphabeticListsEnabled = enabled;
}

/**********************************************************************
 * 定义列表功能开关
 *********************************************************************/
// 函数说明：判断 Options 当前是否满足指定状态。
bool Options::isDefinitionListsEnabled() const
{
    return m_definitionListsEnabled;
}

// 函数说明：设置 Options 的运行参数，并触发必要的界面或数据刷新。
void Options::setDefinitionListsEnabled(bool enabled)
{
    m_definitionListsEnabled = enabled;
}

/**********************************************************************
 * 智能标点功能开关
 *********************************************************************/
// 函数说明：判断 Options 当前是否满足指定状态。
bool Options::isSmartyPantsEnabled() const
{
    return m_smartyPantsEnabled;
}

// 函数说明：设置 Options 的运行参数，并触发必要的界面或数据刷新。
void Options::setSmartyPantsEnabled(bool enabled)
{
    m_smartyPantsEnabled = enabled;
}

/**********************************************************************
 * 脚注功能开关
 *********************************************************************/
// 函数说明：判断 Options 当前是否满足指定状态。
bool Options::isFootnotesEnabled() const
{
    return m_footnotesEnabled;
}

// 函数说明：设置 Options 的运行参数，并触发必要的界面或数据刷新。
void Options::setFootnotesEnabled(bool enabled)
{
    m_footnotesEnabled = enabled;
}

/**********************************************************************
 * 上标功能开关
 *********************************************************************/
// 函数说明：判断 Options 当前是否满足指定状态。
bool Options::isSuperscriptEnabled() const
{
    return m_superscriptEnabled;
}

// 函数说明：设置 Options 的运行参数，并触发必要的界面或数据刷新。
void Options::setSuperscriptEnabled(bool enabled)
{
    m_superscriptEnabled = enabled;
}

/**********************************************************************
 * 数学公式支持开关
 *********************************************************************/
// 函数说明：判断 Options 当前是否满足指定状态。
bool Options::isMathSupportEnabled() const
{
    return m_mathSupportEnabled;
}

// 函数说明：设置 Options 的运行参数，并触发必要的界面或数据刷新。
void Options::setMathSupportEnabled(bool enabled)
{
    m_mathSupportEnabled = enabled;
}

/**********************************************************************
 * 行内公式支持开关
 *********************************************************************/
// 函数说明：判断 Options 当前是否满足指定状态。
bool Options::isMathInlineSupportEnabled() const
{
    return m_mathInlineSupportEnabled;
}

// 函数说明：设置 Options 的运行参数，并触发必要的界面或数据刷新。
void Options::setMathInlineSupportEnabled(bool enabled)
{
    m_mathInlineSupportEnabled = enabled;
}

/**********************************************************************
 * 代码高亮功能开关
 *********************************************************************/
// 函数说明：判断 Options 当前是否满足指定状态。
bool Options::isCodeHighlightingEnabled() const
{
    return m_codeHighlightingEnabled;
}

// 函数说明：设置 Options 的运行参数，并触发必要的界面或数据刷新。
void Options::setCodeHighlightingEnabled(bool enabled)
{
    m_codeHighlightingEnabled = enabled;
}

/**********************************************************************
 * 显示特殊字符功能开关
 *********************************************************************/
// 函数说明：判断 Options 当前是否满足指定状态。
bool Options::isShowSpecialCharactersEnabled() const
{
    return m_showSpecialCharactersEnabled;
}

// 函数说明：设置 Options 的运行参数，并触发必要的界面或数据刷新。
void Options::setShowSpecialCharactersEnabled(bool enabled)
{
    m_showSpecialCharactersEnabled = enabled;
}

/**********************************************************************
 * 自动换行功能开关
 *********************************************************************/
// 函数说明：判断 Options 当前是否满足指定状态。
bool Options::isWordWrapEnabled() const
{
    return m_wordWrapEnabled;
}

// 函数说明：设置 Options 的运行参数，并触发必要的界面或数据刷新。
void Options::setWordWrapEnabled(bool enabled)
{
    m_wordWrapEnabled = enabled;
}

/**********************************************************************
 * 源码等宽显示开关
 *********************************************************************/
// 函数说明：判断 Options 当前是否满足指定状态。
bool Options::isSourceAtSingleSizeEnabled() const
{
    return m_sourceAtSingleSizeEnabled;
}

// 函数说明：设置 Options 的运行参数，并触发必要的界面或数据刷新。
void Options::setSourceAtSingleSizeEnabled(bool enabled)
{
    m_sourceAtSingleSizeEnabled = enabled;
    emit editorStyleChanged();
}

/**********************************************************************
 * 拼写检查功能开关
 *********************************************************************/
// 函数说明：判断 Options 当前是否满足指定状态。
bool Options::isSpellingCheckEnabled() const
{
    return m_spellingCheckEnabled;
}

// 函数说明：设置 Options 的运行参数，并触发必要的界面或数据刷新。
void Options::setSpellingCheckEnabled(bool enabled)
{
    m_spellingCheckEnabled = enabled;
}

/**********************************************************************
 * YAML 头支持开关
 *********************************************************************/
// 函数说明：判断 Options 当前是否满足指定状态。
bool Options::isYamlHeaderSupportEnabled() const
{
    return m_yamlHeaderSupportEnabled;
}

// 函数说明：设置 Options 的运行参数，并触发必要的界面或数据刷新。
void Options::setYamlHeaderSupportEnabled(bool enabled)
{
    m_yamlHeaderSupportEnabled = enabled;
}

/**********************************************************************
 * 图表支持开关
 *********************************************************************/
// 函数说明：判断 Options 当前是否满足指定状态。
bool Options::isDiagramSupportEnabled() const
{
    return m_diagramSupportEnabled;
}

// 函数说明：设置 Options 的运行参数，并触发必要的界面或数据刷新。
void Options::setDiagramSupportEnabled(bool enabled)
{
    m_diagramSupportEnabled = enabled;
}

/**********************************************************************
 * 拼写检查语言设置与获取
 *********************************************************************/
// 函数说明：实现 Options::dictionaryLanguage 的核心逻辑，供当前模块调用。
QString Options::dictionaryLanguage() const
{
    return m_dictionaryLanguage;
}

// 函数说明：设置 Options 的运行参数，并触发必要的界面或数据刷新。
void Options::setDictionaryLanguage(const QString &language)
{
    m_dictionaryLanguage = language;
}

/**********************************************************************
 * Markdown 转换器设置与获取
 *********************************************************************/
// 函数说明：实现 Options::markdownConverter 的核心逻辑，供当前模块调用。
Options::MarkdownConverter Options::markdownConverter() const
{
    return m_markdownConverter;
}

// 函数说明：设置 Options 的运行参数，并触发必要的界面或数据刷新。
void Options::setMarkdownConverter(Options::MarkdownConverter converter)
{
    if (m_markdownConverter != converter) {
        m_markdownConverter = converter;
        emit markdownConverterChanged();
    }
}

/**********************************************************************
 * 最后使用的主题设置与获取
 *********************************************************************/
// 函数说明：实现 Options::lastUsedTheme 的核心逻辑，供当前模块调用。
QString Options::lastUsedTheme() const
{
    return m_lastUsedTheme;
}

// 函数说明：设置 Options 的运行参数，并触发必要的界面或数据刷新。
void Options::setLastUsedTheme(const QString &theme)
{
    m_lastUsedTheme = theme;
}

/**********************************************************************
 * 从配置文件读取所有设置
 *********************************************************************/
// 函数说明：读取 Options 的配置或数据，并同步到运行时状态。
void Options::readSettings()
{
    QSettings settings;

    // 通用设置读取
    m_markdownConverter = (Options::MarkdownConverter)settings.value(MARKDOWN_CONVERTER, 0).toInt();
    m_lastUsedTheme = settings.value(LAST_USED_THEME, THEME_DEFAULT).toString();

    // 编辑器设置读取
    QString fontFamily = settings.value(FONT_FAMILY, FONT_FAMILY_DEFAULT).toString();
    int fontSize = settings.value(FONT_SIZE, 10).toInt();

    m_tabWidth = settings.value(TAB_WIDTH, 8).toInt();
    m_lineColumnEnabled = settings.value(LINECOLUMN_ENABLED, false).toBool();
    m_rulerEnabled = settings.value(RULER_ENABLED, false).toBool();
    m_rulerPos = settings.value(RULER_POS, 80).toInt();

    // 创建等宽编辑器字体
    QFont f(fontFamily, fontSize);
    f.setStyleHint(QFont::TypeWriter);
    setEditorFont(f);

    // HTML 预览设置（使用默认值，WebEngine 不支持 QWebSettings）
    m_standardFontFamily = settings.value(PREVIEW_STANDARD_FONT, "Arial").toString();
    m_fixedFontFamily = settings.value(PREVIEW_FIXED_FONT, "Courier New").toString();
    m_serifFontFamily = settings.value(PREVIEW_SERIF_FONT, "Times New Roman").toString();
    m_sansSerifFontFamily = settings.value(PREVIEW_SANSSERIF_FONT, "Arial").toString();
    m_defaultFontSize = settings.value(PREVIEW_DEFAULT_FONT_SIZE, 16).toInt();
    m_defaultFixedFontSize = settings.value(PREVIEW_DEFAULT_FIXED_FONT_SIZE, 13).toInt();
    m_mathInlineSupportEnabled = settings.value(MATHINLINESUPPORT_ENABLED, false).toBool();

    // 代理设置读取
    m_proxyMode = (Options::ProxyMode)settings.value(PROXY_MODE, 0).toInt();
    m_proxyHost = settings.value(PROXY_HOST, "").toString();
    m_proxyPort = settings.value(PROXY_PORT, 0).toInt();
    m_proxyUser = settings.value(PROXY_USER, "").toString();
    m_proxyPassword = settings.value(PROXY_PASSWORD, "").toString();

    // 快捷键设置读取
    settings.beginGroup("shortcuts");
    for (const QString &actionName : settings.childKeys()) {
        QKeySequence keySequence = settings.value(actionName, "").value<QKeySequence>();
        addCustomShortcut(actionName, keySequence);
    }
    settings.endGroup();

    // 扩展功能开关读取
    m_autolinkEnabled = settings.value(AUTOLINK_ENABLED, true).toBool();
    m_strikethroughEnabled = settings.value(STRIKETHROUGH_ENABLED, true).toBool();
    m_alphabeticListsEnabled = settings.value(ALPHABETICLISTS_ENABLED, true).toBool();
    m_definitionListsEnabled = settings.value(DEFINITIONSLISTS_ENABLED, true).toBool();
    m_smartyPantsEnabled = settings.value(SMARTYPANTS_ENABLED, true).toBool();
    m_footnotesEnabled = settings.value(FOOTNOTES_ENABLED, true).toBool();
    m_superscriptEnabled = settings.value(SUPERSCRIPT_ENABLED, true).toBool();

    m_mathSupportEnabled = settings.value(MATHSUPPORT_ENABLED, false).toBool();
    m_codeHighlightingEnabled = settings.value(CODEHIGHLIGHT_ENABLED, false).toBool();
    m_showSpecialCharactersEnabled = settings.value(SHOWSPECIALCHARACTERS_ENABLED, false).toBool();
    m_wordWrapEnabled = settings.value(WORDWRAP_ENABLED, true).toBool();
    m_sourceAtSingleSizeEnabled = settings.value(SOURCEATSINGLESIZE_ENABLED, true).toBool();
    m_yamlHeaderSupportEnabled = settings.value(YAMLHEADERSUPPORT_ENABLED, false).toBool();
    m_diagramSupportEnabled = settings.value(DIAGRAMSUPPORT_ENABLED, false).toBool();

    // 拼写检查设置
    m_spellingCheckEnabled = settings.value(SPELLINGCHECK_ENABLED, true).toBool();
    m_dictionaryLanguage = settings.value(DICTIONARY_LANGUAGE, "en_US").toString();

    // 兼容旧版本：迁移已废弃的主题配置项
    if (settings.contains(DEPRECATED__LAST_USED_STYLE)) {
        migrateLastUsedStyleOption(settings);
    }

    // 应用所有读取的配置
    apply();
}

/**********************************************************************
 * 将所有设置写入配置文件
 *********************************************************************/
// 函数说明：写入 Options 的配置或数据，用于下次启动恢复。
void Options::writeSettings()
{
    QSettings settings;

    // 通用设置写入
    settings.setValue(MARKDOWN_CONVERTER, m_markdownConverter);
    settings.setValue(LAST_USED_THEME, m_lastUsedTheme);

    // 编辑器设置写入
    settings.setValue(FONT_FAMILY, font.family());
    settings.setValue(FONT_SIZE, font.pointSize());
    settings.setValue(TAB_WIDTH, m_tabWidth);
    settings.setValue(LINECOLUMN_ENABLED, m_lineColumnEnabled);
    settings.setValue(RULER_ENABLED, m_rulerEnabled);
    settings.setValue(RULER_POS, m_rulerPos);

    // 预览设置写入
    settings.setValue(PREVIEW_STANDARD_FONT, m_standardFontFamily);
    settings.setValue(PREVIEW_FIXED_FONT, m_fixedFontFamily);
    settings.setValue(PREVIEW_SERIF_FONT, m_serifFontFamily);
    settings.setValue(PREVIEW_SANSSERIF_FONT, m_sansSerifFontFamily);
    settings.setValue(PREVIEW_DEFAULT_FONT_SIZE, m_defaultFontSize);
    settings.setValue(PREVIEW_DEFAULT_FIXED_FONT_SIZE, m_defaultFixedFontSize);
    settings.setValue(MATHINLINESUPPORT_ENABLED, m_mathInlineSupportEnabled);

    // 代理设置写入
    settings.setValue(PROXY_MODE, m_proxyMode);
    settings.setValue(PROXY_HOST, m_proxyHost);
    settings.setValue(PROXY_PORT, m_proxyPort);
    settings.setValue(PROXY_USER, m_proxyUser);
    settings.setValue(PROXY_PASSWORD, m_proxyPassword);

    // 快捷键设置写入
    settings.beginGroup("shortcuts");
    QMap<QString, QKeySequence>::const_iterator it = m_customShortcuts.constBegin();
    while (it != m_customShortcuts.constEnd()) {
        settings.setValue(it.key(), it.value());
        ++it;
    }
    settings.endGroup();

    // 扩展功能写入
    settings.setValue(AUTOLINK_ENABLED, m_autolinkEnabled);
    settings.setValue(STRIKETHROUGH_ENABLED, m_strikethroughEnabled);
    settings.setValue(ALPHABETICLISTS_ENABLED, m_alphabeticListsEnabled);
    settings.setValue(DEFINITIONSLISTS_ENABLED, m_definitionListsEnabled);
    settings.setValue(SMARTYPANTS_ENABLED, m_smartyPantsEnabled);
    settings.setValue(FOOTNOTES_ENABLED, m_footnotesEnabled);
    settings.setValue(SUPERSCRIPT_ENABLED, m_superscriptEnabled);

    settings.setValue(MATHSUPPORT_ENABLED, m_mathSupportEnabled);
    settings.setValue(CODEHIGHLIGHT_ENABLED, m_codeHighlightingEnabled);
    settings.setValue(SHOWSPECIALCHARACTERS_ENABLED, m_showSpecialCharactersEnabled);
    settings.setValue(WORDWRAP_ENABLED, m_wordWrapEnabled);
    settings.setValue(SOURCEATSINGLESIZE_ENABLED, m_sourceAtSingleSizeEnabled);
    settings.setValue(YAMLHEADERSUPPORT_ENABLED, m_yamlHeaderSupportEnabled);
    settings.setValue(DIAGRAMSUPPORT_ENABLED, m_diagramSupportEnabled);

    // 拼写检查写入
    settings.setValue(SPELLINGCHECK_ENABLED, m_spellingCheckEnabled);
    settings.setValue(DICTIONARY_LANGUAGE, m_dictionaryLanguage);
}

/**********************************************************************
 * 兼容旧版本：迁移废弃的主题配置项
 * 将旧版 actionXXX 格式映射为新版主题名称
 *********************************************************************/
// 函数说明：实现 Options::migrateLastUsedStyleOption 的核心逻辑，供当前模块调用。
void Options::migrateLastUsedStyleOption(QSettings &settings)
{
    // 旧版样式名称 -> 新版主题名称映射表
    static const QMap<QString, QString> migrations {
        { "actionDefault", "Default" },
        { "actionGithub", "Github" },
        { "actionSolarizedLight", "Solarized Light" },
        { "actionSolarizedDark", "Solarized Dark" },
        { "actionClearness", "Clearness" },
        { "actionClearnessDark", "Clearness Dark" },
        { "actionBywordDark", "Byword Dark" }
    };

    // 读取旧配置并映射为新主题
    QString lastUsedStyle = settings.value(DEPRECATED__LAST_USED_STYLE).toString();
    m_lastUsedTheme = migrations[lastUsedStyle];

    // 删除旧配置项
    settings.remove(DEPRECATED__LAST_USED_STYLE);
}

