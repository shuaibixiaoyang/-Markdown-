// 文件说明：app\controls\languagemenu.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "languagemenu.h" // 中文注释：引入当前文件需要的依赖头文件。

#include <QActionGroup> // 中文注释：引入当前文件需要的依赖头文件。
#include <spellchecker/dictionary.h> // 中文注释：引入当前文件需要的依赖头文件。
#include "hunspell/spellchecker.h" // 中文注释：引入当前文件需要的依赖头文件。

// 函数说明：构造 LanguageMenu 对象，初始化本模块需要的状态、界面和资源。
LanguageMenu::LanguageMenu(QWidget *parent) : // 中文注释：保留当前代码结构。
    QMenu(tr("Languages"), parent), // 中文注释：声明变量、对象或函数。
    dictionariesGroup(new QActionGroup(this)) // 中文注释：创建新的对象实例。
{ // 中文注释：进入当前代码块。
} // 中文注释：结束当前代码块。

// 函数说明：加载 LanguageMenu 需要的数据、配置或外部资源。
void LanguageMenu::loadDictionaries(const QString &currentLanguage) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    QMap<QString, Dictionary> dictionaries = hunspell::SpellChecker::availableDictionaries(); // 中文注释：声明变量、对象或函数。

    if (dictionaries.isEmpty()) { // 中文注释：判断条件是否成立。
        QAction *noDict = this->addAction(tr("No dictionaries available")); // 中文注释：声明变量、对象或函数。
        noDict->setEnabled(false); // 中文注释：执行当前语句。
        return; // 中文注释：返回函数处理结果。
    } // 中文注释：结束当前代码块。

    QMapIterator<QString, Dictionary> it(dictionaries); // 中文注释：声明变量、对象或函数。
    while (it.hasNext()) { // 中文注释：按条件持续循环处理。
        it.next(); // 中文注释：执行当前语句。

        Dictionary dictionary = it.value(); // 中文注释：声明变量、对象或函数。

        // create an action for the dictionary
        QAction *action = createAction(dictionary); // 中文注释：声明变量、对象或函数。

        if (dictionary.language() == currentLanguage) { // 中文注释：判断条件是否成立。
            action->setChecked(true); // 中文注释：执行当前语句。
            action->trigger(); // 中文注释：执行当前语句。
        } // 中文注释：结束当前代码块。
    } // 中文注释：结束当前代码块。
} // 中文注释：结束当前代码块。

// 函数说明：实现 LanguageMenu::languageTriggered 的核心逻辑，供当前模块调用。
void LanguageMenu::languageTriggered() // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    QAction *action = qobject_cast<QAction*>(sender()); // 中文注释：声明变量、对象或函数。
    emit languageTriggered(action->data().value<Dictionary>()); // 中文注释：发出 Qt 信号通知外部对象。
} // 中文注释：结束当前代码块。

QAction *LanguageMenu::createAction(const Dictionary &dictionary) // 中文注释：声明变量、对象或函数。
{ // 中文注释：进入当前代码块。
    QAction *action = this->addAction(QString("%1 / %2").arg(dictionary.languageName()).arg(dictionary.countryName())); // 中文注释：声明变量、对象或函数。
    connect(action, &QAction::triggered, this, QOverload<>::of(&LanguageMenu::languageTriggered)); // 中文注释：连接信号与槽，建立事件响应关系。
    action->setCheckable(true); // 中文注释：执行当前语句。
    action->setActionGroup(dictionariesGroup); // 中文注释：执行当前语句。

    QVariant data; // 中文注释：声明变量、对象或函数。
    data.setValue(dictionary); // 中文注释：执行当前语句。
    action->setData(data); // 中文注释：执行当前语句。

    return action; // 中文注释：返回函数处理结果。
} // 中文注释：结束当前代码块。

