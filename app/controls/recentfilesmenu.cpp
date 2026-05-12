// 文件说明：app\controls\recentfilesmenu.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "recentfilesmenu.h" // 中文注释：引入当前文件需要的依赖头文件。

#include <QFileInfo> // 中文注释：引入当前文件需要的依赖头文件。
#include <QDir> // 中文注释：引入当前文件需要的依赖头文件。
#include <QSettings> // 中文注释：引入当前文件需要的依赖头文件。

// 函数说明：构造 RecentFilesMenu 对象，初始化本模块需要的状态、界面和资源。
RecentFilesMenu::RecentFilesMenu(QWidget *parent) : // 中文注释：保留当前代码结构。
    QMenu(tr("Recent &Files"), parent) // 中文注释：声明变量、对象或函数。
{ // 中文注释：进入当前代码块。
} // 中文注释：结束当前代码块。

// 函数说明：读取 RecentFilesMenu 的配置或数据，并同步到运行时状态。
void RecentFilesMenu::readState() // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    QSettings settings; // 中文注释：声明变量、对象或函数。

    int size = settings.beginReadArray("recentFiles"); // 中文注释：声明变量、对象或函数。
    for (int i = 0; i < size; ++i) { // 中文注释：开始循环遍历数据。
        settings.setArrayIndex(i); // 中文注释：执行当前语句。
        recentFiles << settings.value("fileName").toString(); // 中文注释：执行当前语句。
    } // 中文注释：结束当前代码块。
    settings.endArray(); // 中文注释：执行当前语句。

    updateMenu(); // 中文注释：执行当前语句。
} // 中文注释：结束当前代码块。

// 函数说明：保存 RecentFilesMenu 当前状态，保证用户修改可以持久化。
void RecentFilesMenu::saveState() const // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    QSettings settings; // 中文注释：声明变量、对象或函数。

    settings.beginWriteArray("recentFiles"); // 中文注释：执行当前语句。
    for (int i = 0; i < recentFiles.size(); ++i) { // 中文注释：开始循环遍历数据。
        settings.setArrayIndex(i); // 中文注释：执行当前语句。
        settings.setValue("fileName", recentFiles.at(i)); // 中文注释：执行当前语句。
    } // 中文注释：结束当前代码块。
    settings.endArray(); // 中文注释：执行当前语句。
} // 中文注释：结束当前代码块。

// 函数说明：向 RecentFilesMenu 管理的数据集合中添加一项内容。
void RecentFilesMenu::addFile(const QString &fileName) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    QFileInfo fileInfo(fileName); // 中文注释：声明变量、对象或函数。
    QString absoluteNativeFileName(QDir::toNativeSeparators(fileInfo.absoluteFilePath())); // 中文注释：声明变量、对象或函数。

    // add file to top of list
    recentFiles.removeAll(absoluteNativeFileName); // 中文注释：执行当前语句。
    recentFiles.prepend(absoluteNativeFileName); // 中文注释：执行当前语句。

    // remove last entry if list contains more than 10 entries
    if (recentFiles.size() > 10) { // 中文注释：判断条件是否成立。
        recentFiles.removeLast(); // 中文注释：执行当前语句。
    } // 中文注释：结束当前代码块。

    updateMenu(); // 中文注释：执行当前语句。
} // 中文注释：结束当前代码块。

// 函数说明：清空 RecentFilesMenu 保存的临时状态或缓存数据。
void RecentFilesMenu::clearMenu() // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    recentFiles.clear(); // 中文注释：执行当前语句。
    updateMenu(); // 中文注释：执行当前语句。
} // 中文注释：结束当前代码块。

// 函数说明：实现 RecentFilesMenu::recentFileTriggered 的核心逻辑，供当前模块调用。
void RecentFilesMenu::recentFileTriggered() // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    QAction *action = qobject_cast<QAction*>(sender()); // 中文注释：声明变量、对象或函数。
    emit recentFileTriggered(action->data().toString()); // 中文注释：发出 Qt 信号通知外部对象。
} // 中文注释：结束当前代码块。

// 函数说明：刷新 RecentFilesMenu 的内部状态，并同步到相关界面。
void RecentFilesMenu::updateMenu() // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    clear(); // 中文注释：执行当前语句。

    for (const QString &recentFile : recentFiles) { // 中文注释：开始循环遍历数据。
        QAction *action = addAction(recentFile); // 中文注释：声明变量、对象或函数。
        action->setData(recentFile); // 中文注释：执行当前语句。

        connect(action, &QAction::triggered, // 中文注释：连接信号与槽，建立事件响应关系。
                this, QOverload<>::of(&RecentFilesMenu::recentFileTriggered)); // 中文注释：执行当前语句。
    } // 中文注释：结束当前代码块。

    addSeparator(); // 中文注释：执行当前语句。
    addAction(tr("Clear Menu"), this, &RecentFilesMenu::clearMenu); // 中文注释：执行当前语句。

    setEnabled(!recentFiles.empty()); // 中文注释：执行当前语句。
} // 中文注释：结束当前代码块。

