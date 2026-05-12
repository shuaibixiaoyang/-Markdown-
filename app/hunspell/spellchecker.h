// 文件说明：app\hunspell\spellchecker.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef HUNSPELL_SPELLCHECKER_H
#define HUNSPELL_SPELLCHECKER_H

#include <QtCore/qmap.h>
#include <QtCore/qstring.h>
#include <QStringConverter>

class Dictionary;
class Hunspell;

namespace hunspell {

class SpellChecker
{
public:
    SpellChecker();
    ~SpellChecker();

    bool isCorrect(const QString &word);
    QStringList suggestions(const QString &word);
    void addToUserWordlist(const QString &word);

    void loadDictionary(const QString &dictFilePath);
    void loadUserWordlist(const QString &userWordlistPath);

    static QMap<QString, Dictionary> availableDictionaries();

#ifdef Q_OS_MAC
    // macOS 原生拼写检查（在 spellchecker_macx.mm 中实现）
    static void nativeSetLanguage(const QString &language);
    static bool nativeIsCorrect(const QString &word);
    static QStringList nativeSuggestions(const QString &word);
#endif

private:
    Hunspell *hunspellChecker;
    QString userWordlist;
    QStringEncoder *encoder;
    QStringDecoder *decoder;
    bool m_useNativeChecker = false;
};

} // namespace Hunspell

#endif // HUNSPELL_SPELLCHECKER_H

