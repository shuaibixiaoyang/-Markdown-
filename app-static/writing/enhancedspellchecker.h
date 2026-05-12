// 文件说明：app-static\writing\enhancedspellchecker.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef ENHANCEDSPELLCHECKER_H
#define ENHANCEDSPELLCHECKER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QSet>
#include <QPlainTextEdit>
#include <QTextCharFormat>

/**
 * @brief 增强拼写检查器
 *
 * 功能：
 * - 多语言支持
 * - 自定义词典
 * - 专业术语词典
 * - 实时拼写检查
 * - 拼写建议
 * - 自动更正
 */
class EnhancedSpellChecker : public QObject
{
    Q_OBJECT

public:
    // 支持的语言
    enum class Language {
        English_US,
        English_UK,
        Chinese_Simplified,
        Chinese_Traditional,
        German,
        French,
        Spanish,
        Japanese,
        Custom
    };
    Q_ENUM(Language)

    // 拼写错误信息
    struct SpellingError {
        int position;           // 位置
        int length;             // 长度
        QString word;           // 错误单词
        QStringList suggestions;// 建议
        bool isIgnored;         // 是否忽略

        SpellingError() : position(0), length(0), isIgnored(false) {}
    };

    // 自动更正规则
    struct AutoCorrectRule {
        QString from;           // 原文
        QString to;             // 更正为
        bool enabled;           // 是否启用
        int useCount;           // 使用次数

        AutoCorrectRule() : enabled(true), useCount(0) {}
    };

    explicit EnhancedSpellChecker(QObject *parent = nullptr);
    ~EnhancedSpellChecker();

    // 设置编辑器
    void setEditor(QPlainTextEdit *editor);

    // 语言设置
    void setLanguage(Language lang);
    void addLanguage(Language lang);
    void removeLanguage(Language lang);
    QList<Language> activeLanguages() const { return m_activeLanguages; }
    bool isLanguageAvailable(Language lang) const;

    // 拼写检查
    bool checkWord(const QString &word);
    QStringList getSuggestions(const QString &word, int maxCount = 5);
    QVector<SpellingError> checkText(const QString &text);
    void checkDocument();

    // 自定义词典
    bool addToUserDictionary(const QString &word);
    bool removeFromUserDictionary(const QString &word);
    QStringList userDictionary() const { return m_userDictionary.values(); }
    bool loadUserDictionary(const QString &filePath);
    bool saveUserDictionary(const QString &filePath);

    // 专业术语词典
    bool loadTechnicalDictionary(const QString &name, const QString &filePath);
    void enableTechnicalDictionary(const QString &name, bool enable);
    QStringList technicalDictionaries() const { return m_technicalDictionaries.keys(); }

    // 忽略列表
    void ignoreWord(const QString &word);
    void ignoreAll(const QString &word);
    void unignoreWord(const QString &word);
    bool isIgnored(const QString &word) const;

    // 自动更正
    void setAutoCorrectEnabled(bool enabled);
    bool isAutoCorrectEnabled() const { return m_autoCorrectEnabled; }
    void addAutoCorrectRule(const QString &from, const QString &to);
    void removeAutoCorrectRule(const QString &from);
    QVector<AutoCorrectRule> autoCorrectRules() const;
    bool loadAutoCorrectRules(const QString &filePath);
    bool saveAutoCorrectRules(const QString &filePath);

    // 配置
    void setRealTimeCheck(bool enabled);
    void setHighlightErrors(bool enabled);
    void setErrorHighlightColor(const QColor &color);
    void setMinWordLength(int length);

    // 统计
    int errorCount() const { return m_currentErrors.size(); }
    int checkedWordCount() const { return m_checkedWordCount; }

signals:
    void spellingErrorsFound(const QVector<SpellingError> &errors);
    void wordCorrected(const QString &from, const QString &to);
    void dictionaryUpdated();
    void checkingStarted();
    void checkingFinished();

public slots:
    void recheckDocument();
    void clearErrors();

private slots:
    void onTextChanged();
    void performCheck();

private:
    void setupDefaultAutoCorrect();
    bool isValidWord(const QString &word);
    QString normalizeWord(const QString &word) const;
    void highlightErrors();
    void clearHighlights();
    QString autoCorrect(const QString &word);
    int levenshteinDistance(const QString &s1, const QString &s2);
    QStringList findSimilarWords(const QString &word, int maxDistance = 2);

    QPlainTextEdit *m_editor;
    QList<Language> m_activeLanguages;

    // 词典
    QSet<QString> m_dictionary;             // 主词典
    QSet<QString> m_userDictionary;         // 用户词典
    QMap<QString, QSet<QString>> m_technicalDictionaries;  // 专业词典
    QMap<QString, bool> m_technicalDictionaryEnabled;      // 启用状态
    QSet<QString> m_ignoredWords;           // 忽略的单词

    // 自动更正
    QMap<QString, AutoCorrectRule> m_autoCorrectRules;
    bool m_autoCorrectEnabled;

    // 当前错误
    QVector<SpellingError> m_currentErrors;
    QList<QTextEdit::ExtraSelection> m_errorSelections;

    // 配置
    bool m_realTimeCheck;
    bool m_highlightErrors;
    QColor m_errorColor;
    int m_minWordLength;
    int m_checkedWordCount;

    // 延迟检查定时器
    QTimer *m_checkTimer;
};

#endif // ENHANCEDSPELLCHECKER_H

