// 文件说明：app\hunspell\spellchecker.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "spellchecker.h"
using hunspell::SpellChecker;

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QStringConverter>

#include <hunspell/hunspell.hxx>

#include <spellchecker/dictionary.h>
#include <datalocation.h>

// 函数说明：构造 SpellChecker 对象，初始化本模块需要的状态、界面和资源。
SpellChecker::SpellChecker() :
    hunspellChecker(0),
    encoder(nullptr),
    decoder(nullptr)
{
}

// 函数说明：销毁 SpellChecker 对象，释放本模块持有的资源。
SpellChecker::~SpellChecker()
{
    delete hunspellChecker;
    delete encoder;
    delete decoder;
}

// 函数说明：判断 SpellChecker 当前是否满足指定状态。
bool SpellChecker::isCorrect(const QString &word)
{
#ifdef Q_OS_MAC
    if (m_useNativeChecker) {
        return nativeIsCorrect(word);
    }
#endif

    if (!encoder || !hunspellChecker) {
        return true;
    }

    QByteArray ba = encoder->encode(word);
    return hunspellChecker->spell(ba) != 0;
}

// 函数说明：实现 SpellChecker::suggestions 的核心逻辑，供当前模块调用。
QStringList SpellChecker::suggestions(const QString &word)
{
#ifdef Q_OS_MAC
    if (m_useNativeChecker) {
        return nativeSuggestions(word);
    }
#endif

    QStringList suggestions;

    if (!encoder || !decoder || !hunspellChecker) {
        return suggestions;
    }

    char **suggestedWords;
    QByteArray ba = encoder->encode(word);
    int count = hunspellChecker->suggest(&suggestedWords, ba);

    for (int i = 0; i < count; ++i) {
        suggestions << decoder->decode(QByteArray(suggestedWords[i]));
    }

    hunspellChecker->free_list(&suggestedWords, count);

    return suggestions;
}

// 函数说明：向 SpellChecker 管理的数据集合中添加一项内容。
void SpellChecker::addToUserWordlist(const QString &word)
{
    if (!encoder || !hunspellChecker) {
        return;
    }

    QByteArray encoded = encoder->encode(word);
    hunspellChecker->add(encoded.constData());
    if(!userWordlist.isEmpty()) {
        QFile userWordlistFile(userWordlist);
        if(!userWordlistFile.open(QIODevice::Append))
            return;

        QTextStream stream(&userWordlistFile);
        stream << word << "\n";
        userWordlistFile.close();
    }
}

// 函数说明：加载 SpellChecker 需要的数据、配置或外部资源。
void SpellChecker::loadDictionary(const QString &dictFilePath)
{
    delete hunspellChecker;
    hunspellChecker = nullptr;
    delete encoder;
    encoder = nullptr;
    delete decoder;
    decoder = nullptr;
    m_useNativeChecker = false;

    // 如果 filePath 为空（macOS 系统字典），使用原生拼写检查器
    if (dictFilePath.isEmpty()) {
#ifdef Q_OS_MAC
        qDebug() << "Using macOS native spell checker";
        m_useNativeChecker = true;
#endif
        return;
    }

    // 检查字典文件是否存在
    if (!QFile::exists(dictFilePath)) {
        qDebug() << "Dictionary file not found:" << dictFilePath;
#ifdef Q_OS_MAC
        // macOS 上回退到原生拼写检查器
        qDebug() << "Falling back to macOS native spell checker";
        m_useNativeChecker = true;
#endif
        return;
    }

    qDebug() << "Load dictionary from path" << dictFilePath;

    QString affixFilePath(dictFilePath);
    affixFilePath.replace(".dic", ".aff");

    hunspellChecker = new Hunspell(affixFilePath.toLocal8Bit(), dictFilePath.toLocal8Bit());

    // Get encoding from hunspell and create encoder/decoder
    const char* encoding = hunspellChecker->get_dic_encoding();
    QByteArray encodingName(encoding);

    // Try to create encoder/decoder for the dictionary encoding
    auto encodingOpt = QStringConverter::encodingForName(encodingName);
    if (encodingOpt) {
        encoder = new QStringEncoder(*encodingOpt);
        decoder = new QStringDecoder(*encodingOpt);
    } else {
        // Fallback to UTF-8
        encoder = new QStringEncoder(QStringConverter::Utf8);
        decoder = new QStringDecoder(QStringConverter::Utf8);
    }

    // also load user word list
    QString path = DataLocation::writableLocation();
    loadUserWordlist(path + "/user.dic");
}

// 函数说明：加载 SpellChecker 需要的数据、配置或外部资源。
void SpellChecker::loadUserWordlist(const QString &userWordlistPath)
{
    userWordlist = userWordlistPath;

    if (!encoder || !hunspellChecker) return;

    QFile userWordlistFile(userWordlistPath);
    if (!userWordlistFile.open(QIODevice::ReadOnly))
        return;

    QTextStream stream(&userWordlistFile);
    for (QString word = stream.readLine(); !word.isEmpty(); word = stream.readLine()) {
        QByteArray encoded = encoder->encode(word);
        hunspellChecker->add(encoded.constData());
    }
}

