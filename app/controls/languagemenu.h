// 文件说明：app\controls\languagemenu.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef LANGUAGEMENU_H // 中文注释：开始头文件保护，避免重复包含。
#define LANGUAGEMENU_H // 中文注释：定义头文件保护宏。

#include <QMenu> // 中文注释：引入当前文件需要的依赖头文件。

class Dictionary;  // 前向声明字典类 // 中文注释：声明类类型或前置声明类。

/**
 * 语言选择菜单类
 *
 * 继承自QMenu，提供语言字典的选择功能。
 * 菜单项以单选按钮组的形式展示，支持当前语言的高亮显示。
 */
class LanguageMenu : public QMenu // 中文注释：声明类类型或前置声明类。
{ // 中文注释：进入当前代码块。
    Q_OBJECT // 中文注释：启用 Qt 元对象系统能力。

public: // 中文注释：声明类成员的访问权限区域。
    /**
     * 构造函数
     * parent 父窗口指针，默认为nullptr
     */
    explicit LanguageMenu(QWidget *parent = nullptr); // 中文注释：声明变量、对象或函数。

    /**
     * 加载可用字典列表
     * currentLanguage 当前使用的语言，用于高亮对应的菜单项
     *
     * 根据字典集合动态创建菜单项，并设置当前语言的选中状态
     */
    void loadDictionaries(const QString &currentLanguage); // 中文注释：声明变量、对象或函数。

signals: // 中文注释：声明 Qt 信号区域。
    /**
     * 语言选择信号
     * dictionary 被选中的字典对象
     *
     * 当用户从菜单中选择语言时发射，携带对应的字典信息
     */
    void languageTriggered(const Dictionary &dictionary); // 中文注释：声明变量、对象或函数。

private slots: // 中文注释：声明 Qt 槽函数区域。
    /**
     * 语言触发的内部槽函数
     * 将QAction的触发信号转换为带Dictionary对象的信号
     */
    void languageTriggered(); // 中文注释：声明变量、对象或函数。

private: // 中文注释：声明类成员的访问权限区域。
    /**
     * 创建菜单项
     * dictionary 字典对象
     * 返回 创建的QAction对象
     *
     * 根据字典信息创建对应的菜单项，并设置相关属性
     */
    QAction *createAction(const Dictionary &dictionary); // 中文注释：声明变量、对象或函数。

private: // 中文注释：声明类成员的访问权限区域。
    QActionGroup *dictionariesGroup;  // 菜单项组，确保语言选项为单选 // 中文注释：声明变量、对象或函数。
}; // 中文注释：结束类型或作用域声明。

#endif // LANGUAGEMENU_H // 中文注释：结束条件编译或头文件保护。

