// 文件说明：app-static\writing\enhancedspellchecker.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "enhancedspellchecker.h"

#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTimer>
#include <QTextBlock>
#include <algorithm>

// 函数说明：构造 EnhancedSpellChecker 对象，初始化本模块需要的状态、界面和资源。
EnhancedSpellChecker::EnhancedSpellChecker(QObject *parent)
    : QObject(parent)
    , m_editor(nullptr)
    , m_autoCorrectEnabled(true)
    , m_realTimeCheck(true)
    , m_highlightErrors(true)
    , m_errorColor(QColor(255, 0, 0, 100))
    , m_minWordLength(2)
    , m_checkedWordCount(0)
    , m_checkTimer(new QTimer(this))
{
    m_activeLanguages.append(Language::English_US);

    m_checkTimer->setSingleShot(true);
    m_checkTimer->setInterval(500);
    connect(m_checkTimer, &QTimer::timeout,
            this, &EnhancedSpellChecker::performCheck);

    setupDefaultAutoCorrect();
}

// 函数说明：销毁 EnhancedSpellChecker 对象，释放本模块持有的资源。
EnhancedSpellChecker::~EnhancedSpellChecker()
{
}

// 函数说明：设置 EnhancedSpellChecker 的运行参数，并触发必要的界面或数据刷新。
void EnhancedSpellChecker::setEditor(QPlainTextEdit *editor)
{
    if (m_editor) {
        disconnect(m_editor, nullptr, this, nullptr);
        clearHighlights();
    }

    m_editor = editor;

    if (m_editor) {
        connect(m_editor, &QPlainTextEdit::textChanged,
                this, &EnhancedSpellChecker::onTextChanged);

        if (m_realTimeCheck) {
            recheckDocument();
        }
    }
}

// 函数说明：设置 EnhancedSpellChecker 的运行参数，并触发必要的界面或数据刷新。
void EnhancedSpellChecker::setLanguage(Language lang)
{
    m_activeLanguages.clear();
    m_activeLanguages.append(lang);
    recheckDocument();
}

// 函数说明：向 EnhancedSpellChecker 管理的数据集合中添加一项内容。
void EnhancedSpellChecker::addLanguage(Language lang)
{
    if (!m_activeLanguages.contains(lang)) {
        m_activeLanguages.append(lang);
        recheckDocument();
    }
}

// 函数说明：从 EnhancedSpellChecker 管理的数据集合中移除指定内容。
void EnhancedSpellChecker::removeLanguage(Language lang)
{
    m_activeLanguages.removeAll(lang);
    recheckDocument();
}

// 函数说明：判断 EnhancedSpellChecker 当前是否满足指定状态。
bool EnhancedSpellChecker::isLanguageAvailable(Language lang) const
{
    // 检查语言词典是否可用
    Q_UNUSED(lang)
    return true;  // 简化实现
}

// 函数说明：实现 EnhancedSpellChecker::checkWord 的核心逻辑，供当前模块调用。
bool EnhancedSpellChecker::checkWord(const QString &word)
{
    if (word.length() < m_minWordLength) {
        return true;
    }

    QString normalized = normalizeWord(word);

    // 检查是否忽略
    if (m_ignoredWords.contains(normalized)) {
        return true;
    }

    // 检查用户词典
    if (m_userDictionary.contains(normalized)) {
        return true;
    }

    // 检查主词典
    if (m_dictionary.contains(normalized)) {
        return true;
    }

    // 检查专业词典
    for (auto it = m_technicalDictionaries.begin();
         it != m_technicalDictionaries.end(); ++it) {
        if (m_technicalDictionaryEnabled.value(it.key(), true)) {
            if (it.value().contains(normalized)) {
                return true;
            }
        }
    }

    // 检查是否是有效的词（数字、特殊格式等）
    if (isValidWord(word)) {
        return true;
    }

    return false;
}

// 函数说明：读取 EnhancedSpellChecker 当前保存的状态或计算结果。
QStringList EnhancedSpellChecker::getSuggestions(const QString &word, int maxCount)
{
    QStringList suggestions;
    QString normalized = normalizeWord(word);

    // 查找相似词
    QStringList similar = findSimilarWords(normalized);

    // 按编辑距离排序
    std::sort(similar.begin(), similar.end(),
              [this, &normalized](const QString &a, const QString &b) {
                  return levenshteinDistance(normalized, a) <
                         levenshteinDistance(normalized, b);
              });

    // 取前 N 个
    for (int i = 0; i < qMin(maxCount, similar.size()); ++i) {
        suggestions.append(similar[i]);
    }

    return suggestions;
}

// 函数说明：实现 EnhancedSpellChecker::checkText 的核心逻辑，供当前模块调用。
QVector<EnhancedSpellChecker::SpellingError> EnhancedSpellChecker::checkText(const QString &text)
{
    QVector<SpellingError> errors;
    m_checkedWordCount = 0;

    // 匹配单词（支持中英文混合）
    QRegularExpression wordRegex("\\b[a-zA-Z]+\\b");
    QRegularExpressionMatchIterator it = wordRegex.globalMatch(text);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString word = match.captured();
        m_checkedWordCount++;

        if (!checkWord(word)) {
            SpellingError error;
            error.position = match.capturedStart();
            error.length = match.capturedLength();
            error.word = word;
            error.suggestions = getSuggestions(word);
            error.isIgnored = false;
            errors.append(error);
        }
    }

    return errors;
}

// 函数说明：实现 EnhancedSpellChecker::checkDocument 的核心逻辑，供当前模块调用。
void EnhancedSpellChecker::checkDocument()
{
    if (!m_editor) return;

    emit checkingStarted();

    QString text = m_editor->toPlainText();
    m_currentErrors = checkText(text);

    if (m_highlightErrors) {
        highlightErrors();
    }

    emit spellingErrorsFound(m_currentErrors);
    emit checkingFinished();
}

// 函数说明：向 EnhancedSpellChecker 管理的数据集合中添加一项内容。
bool EnhancedSpellChecker::addToUserDictionary(const QString &word)
{
    QString normalized = normalizeWord(word);
    if (!m_userDictionary.contains(normalized)) {
        m_userDictionary.insert(normalized);
        emit dictionaryUpdated();
        recheckDocument();
        return true;
    }
    return false;
}

// 函数说明：从 EnhancedSpellChecker 管理的数据集合中移除指定内容。
bool EnhancedSpellChecker::removeFromUserDictionary(const QString &word)
{
    QString normalized = normalizeWord(word);
    if (m_userDictionary.remove(normalized)) {
        emit dictionaryUpdated();
        recheckDocument();
        return true;
    }
    return false;
}

// 函数说明：加载 EnhancedSpellChecker 需要的数据、配置或外部资源。
bool EnhancedSpellChecker::loadUserDictionary(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    while (!in.atEnd()) {
        QString word = in.readLine().trimmed();
        if (!word.isEmpty() && !word.startsWith('#')) {
            m_userDictionary.insert(normalizeWord(word));
        }
    }

    file.close();
    emit dictionaryUpdated();
    return true;
}

// 函数说明：保存 EnhancedSpellChecker 当前状态，保证用户修改可以持久化。
bool EnhancedSpellChecker::saveUserDictionary(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    out << "# 用户词典\n";
    out << "# User Dictionary\n\n";

    QStringList words = m_userDictionary.values();
    words.sort();
    for (const QString &word : words) {
        out << word << "\n";
    }

    file.close();
    return true;
}

// 函数说明：加载 EnhancedSpellChecker 需要的数据、配置或外部资源。
bool EnhancedSpellChecker::loadTechnicalDictionary(const QString &name, const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QSet<QString> dictionary;
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    while (!in.atEnd()) {
        QString word = in.readLine().trimmed();
        if (!word.isEmpty() && !word.startsWith('#')) {
            dictionary.insert(normalizeWord(word));
        }
    }

    file.close();

    m_technicalDictionaries[name] = dictionary;
    m_technicalDictionaryEnabled[name] = true;

    emit dictionaryUpdated();
    return true;
}

// 函数说明：实现 EnhancedSpellChecker::enableTechnicalDictionary 的核心逻辑，供当前模块调用。
void EnhancedSpellChecker::enableTechnicalDictionary(const QString &name, bool enable)
{
    if (m_technicalDictionaries.contains(name)) {
        m_technicalDictionaryEnabled[name] = enable;
        recheckDocument();
    }
}

// 函数说明：实现 EnhancedSpellChecker::ignoreWord 的核心逻辑，供当前模块调用。
void EnhancedSpellChecker::ignoreWord(const QString &word)
{
    m_ignoredWords.insert(normalizeWord(word));
    recheckDocument();
}

// 函数说明：实现 EnhancedSpellChecker::ignoreAll 的核心逻辑，供当前模块调用。
void EnhancedSpellChecker::ignoreAll(const QString &word)
{
    ignoreWord(word);
}

// 函数说明：实现 EnhancedSpellChecker::unignoreWord 的核心逻辑，供当前模块调用。
void EnhancedSpellChecker::unignoreWord(const QString &word)
{
    m_ignoredWords.remove(normalizeWord(word));
    recheckDocument();
}

// 函数说明：判断 EnhancedSpellChecker 当前是否满足指定状态。
bool EnhancedSpellChecker::isIgnored(const QString &word) const
{
    return m_ignoredWords.contains(normalizeWord(word));
}

// 函数说明：设置 EnhancedSpellChecker 的运行参数，并触发必要的界面或数据刷新。
void EnhancedSpellChecker::setAutoCorrectEnabled(bool enabled)
{
    m_autoCorrectEnabled = enabled;
}

// 函数说明：向 EnhancedSpellChecker 管理的数据集合中添加一项内容。
void EnhancedSpellChecker::addAutoCorrectRule(const QString &from, const QString &to)
{
    AutoCorrectRule rule;
    rule.from = from;
    rule.to = to;
    rule.enabled = true;
    rule.useCount = 0;
    m_autoCorrectRules[from] = rule;
}

// 函数说明：从 EnhancedSpellChecker 管理的数据集合中移除指定内容。
void EnhancedSpellChecker::removeAutoCorrectRule(const QString &from)
{
    m_autoCorrectRules.remove(from);
}

// 函数说明：实现 EnhancedSpellChecker::autoCorrectRules 的核心逻辑，供当前模块调用。
QVector<EnhancedSpellChecker::AutoCorrectRule> EnhancedSpellChecker::autoCorrectRules() const
{
    QVector<AutoCorrectRule> rules;
    for (const auto &rule : m_autoCorrectRules) {
        rules.append(rule);
    }
    return rules;
}

// 函数说明：加载 EnhancedSpellChecker 需要的数据、配置或外部资源。
bool EnhancedSpellChecker::loadAutoCorrectRules(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (doc.isNull()) return false;

    QJsonArray rulesArray = doc.object()["rules"].toArray();
    for (const QJsonValue &value : rulesArray) {
        QJsonObject obj = value.toObject();
        AutoCorrectRule rule;
        rule.from = obj["from"].toString();
        rule.to = obj["to"].toString();
        rule.enabled = obj["enabled"].toBool(true);
        rule.useCount = obj["useCount"].toInt(0);
        m_autoCorrectRules[rule.from] = rule;
    }

    return true;
}

// 函数说明：保存 EnhancedSpellChecker 当前状态，保证用户修改可以持久化。
bool EnhancedSpellChecker::saveAutoCorrectRules(const QString &filePath)
{
    QJsonArray rulesArray;
    for (const auto &rule : m_autoCorrectRules) {
        QJsonObject obj;
        obj["from"] = rule.from;
        obj["to"] = rule.to;
        obj["enabled"] = rule.enabled;
        obj["useCount"] = rule.useCount;
        rulesArray.append(obj);
    }

    QJsonObject root;
    root["version"] = 1;
    root["rules"] = rulesArray;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    file.write(QJsonDocument(root).toJson());
    file.close();
    return true;
}

// 函数说明：设置 EnhancedSpellChecker 的运行参数，并触发必要的界面或数据刷新。
void EnhancedSpellChecker::setRealTimeCheck(bool enabled)
{
    m_realTimeCheck = enabled;
    if (enabled) {
        recheckDocument();
    } else {
        clearHighlights();
    }
}

// 函数说明：设置 EnhancedSpellChecker 的运行参数，并触发必要的界面或数据刷新。
void EnhancedSpellChecker::setHighlightErrors(bool enabled)
{
    m_highlightErrors = enabled;
    if (enabled) {
        highlightErrors();
    } else {
        clearHighlights();
    }
}

// 函数说明：设置 EnhancedSpellChecker 的运行参数，并触发必要的界面或数据刷新。
void EnhancedSpellChecker::setErrorHighlightColor(const QColor &color)
{
    m_errorColor = color;
    highlightErrors();
}

// 函数说明：设置 EnhancedSpellChecker 的运行参数，并触发必要的界面或数据刷新。
void EnhancedSpellChecker::setMinWordLength(int length)
{
    m_minWordLength = length;
    recheckDocument();
}

// 函数说明：实现 EnhancedSpellChecker::recheckDocument 的核心逻辑，供当前模块调用。
void EnhancedSpellChecker::recheckDocument()
{
    if (m_realTimeCheck && m_editor) {
        checkDocument();
    }
}

// 函数说明：清空 EnhancedSpellChecker 保存的临时状态或缓存数据。
void EnhancedSpellChecker::clearErrors()
{
    m_currentErrors.clear();
    clearHighlights();
}

// 函数说明：响应 EnhancedSpellChecker 收到的信号或异步回调，并更新界面状态。
void EnhancedSpellChecker::onTextChanged()
{
    if (m_realTimeCheck) {
        m_checkTimer->start();
    }
}

// 函数说明：实现 EnhancedSpellChecker::performCheck 的核心逻辑，供当前模块调用。
void EnhancedSpellChecker::performCheck()
{
    checkDocument();
}

// 函数说明：初始化 EnhancedSpellChecker 的 setupDefaultAutoCorrect 相关界面、动作或服务连接。
void EnhancedSpellChecker::setupDefaultAutoCorrect()
{
    // 常见拼写错误
    addAutoCorrectRule("teh", "the");
    addAutoCorrectRule("adn", "and");
    addAutoCorrectRule("taht", "that");
    addAutoCorrectRule("wiht", "with");
    addAutoCorrectRule("hte", "the");
    addAutoCorrectRule("fro", "for");
    addAutoCorrectRule("yuo", "you");
    addAutoCorrectRule("cna", "can");
    addAutoCorrectRule("ahve", "have");
    addAutoCorrectRule("dont", "don't");
    addAutoCorrectRule("doesnt", "doesn't");
    addAutoCorrectRule("cant", "can't");
    addAutoCorrectRule("wont", "won't");
    addAutoCorrectRule("im", "I'm");
    addAutoCorrectRule("ive", "I've");
    addAutoCorrectRule("youre", "you're");
    addAutoCorrectRule("theyre", "they're");
    addAutoCorrectRule("thier", "their");
    addAutoCorrectRule("recieve", "receive");
    addAutoCorrectRule("beleive", "believe");
    addAutoCorrectRule("occured", "occurred");
}

// 函数说明：判断 EnhancedSpellChecker 当前是否满足指定状态。
bool EnhancedSpellChecker::isValidWord(const QString &word)
{
    // 检查是否是数字
    bool isNumber;
    word.toDouble(&isNumber);
    if (isNumber) return true;

    // 检查是否是常见缩写或格式
    QRegularExpression patterns[] = {
        QRegularExpression("^[A-Z]+$"),           // 全大写缩写
        QRegularExpression("^\\d+[a-zA-Z]+$"),    // 数字+字母
        QRegularExpression("^[a-zA-Z]+\\d+$"),    // 字母+数字
        QRegularExpression("^v\\d+(\\.\\d+)*$"),  // 版本号
    };

    for (const auto &regex : patterns) {
        if (regex.match(word).hasMatch()) {
            return true;
        }
    }

    return false;
}

// 函数说明：实现 EnhancedSpellChecker::normalizeWord 的核心逻辑，供当前模块调用。
QString EnhancedSpellChecker::normalizeWord(const QString &word) const
{
    return word.toLower().trimmed();
}

// 函数说明：实现 EnhancedSpellChecker::highlightErrors 的核心逻辑，供当前模块调用。
void EnhancedSpellChecker::highlightErrors()
{
    if (!m_editor || !m_highlightErrors) return;

    m_errorSelections.clear();

    QTextCharFormat format;
    format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    format.setUnderlineColor(m_errorColor);

    for (const auto &error : m_currentErrors) {
        QTextEdit::ExtraSelection selection;
        selection.format = format;

        QTextCursor cursor(m_editor->document());
        cursor.setPosition(error.position);
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, error.length);
        selection.cursor = cursor;

        m_errorSelections.append(selection);
    }

    // 合并现有的额外选择
    QList<QTextEdit::ExtraSelection> allSelections = m_editor->extraSelections();

    // 移除旧的拼写错误高亮（假设使用特定的波浪线样式）
    QList<QTextEdit::ExtraSelection> filteredSelections;
    for (const auto &sel : allSelections) {
        if (sel.format.underlineStyle() != QTextCharFormat::WaveUnderline) {
            filteredSelections.append(sel);
        }
    }

    filteredSelections.append(m_errorSelections);
    m_editor->setExtraSelections(filteredSelections);
}

// 函数说明：清空 EnhancedSpellChecker 保存的临时状态或缓存数据。
void EnhancedSpellChecker::clearHighlights()
{
    if (!m_editor) return;

    QList<QTextEdit::ExtraSelection> selections = m_editor->extraSelections();
    QList<QTextEdit::ExtraSelection> filtered;

    for (const auto &sel : selections) {
        if (sel.format.underlineStyle() != QTextCharFormat::WaveUnderline) {
            filtered.append(sel);
        }
    }

    m_editor->setExtraSelections(filtered);
    m_errorSelections.clear();
}

// 函数说明：实现 EnhancedSpellChecker::autoCorrect 的核心逻辑，供当前模块调用。
QString EnhancedSpellChecker::autoCorrect(const QString &word)
{
    if (!m_autoCorrectEnabled) return word;

    if (m_autoCorrectRules.contains(word)) {
        AutoCorrectRule &rule = m_autoCorrectRules[word];
        if (rule.enabled) {
            rule.useCount++;
            emit wordCorrected(word, rule.to);
            return rule.to;
        }
    }

    return word;
}

// 函数说明：实现 EnhancedSpellChecker::levenshteinDistance 的核心逻辑，供当前模块调用。
int EnhancedSpellChecker::levenshteinDistance(const QString &s1, const QString &s2)
{
    int len1 = s1.length();
    int len2 = s2.length();

    QVector<QVector<int>> dp(len1 + 1, QVector<int>(len2 + 1));

    for (int i = 0; i <= len1; ++i) dp[i][0] = i;
    for (int j = 0; j <= len2; ++j) dp[0][j] = j;

    for (int i = 1; i <= len1; ++i) {
        for (int j = 1; j <= len2; ++j) {
            int cost = (s1[i-1] == s2[j-1]) ? 0 : 1;
            dp[i][j] = qMin(qMin(
                dp[i-1][j] + 1,      // 删除
                dp[i][j-1] + 1),     // 插入
                dp[i-1][j-1] + cost  // 替换
            );
        }
    }

    return dp[len1][len2];
}

// 函数说明：实现 EnhancedSpellChecker::findSimilarWords 的核心逻辑，供当前模块调用。
QStringList EnhancedSpellChecker::findSimilarWords(const QString &word, int maxDistance)
{
    QStringList similar;
    QString normalized = normalizeWord(word);

    // 搜索用户词典
    for (const QString &dictWord : m_userDictionary) {
        if (levenshteinDistance(normalized, dictWord) <= maxDistance) {
            similar.append(dictWord);
        }
    }

    // 搜索主词典
    for (const QString &dictWord : m_dictionary) {
        if (levenshteinDistance(normalized, dictWord) <= maxDistance) {
            similar.append(dictWord);
        }
    }

    // 搜索专业词典
    for (auto it = m_technicalDictionaries.begin();
         it != m_technicalDictionaries.end(); ++it) {
        if (m_technicalDictionaryEnabled.value(it.key(), true)) {
            for (const QString &dictWord : it.value()) {
                if (levenshteinDistance(normalized, dictWord) <= maxDistance) {
                    similar.append(dictWord);
                }
            }
        }
    }

    similar.removeDuplicates();
    return similar;
}

