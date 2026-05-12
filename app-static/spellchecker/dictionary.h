// 文件说明：app-static\spellchecker\dictionary.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef DICTIONARY_H
#define DICTIONARY_H

#include <QtCore/qmetatype.h>
#include <QtCore/qstring.h>

class Dictionary
{
public:
    Dictionary();
    Dictionary(const QString &language, const QString &filePath);
    Dictionary(const Dictionary &other);
    ~Dictionary();

    QString language() const;
    QString languageName() const;

    QString countryName() const;

    QString filePath() const;

private:
    QString m_language;
    QString m_filePath;
};

Q_DECLARE_METATYPE(Dictionary);

#endif // DICTIONARY_H

