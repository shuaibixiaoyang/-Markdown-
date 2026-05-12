// 文件说明：app-static\security\passwordstrength.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef PASSWORDSTRENGTH_H
#define PASSWORDSTRENGTH_H

#include <QString>
#include <QStringList>
#include <QSet>
#include <QRegularExpression>

/**
 * @brief 密码强度分析器
 * 
 * 基于多种因素评估密码强度：
 * - 长度
 * - 字符多样性（大小写、数字、符号）
 * - 常见模式检测
 * - 字典攻击检测
 * - 键盘模式检测
 * - 重复字符检测
 */
class PasswordStrength
{
public:
    // 强度级别
    enum class Level {
        VeryWeak,   // 0-20
        Weak,       // 21-40
        Fair,       // 41-60
        Strong,     // 61-80
        VeryStrong  // 81-100
    };
    
    // 分析结果
    struct Analysis {
        int score;              // 0-100
        Level level;
        QString levelText;
        QStringList warnings;
        QStringList suggestions;
        
        // 详细评分
        int lengthScore;
        int charsetScore;
        int patternPenalty;
        int bonusScore;
        
        // 估计破解时间
        QString crackTimeDisplay;
        double crackTimeSeconds;
    };
    
    PasswordStrength();
    
    // 分析密码
    Analysis analyze(const QString &password) const;
    
    // 快速评分 (0-100)
    int score(const QString &password) const;
    
    // 获取强度级别
    Level level(const QString &password) const;
    
    // 检查是否满足最低要求
    bool meetsMinimumRequirements(const QString &password, 
                                   int minLength = 8,
                                   bool requireUpper = true,
                                   bool requireLower = true,
                                   bool requireDigit = true,
                                   bool requireSymbol = false) const;
    
    // 生成密码建议
    QStringList generateSuggestions(const QString &password) const;
    
private:
    // 评分因素
    int calculateLengthScore(const QString &password) const;
    int calculateCharsetScore(const QString &password) const;
    int calculatePatternPenalty(const QString &password) const;
    int calculateBonusScore(const QString &password) const;
    
    // 模式检测
    bool hasSequentialChars(const QString &password, int minLength = 3) const;
    bool hasRepeatedChars(const QString &password, int minRepeat = 3) const;
    bool hasKeyboardPattern(const QString &password) const;
    bool isCommonPassword(const QString &password) const;
    bool hasDatePattern(const QString &password) const;
    
    // 熵计算
    double calculateEntropy(const QString &password) const;
    double estimateCrackTime(double entropy) const;
    QString formatCrackTime(double seconds) const;
    
    // 字符集大小
    int getCharsetSize(const QString &password) const;
    
    // 常见密码列表
    QSet<QString> m_commonPasswords;
    
    // 键盘布局模式
    QStringList m_keyboardPatterns;
    
    // 常见序列
    QStringList m_sequences;
};


// ==================== 实现 ====================

inline PasswordStrength::PasswordStrength()
{
    // 常见弱密码（前100个最常见的）
    m_commonPasswords = {
        "password", "123456", "12345678", "qwerty", "abc123",
        "monkey", "1234567", "letmein", "trustno1", "dragon",
        "baseball", "iloveyou", "master", "sunshine", "ashley",
        "bailey", "shadow", "123123", "654321", "superman",
        "qazwsx", "michael", "football", "password1", "password123",
        "batman", "login", "admin", "welcome", "hello",
        "charlie", "donald", "password2", "qwerty123", "hello123",
        "1q2w3e4r", "1234qwer", "q1w2e3r4", "123qwe", "zxcvbn"
    };
    
    // 键盘模式
    m_keyboardPatterns = {
        "qwerty", "qwertz", "azerty", "qweasd", "asdfgh",
        "zxcvbn", "1qaz2wsx", "1234qwer", "qwer1234", "!@#$%",
        "12345", "09876", "aaaaa", "11111", "00000"
    };
    
    // 常见序列
    m_sequences = {
        "abc", "bcd", "cde", "def", "efg", "fgh", "ghi", "hij",
        "ijk", "jkl", "klm", "lmn", "mno", "nop", "opq", "pqr",
        "qrs", "rst", "stu", "tuv", "uvw", "vwx", "wxy", "xyz",
        "012", "123", "234", "345", "456", "567", "678", "789"
    };
}

inline PasswordStrength::Analysis PasswordStrength::analyze(const QString &password) const
{
    Analysis result;
    
    if (password.isEmpty()) {
        result.score = 0;
        result.level = Level::VeryWeak;
        result.levelText = QObject::tr("非常弱");
        result.warnings << QObject::tr("密码不能为空");
        return result;
    }
    
    // 计算各项评分
    result.lengthScore = calculateLengthScore(password);
    result.charsetScore = calculateCharsetScore(password);
    result.patternPenalty = calculatePatternPenalty(password);
    result.bonusScore = calculateBonusScore(password);
    
    // 总分
    result.score = qBound(0, 
        result.lengthScore + result.charsetScore + result.bonusScore - result.patternPenalty, 
        100);
    
    // 确定级别
    if (result.score <= 20) {
        result.level = Level::VeryWeak;
        result.levelText = QObject::tr("非常弱");
    } else if (result.score <= 40) {
        result.level = Level::Weak;
        result.levelText = QObject::tr("弱");
    } else if (result.score <= 60) {
        result.level = Level::Fair;
        result.levelText = QObject::tr("中等");
    } else if (result.score <= 80) {
        result.level = Level::Strong;
        result.levelText = QObject::tr("强");
    } else {
        result.level = Level::VeryStrong;
        result.levelText = QObject::tr("非常强");
    }
    
    // 警告
    if (password.length() < 8) {
        result.warnings << QObject::tr("密码太短（至少8个字符）");
    }
    if (isCommonPassword(password)) {
        result.warnings << QObject::tr("这是一个常见的弱密码");
    }
    if (hasSequentialChars(password)) {
        result.warnings << QObject::tr("包含连续字符序列");
    }
    if (hasRepeatedChars(password)) {
        result.warnings << QObject::tr("包含重复字符");
    }
    if (hasKeyboardPattern(password)) {
        result.warnings << QObject::tr("包含键盘模式");
    }
    if (hasDatePattern(password)) {
        result.warnings << QObject::tr("包含日期模式");
    }
    
    // 建议
    result.suggestions = generateSuggestions(password);
    
    // 破解时间估计
    double entropy = calculateEntropy(password);
    result.crackTimeSeconds = estimateCrackTime(entropy);
    result.crackTimeDisplay = formatCrackTime(result.crackTimeSeconds);
    
    return result;
}

inline int PasswordStrength::score(const QString &password) const
{
    return analyze(password).score;
}

inline PasswordStrength::Level PasswordStrength::level(const QString &password) const
{
    return analyze(password).level;
}

inline bool PasswordStrength::meetsMinimumRequirements(
    const QString &password,
    int minLength,
    bool requireUpper,
    bool requireLower,
    bool requireDigit,
    bool requireSymbol) const
{
    if (password.length() < minLength) return false;
    
    bool hasUpper = false, hasLower = false, hasDigit = false, hasSymbol = false;
    
    for (const QChar &c : password) {
        if (c.isUpper()) hasUpper = true;
        else if (c.isLower()) hasLower = true;
        else if (c.isDigit()) hasDigit = true;
        else if (!c.isLetterOrNumber()) hasSymbol = true;
    }
    
    if (requireUpper && !hasUpper) return false;
    if (requireLower && !hasLower) return false;
    if (requireDigit && !hasDigit) return false;
    if (requireSymbol && !hasSymbol) return false;
    
    return true;
}

inline int PasswordStrength::calculateLengthScore(const QString &password) const
{
    int len = password.length();
    if (len == 0) return 0;
    if (len < 6) return 5;
    if (len < 8) return 10;
    if (len < 10) return 20;
    if (len < 12) return 25;
    if (len < 14) return 30;
    if (len < 16) return 35;
    return 40;  // 16+ 字符
}

inline int PasswordStrength::calculateCharsetScore(const QString &password) const
{
    bool hasLower = false, hasUpper = false, hasDigit = false, hasSymbol = false;
    
    for (const QChar &c : password) {
        if (c.isLower()) hasLower = true;
        else if (c.isUpper()) hasUpper = true;
        else if (c.isDigit()) hasDigit = true;
        else hasSymbol = true;
    }
    
    int score = 0;
    int typesCount = 0;
    
    if (hasLower) { score += 10; typesCount++; }
    if (hasUpper) { score += 10; typesCount++; }
    if (hasDigit) { score += 10; typesCount++; }
    if (hasSymbol) { score += 15; typesCount++; }
    
    // 多种类型奖励
    if (typesCount >= 3) score += 5;
    if (typesCount == 4) score += 10;
    
    return score;
}

inline int PasswordStrength::calculatePatternPenalty(const QString &password) const
{
    int penalty = 0;
    
    if (isCommonPassword(password)) penalty += 40;
    if (hasSequentialChars(password)) penalty += 15;
    if (hasRepeatedChars(password)) penalty += 15;
    if (hasKeyboardPattern(password)) penalty += 20;
    if (hasDatePattern(password)) penalty += 10;
    
    // 全部相同字符
    if (password.count(password[0]) == password.length()) {
        penalty += 30;
    }
    
    return penalty;
}

inline int PasswordStrength::calculateBonusScore(const QString &password) const
{
    int bonus = 0;
    
    // 中间数字或符号
    QString middle = password.mid(1, password.length() - 2);
    int middleSymbolDigit = 0;
    for (const QChar &c : middle) {
        if (c.isDigit() || !c.isLetterOrNumber()) {
            middleSymbolDigit++;
        }
    }
    bonus += qMin(middleSymbolDigit * 2, 10);
    
    // 唯一字符比例
    QSet<QChar> uniqueChars;
    for (const QChar &c : password) {
        uniqueChars.insert(c);
    }
    double uniqueRatio = static_cast<double>(uniqueChars.size()) / password.length();
    if (uniqueRatio > 0.8) bonus += 5;
    
    return bonus;
}

inline bool PasswordStrength::hasSequentialChars(const QString &password, int minLength) const
{
    QString lower = password.toLower();
    
    for (const QString &seq : m_sequences) {
        if (lower.contains(seq)) return true;
        // 反向检查
        QString reversed = seq;
        std::reverse(reversed.begin(), reversed.end());
        if (lower.contains(reversed)) return true;
    }
    
    return false;
}

inline bool PasswordStrength::hasRepeatedChars(const QString &password, int minRepeat) const
{
    if (password.length() < minRepeat) return false;
    
    int count = 1;
    QChar lastChar = password[0];
    
    for (int i = 1; i < password.length(); ++i) {
        if (password[i] == lastChar) {
            count++;
            if (count >= minRepeat) return true;
        } else {
            count = 1;
            lastChar = password[i];
        }
    }
    
    return false;
}

inline bool PasswordStrength::hasKeyboardPattern(const QString &password) const
{
    QString lower = password.toLower();
    
    for (const QString &pattern : m_keyboardPatterns) {
        if (lower.contains(pattern)) return true;
    }
    
    return false;
}

inline bool PasswordStrength::isCommonPassword(const QString &password) const
{
    return m_commonPasswords.contains(password.toLower());
}

inline bool PasswordStrength::hasDatePattern(const QString &password) const
{
    // 检测常见日期格式
    static QRegularExpression datePatterns[] = {
        QRegularExpression("\\d{4}[-/]?\\d{2}[-/]?\\d{2}"),  // YYYY-MM-DD
        QRegularExpression("\\d{2}[-/]?\\d{2}[-/]?\\d{4}"),  // DD-MM-YYYY
        QRegularExpression("19\\d{2}|20\\d{2}"),              // 年份
        QRegularExpression("0[1-9]|1[0-2]")                   // 月份
    };
    
    for (const auto &pattern : datePatterns) {
        if (pattern.match(password).hasMatch()) {
            return true;
        }
    }
    
    return false;
}

inline double PasswordStrength::calculateEntropy(const QString &password) const
{
    int charsetSize = getCharsetSize(password);
    if (charsetSize == 0) return 0;
    
    return password.length() * log2(charsetSize);
}

inline int PasswordStrength::getCharsetSize(const QString &password) const
{
    bool hasLower = false, hasUpper = false, hasDigit = false, hasSymbol = false;
    
    for (const QChar &c : password) {
        if (c.isLower()) hasLower = true;
        else if (c.isUpper()) hasUpper = true;
        else if (c.isDigit()) hasDigit = true;
        else hasSymbol = true;
    }
    
    int size = 0;
    if (hasLower) size += 26;
    if (hasUpper) size += 26;
    if (hasDigit) size += 10;
    if (hasSymbol) size += 32;  // 常见符号
    
    return size;
}

inline double PasswordStrength::estimateCrackTime(double entropy) const
{
    // 假设每秒 10 亿次尝试 (GPU 暴力破解)
    const double attemptsPerSecond = 1e9;
    double combinations = pow(2, entropy);
    return combinations / attemptsPerSecond / 2;  // 平均情况
}

inline QString PasswordStrength::formatCrackTime(double seconds) const
{
    if (seconds < 1) return QObject::tr("瞬间");
    if (seconds < 60) return QObject::tr("%1 秒").arg(static_cast<int>(seconds));
    if (seconds < 3600) return QObject::tr("%1 分钟").arg(static_cast<int>(seconds / 60));
    if (seconds < 86400) return QObject::tr("%1 小时").arg(static_cast<int>(seconds / 3600));
    if (seconds < 2592000) return QObject::tr("%1 天").arg(static_cast<int>(seconds / 86400));
    if (seconds < 31536000) return QObject::tr("%1 个月").arg(static_cast<int>(seconds / 2592000));
    if (seconds < 3153600000) return QObject::tr("%1 年").arg(static_cast<int>(seconds / 31536000));
    return QObject::tr("数百年");
}

inline QStringList PasswordStrength::generateSuggestions(const QString &password) const
{
    QStringList suggestions;
    
    if (password.length() < 12) {
        suggestions << QObject::tr("增加密码长度到至少12个字符");
    }
    
    bool hasLower = false, hasUpper = false, hasDigit = false, hasSymbol = false;
    for (const QChar &c : password) {
        if (c.isLower()) hasLower = true;
        else if (c.isUpper()) hasUpper = true;
        else if (c.isDigit()) hasDigit = true;
        else hasSymbol = true;
    }
    
    if (!hasUpper) suggestions << QObject::tr("添加大写字母");
    if (!hasLower) suggestions << QObject::tr("添加小写字母");
    if (!hasDigit) suggestions << QObject::tr("添加数字");
    if (!hasSymbol) suggestions << QObject::tr("添加特殊符号 (!@#$%^&*)");
    
    if (isCommonPassword(password)) {
        suggestions << QObject::tr("使用不常见的密码组合");
    }
    
    if (hasKeyboardPattern(password) || hasSequentialChars(password)) {
        suggestions << QObject::tr("避免使用连续字符或键盘模式");
    }
    
    if (suggestions.isEmpty()) {
        suggestions << QObject::tr("密码强度良好！");
    }
    
    return suggestions;
}

#endif // PASSWORDSTRENGTH_H

