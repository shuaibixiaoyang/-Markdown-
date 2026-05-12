// 文件说明：app\hunspell\spellchecker_macx.mm
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "spellchecker.h"
using hunspell::SpellChecker;

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QDirIterator>

#include <spellchecker/dictionary.h>

#import <AppKit/AppKit.h>

QMap<QString, Dictionary> SpellChecker::availableDictionaries()
{
    QMap<QString, Dictionary> dictionaries;

    // 1. 先搜索 Hunspell 字典文件（多个路径）
    QStringList paths;
    paths << (qApp->applicationDirPath() + "/../Resources/dictionaries")
          << "/opt/homebrew/share/hunspell"
          << "/usr/local/share/hunspell"
          << "/usr/share/hunspell"
          << "/Library/Spelling"
          << QDir::homePath() + "/Library/Spelling";

    for (const QString &path : paths) {
        QDir dictPath(path);
        dictPath.setFilter(QDir::Files);
        dictPath.setNameFilters(QStringList() << "*.dic");
        if (dictPath.exists()) {
            QDirIterator it(dictPath);
            while (it.hasNext()) {
                it.next();

                // 检查同名 .aff 文件是否存在
                QString affPath = it.filePath();
                affPath.replace(".dic", ".aff");
                if (!QFile::exists(affPath)) continue;

                QString language = it.fileName();
                language.remove(".dic");
                language.truncate(5); // 只保留语言和国家代码

                Dictionary dict(language, it.filePath());
                dictionaries.insert(language, dict);
            }
        }
    }

    // 2. 如果没有找到 Hunspell 字典，使用 macOS 系统拼写检查器获取可用语言
    if (dictionaries.isEmpty()) {
        @autoreleasepool {
            NSSpellChecker *checker = [NSSpellChecker sharedSpellChecker];
            NSArray<NSString *> *languages = [checker availableLanguages];

            for (NSString *lang in languages) {
                QString langStr = QString::fromNSString(lang);
                // NSSpellChecker 返回的格式可能是 "en" 或 "en_US" 或 "en-US"
                langStr.replace('-', '_');

                // 空 filePath 表示使用系统拼写检查器
                Dictionary dict(langStr, QString());
                dictionaries.insert(langStr, dict);
            }
        }
    }

    return dictionaries;
}

// macOS 原生拼写检查功能
namespace hunspell {

void SpellChecker::nativeSetLanguage(const QString &language)
{
    @autoreleasepool {
        NSSpellChecker *checker = [NSSpellChecker sharedSpellChecker];
        NSString *lang = language.toNSString();
        [checker setLanguage:lang];
    }
}

bool SpellChecker::nativeIsCorrect(const QString &word)
{
    @autoreleasepool {
        NSSpellChecker *checker = [NSSpellChecker sharedSpellChecker];
        NSString *nsWord = word.toNSString();
        NSRange range = [checker checkSpellingOfString:nsWord startingAt:0];
        return range.location == NSNotFound;
    }
}

QStringList SpellChecker::nativeSuggestions(const QString &word)
{
    QStringList result;
    @autoreleasepool {
        NSSpellChecker *checker = [NSSpellChecker sharedSpellChecker];
        NSString *nsWord = word.toNSString();
        NSArray<NSString *> *guesses = [checker guessesForWordRange:NSMakeRange(0, nsWord.length)
                                                           inString:nsWord
                                                           language:[checker language]
                                                 inSpellDocumentWithTag:0];
        for (NSString *guess in guesses) {
            result << QString::fromNSString(guess);
        }
    }
    return result;
}

} // namespace hunspell

