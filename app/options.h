// 文件说明：app\options.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef OPTIONS_H
#define OPTIONS_H

#include <QObject>
#include <QFont>
#include <QKeySequence>
#include <QMap>
#include <QSettings>

/**
 * @brief 全局配置管理类
 * 统一管理：编辑器、预览、网络、快捷键、Markdown扩展等所有设置
 * 自动保存/读取配置，修改后发送信号通知界面更新
 */
class Options : public QObject
{
    Q_OBJECT
public:
    //代理模式枚举：无代理 / 系统代理 / 手动代理
    enum ProxyMode { NoProxy, SystemProxy, ManualProxy };
#ifndef ENABLE_HOEDOWN
    //markdown转换器类型
    enum MarkdownConverter { DiscountMarkdownConverter, RevealMarkdownConverter };
#else
    enum MarkdownConverter { DiscountMarkdownConverter, HoedownMarkdownConverter, RevealMarkdownConverter };
#endif
    //显示构造函数
    explicit Options(QObject *parent = nullptr);
    //应用配置
    void apply();

    /* Editor options 编辑器设置 */
    //获取编辑器字体
    QFont editorFont() const;
    //设置编辑器字体
    void setEditorFont(const QFont &font);
    //获取制表符宽度
    int tabWidth() const;
    //设置制表符宽度
    void setTabWidth(int width);
    //是否显示行号列号
    bool isLineColumnEnabled() const;
    void setLineColumnEnabled(bool enabled);

    bool isRulerEnabled() const;
    void setRulerEnabled(bool enabled);
   // 标尺线位置
    int rulerPos() const;
    void setRulerPos(int pos);
     /* ===================== HTML 预览字体设置 ===================== */
    /* HTML preview options */
    QFont standardFont() const;
    void setStandardFont(const QFont &font);

    QFont serifFont() const;
    void setSerifFont(const QFont &font);

    QFont sansSerifFont() const;
    void setSansSerifFont(const QFont &font);

    QFont fixedFont() const;
    void setFixedFont(const QFont &font);
    // 默认字体大小 / 等宽字体大小
    int defaultFontSize() const;
    void setDefaultFontSize(int size);

    int defaultFixedFontSize() const;
    void setDefaultFixedFontSize(int size);

    /* Internet options */
    //网络代理设置
    ProxyMode proxyMode() const;
    void setProxyMode(ProxyMode mode);

    QString proxyHost() const;
    void setProxyHost(const QString &host);

    quint16 proxyPort() const;
    void setProxyPort(quint16 port);

    QString proxyUser() const;
    void setProxyUser(const QString &user);

    QString proxyPassword() const;
    void setProxyPassword(const QString &password);

    /* Shortcuts options */
    //自定义快捷键
    void addCustomShortcut(const QString &actionName, const QKeySequence &keySequence);
    bool hasCustomShortcut(const QString &actionName) const;
    QKeySequence customShortcut(const QString &actionName) const;

    /* Extra menu options */
       /* ===================== Markdown 扩展功能开关 ===================== */
    bool isAutolinkEnabled() const;//自动连接
    void setAutolinkEnabled(bool enabled);

    bool isStrikethroughEnabled() const;//删除线
    void setStrikethroughEnabled(bool enabled);

    bool isAlphabeticListsEnabled() const;//字母列表
    void setAlphabeticListsEnabled(bool enabled);

    bool isDefinitionListsEnabled() const;//定义列表
    void setDefinitionListsEnabled(bool enabled);

    bool isSmartyPantsEnabled() const;//智能标点
    void setSmartyPantsEnabled(bool enabled);

    bool isFootnotesEnabled() const;//脚注
    void setFootnotesEnabled(bool enabled);

    bool isSuperscriptEnabled() const;//上标
    void setSuperscriptEnabled(bool enabled);

    bool isMathSupportEnabled() const;//数学公式
    void setMathSupportEnabled(bool enabled);

    bool isMathInlineSupportEnabled() const;//行内数学公式
    void setMathInlineSupportEnabled(bool enabled);

    bool isCodeHighlightingEnabled() const;//代码高亮
    void setCodeHighlightingEnabled(bool enabled);

    bool isShowSpecialCharactersEnabled() const;//显示特殊字符
    void setShowSpecialCharactersEnabled(bool enabled);

    bool isWordWrapEnabled() const;//自动换行
    void setWordWrapEnabled(bool enabled);

    bool isSourceAtSingleSizeEnabled() const;//源码等宽
    void setSourceAtSingleSizeEnabled(bool enabled);

    bool isSpellingCheckEnabled() const;//拼写检查
    void setSpellingCheckEnabled(bool enabled);

    bool isYamlHeaderSupportEnabled() const;//YAML头部支持
    void setYamlHeaderSupportEnabled(bool enabled);

    bool isDiagramSupportEnabled() const;//图表支持
    void setDiagramSupportEnabled(bool enabled);

    QString dictionaryLanguage() const;//拼写检查字典语言
    void setDictionaryLanguage(const QString &language);

    MarkdownConverter markdownConverter() const; // Markdown 转换器选择
    void setMarkdownConverter(MarkdownConverter converter);
    // 最后使用的主题
    QString lastUsedTheme() const;
    void setLastUsedTheme(const QString &theme);
    // 从配置文件读取所有设置
    void readSettings();
     // 将所有设置写入配置文件
    void writeSettings();

signals:
    /* 配置变化信号 → 界面自动更新 */
    void editorFontChanged(const QFont &font);
    void editorStyleChanged();
    void tabWidthChanged(int tabWidth);
    void lineColumnEnabledChanged(bool enabled);
    void rulerEnabledChanged(bool enabled);
    void rulerPosChanged(int pos);

    void proxyConfigurationChanged();
    void markdownConverterChanged();

private:
    // 兼容旧版本配置迁移
    void migrateLastUsedStyleOption(QSettings &settings);

private:
    QFont font;//编辑器字体
    int m_tabWidth;//制表符宽度
    ProxyMode m_proxyMode;//代理模式
    QString m_proxyHost;
    quint16 m_proxyPort;
    QString m_proxyUser;
    QString m_proxyPassword;
    // Markdown 扩展功能开关
    bool m_autolinkEnabled;
    bool m_strikethroughEnabled;
    bool m_alphabeticListsEnabled;
    bool m_definitionListsEnabled;
    bool m_smartyPantsEnabled;
    bool m_footnotesEnabled;
    bool m_superscriptEnabled;
    bool m_mathSupportEnabled;
    bool m_mathInlineSupportEnabled;
    bool m_codeHighlightingEnabled;
    bool m_showSpecialCharactersEnabled;
    bool m_wordWrapEnabled;
    bool m_sourceAtSingleSizeEnabled;
    bool m_spellingCheckEnabled;
    bool m_yamlHeaderSupportEnabled;
    bool m_diagramSupportEnabled;

    bool m_lineColumnEnabled;//显示行列
    bool m_rulerEnabled;//显示标尺
    int m_rulerPos;//标尺位置
    QString m_dictionaryLanguage;//字典语言
    MarkdownConverter m_markdownConverter;//转换器类型
    QString m_lastUsedTheme;//最后用的主题
    //预览字体
    QString m_standardFontFamily;
    QString m_fixedFontFamily;
    QString m_serifFontFamily;
    QString m_sansSerifFontFamily;
    int m_defaultFontSize;
    int m_defaultFixedFontSize;
    QMap<QString, QKeySequence> m_customShortcuts;//自定义快捷键
};

#endif // OPTIONS_H

