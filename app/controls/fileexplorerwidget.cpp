// 文件说明：app\controls\fileexplorerwidget.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "fileexplorerwidget.h" // 中文注释：引入当前文件需要的依赖头文件。
#include "ui_fileexplorerwidget.h" // 中文注释：引入当前文件需要的依赖头文件。

#include <QFileSystemModel> // 中文注释：引入当前文件需要的依赖头文件。
#include <QSortFilterProxyModel> // 中文注释：引入当前文件需要的依赖头文件。

class FileSortFilterProxyModel : public QSortFilterProxyModel // 中文注释：声明类类型或前置声明类。
{ // 中文注释：进入当前代码块。
public: // 中文注释：声明类成员的访问权限区域。
    FileSortFilterProxyModel(QObject *parent = 0) : QSortFilterProxyModel(parent) {} // 中文注释：更新变量或对象状态。

protected: // 中文注释：声明类成员的访问权限区域。
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const // 中文注释：声明变量、对象或函数。
    { // 中文注释：进入当前代码块。
        QFileSystemModel *model = static_cast<QFileSystemModel*>(this->sourceModel()); // 中文注释：声明变量、对象或函数。

        QFileInfo leftInfo  = model->fileInfo(left); // 中文注释：声明变量、对象或函数。
        QFileInfo rightInfo = model->fileInfo(right); // 中文注释：声明变量、对象或函数。

        if (leftInfo.isDir() == rightInfo.isDir()) // 中文注释：判断条件是否成立。
            return (leftInfo.filePath().compare(rightInfo.filePath(), Qt::CaseInsensitive) < 0); // 中文注释：返回函数处理结果。

        return leftInfo.isDir(); // 中文注释：返回函数处理结果。
    } // 中文注释：结束当前代码块。
}; // 中文注释：结束类型或作用域声明。


// 函数说明：构造 FileExplorerWidget 对象，初始化本模块需要的状态、界面和资源。
FileExplorerWidget::FileExplorerWidget(QWidget *parent) : // 中文注释：保留当前代码结构。
    QWidget(parent), // 中文注释：声明变量、对象或函数。
    initialized(false), // 中文注释：继续传入下一项参数。
    ui(new Ui::FileExplorerWidget), // 中文注释：创建新的对象实例。
    model(new QFileSystemModel(this)), // 中文注释：创建新的对象实例。
    sortModel(new FileSortFilterProxyModel(this)) // 中文注释：创建新的对象实例。
{ // 中文注释：进入当前代码块。
    ui->setupUi(this); // 中文注释：执行当前语句。

    sortModel->setDynamicSortFilter(true); // 中文注释：执行当前语句。
    sortModel->setSourceModel(model); // 中文注释：执行当前语句。

    ui->fileTreeView->setModel(sortModel); // 中文注释：执行当前语句。
    ui->fileTreeView->hideColumn(1); // 中文注释：执行当前语句。
    ui->fileTreeView->sortByColumn(0, Qt::AscendingOrder); // 中文注释：执行当前语句。

    connect(ui->fileTreeView, &QTreeView::doubleClicked, // 中文注释：连接信号与槽，建立事件响应关系。
            this, &FileExplorerWidget::fileOpen); // 中文注释：执行当前语句。
} // 中文注释：结束当前代码块。

// 函数说明：销毁 FileExplorerWidget 对象，释放本模块持有的资源。
FileExplorerWidget::~FileExplorerWidget() // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    delete ui; // 中文注释：释放动态创建的对象资源。
} // 中文注释：结束当前代码块。

// 函数说明：显示 FileExplorerWidget 管理的面板、对话框或提示信息。
void FileExplorerWidget::showEvent(QShowEvent *event) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    if (!initialized) { // 中文注释：判断条件是否成立。
        model->setRootPath(""); // 中文注释：执行当前语句。
        initialized = true; // 中文注释：更新变量或对象状态。
    } // 中文注释：结束当前代码块。
    QWidget::showEvent(event); // 中文注释：声明变量、对象或函数。
} // 中文注释：结束当前代码块。

// 函数说明：处理主窗口的文件菜单动作，衔接文档读写和界面状态。
void FileExplorerWidget::fileOpen(const QModelIndex &index) // 中文注释：保留当前代码结构。
{ // 中文注释：进入当前代码块。
    QFileInfo info = model->fileInfo(sortModel->mapToSource(index)); // 中文注释：声明变量、对象或函数。
    if (info.isFile()) { // 中文注释：判断条件是否成立。
        const QString filePath = info.filePath(); // 中文注释：声明变量、对象或函数。

        emit fileSelected(filePath); // 中文注释：发出 Qt 信号通知外部对象。
    } // 中文注释：结束当前代码块。
} // 中文注释：结束当前代码块。

