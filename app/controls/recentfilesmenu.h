// 文件说明：app\controls\recentfilesmenu.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef RECENTFILESMENU_H // 中文注释：开始头文件保护，避免重复包含。
#define RECENTFILESMENU_H // 中文注释：定义头文件保护宏。

#include <QMenu> // 中文注释：引入当前文件需要的依赖头文件。

/**
 * 最近文件菜单类
 *
 * 继承自QMenu，提供最近打开文件的记录和管理功能。
 * 支持文件的添加、清除以及持久化保存最近文件列表。
 */
class RecentFilesMenu : public QMenu // 中文注释：声明类类型或前置声明类。
{ // 中文注释：进入当前代码块。
    Q_OBJECT // 中文注释：启用 Qt 元对象系统能力。

public: // 中文注释：声明类成员的访问权限区域。
    /**
     * 构造函数
     * parent 父窗口指针，默认为nullptr
     */
    explicit RecentFilesMenu(QWidget *parent = nullptr); // 中文注释：声明变量、对象或函数。

    /**
     * 读取状态
     * 从配置文件或注册表中加载最近文件列表
     */
    void readState(); // 中文注释：声明变量、对象或函数。

    /**
     * 保存状态
     * 将最近文件列表持久化存储到配置文件或注册表
     */
    void saveState() const; // 中文注释：声明变量、对象或函数。

signals: // 中文注释：声明 Qt 信号区域。
    /**
     * 最近文件触发信号
     * fileName 被选中的文件路径
     *
     * 当用户从菜单中选择最近文件时发射
     */
    void recentFileTriggered(const QString &fileName); // 中文注释：声明变量、对象或函数。

public slots: // 中文注释：声明 Qt 槽函数区域。
    /**
     * 添加文件到最近列表
     * fileName 要添加的文件路径
     *
     * 将文件添加到列表头部，自动去重并保持最大数量限制
     */
    void addFile(const QString &fileName); // 中文注释：声明变量、对象或函数。

    /**
     * 清空最近文件菜单
     * 移除所有最近文件记录
     */
    void clearMenu(); // 中文注释：声明变量、对象或函数。

private slots: // 中文注释：声明 Qt 槽函数区域。
    /**
     * 最近文件触发的内部槽函数
     * 将QAction的触发信号转换为带文件路径的信号
     */
    void recentFileTriggered(); // 中文注释：声明变量、对象或函数。

private: // 中文注释：声明类成员的访问权限区域。
    /**
     * 更新菜单显示
     * 根据最近文件列表重新构建菜单项
     */
    void updateMenu(); // 中文注释：声明变量、对象或函数。

private: // 中文注释：声明类成员的访问权限区域。
    QStringList recentFiles;  // 最近文件路径列表 // 中文注释：声明变量、对象或函数。
}; // 中文注释：结束类型或作用域声明。

#endif // RECENTFILESMENU_H // 中文注释：结束条件编译或头文件保护。

