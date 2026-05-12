// 文件说明：app\controls\activelabel.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "activelabel.h" // 中文注释：引入当前文件需要的依赖头文件。

#include <QAction> // 中文注释：引入当前文件需要的依赖头文件。
#include <QMouseEvent> // 中文注释：引入当前文件需要的依赖头文件。

// 函数说明：构造 ActiveLabel 对象，初始化本模块需要的状态、界面和资源。
ActiveLabel::ActiveLabel(QWidget *parent) : // 中文注释：保留当前代码结构。
    QLabel(parent), // 中文注释：声明变量、对象或函数。
    m_action(0) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
} // 中文注释：结束当前代码块。

// 函数说明：构造 ActiveLabel 对象，初始化本模块需要的状态、界面和资源。
ActiveLabel::ActiveLabel(const QString &text, QWidget *parent) : // 中文注释：保留当前代码结构。
    QLabel(text, parent), // 中文注释：声明变量、对象或函数。
    m_action(0) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
} // 中文注释：结束当前代码块。

// 函数说明：处理鼠标事件，更新选择、拖拽或交互状态。
void ActiveLabel::mouseDoubleClickEvent(QMouseEvent *e) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    // double click with left mouse button?
    if (e->button() == Qt::LeftButton) { // 中文注释：判断条件是否成立。
        emit doubleClicked(); // 中文注释：发出 Qt 信号通知外部对象。
    } // 中文注释：结束当前代码块。

    QLabel::mouseDoubleClickEvent(e); // 中文注释：声明变量、对象或函数。
} // 中文注释：结束当前代码块。

// 函数说明：设置 ActiveLabel 的运行参数，并触发必要的界面或数据刷新。
void ActiveLabel::setAction(QAction *action) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    // if was previously defined, disconnect
    if (m_action) { // 中文注释：判断条件是否成立。
        disconnect(m_action, &QAction::changed, this, &ActiveLabel::updateFromAction); // 中文注释：断开已有信号槽连接。
        disconnect(this, &ActiveLabel::doubleClicked, m_action, &QAction::trigger); // 中文注释：断开已有信号槽连接。
    } // 中文注释：结束当前代码块。

    // set new action
    m_action = action; // 中文注释：更新变量或对象状态。

    // update label using action's data
    updateFromAction(); // 中文注释：执行当前语句。

    // action action and label to have them synced
    // whenever one of them is triggered
    connect(m_action, &QAction::changed, this, &ActiveLabel::updateFromAction); // 中文注释：连接信号与槽，建立事件响应关系。
    connect(this, &ActiveLabel::doubleClicked, m_action, &QAction::trigger); // 中文注释：连接信号与槽，建立事件响应关系。
} // 中文注释：结束当前代码块。

// 函数说明：刷新 ActiveLabel 的内部状态，并同步到相关界面。
void ActiveLabel::updateFromAction() // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    setStatusTip(m_action->statusTip()); // 中文注释：执行当前语句。
    setToolTip(m_action->toolTip()); // 中文注释：执行当前语句。
    setEnabled(m_action->isEnabled()); // 中文注释：执行当前语句。

    // update text based on QAction data
    QString actionText = m_action->text(); // 中文注释：声明变量、对象或函数。
    actionText.remove("&"); // 中文注释：执行当前语句。

    if (m_action->isCheckable()) { // 中文注释：判断条件是否成立。
        if (m_action->isChecked()) // 中文注释：判断条件是否成立。
            setText(QString("%1: %2").arg(actionText).arg(tr("on"))); // 中文注释：执行当前语句。
        else // 中文注释：处理条件不成立时的逻辑。
            setText(QString("%1: %2").arg(actionText).arg(tr("off"))); // 中文注释：执行当前语句。
    } else { // 中文注释：结束当前代码块或分支。
        setText(actionText); // 中文注释：执行当前语句。
    } // 中文注释：结束当前代码块。
} // 中文注释：结束当前代码块。

