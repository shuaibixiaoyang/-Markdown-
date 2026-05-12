// 文件说明：app\hunspell\spellchecker_win.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "spellchecker.h"
using hunspell::SpellChecker;

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QDirIterator>

#include <spellchecker/dictionary.h>
#include <datalocation.h>

// 函数说明：实现 SpellChecker::availableDictionaries 的核心逻辑，供当前模块调用。
QMap<QString, Dictionary> SpellChecker::availableDictionaries()
{
    QMap<QString, Dictionary> dictionaries;

    QStringList paths = DataLocation::standardLocations();
    paths << qApp->applicationDirPath();

    for (const QString &path : paths) {
        QDir dictPath(path + QDir::separator() + "dictionaries");
        dictPath.setFilter(QDir::Files);
        dictPath.setNameFilters(QStringList() << "*.dic");
        if (dictPath.exists()) {
            // loop over all dictionaries in directory
            QDirIterator it(dictPath);
            while (it.hasNext()) {
                it.next();

                QString language = it.fileName().remove(".dic");
                language.truncate(5); // just language and country code

                Dictionary dict(it.fileName(), it.filePath());
                dictionaries.insert(language, dict);
            }
        }
    }

    return dictionaries;
}

