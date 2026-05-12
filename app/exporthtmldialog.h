// 文件说明：app\exporthtmldialog.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef EXPORTHTMLDIALOG_H
#define EXPORTHTMLDIALOG_H

// 包含QDialog基类头文件
#include <QDialog>

// UI命名空间，存放Qt Designer生成的界面类
namespace Ui {
class ExportHtmlDialog;
}

// 前置声明QPrinter类，避免头文件依赖
class QPrinter;

// 导出HTML对话框类，用于配置并执行HTML导出操作
class ExportHtmlDialog : public QDialog
{
    // Qt元对象宏，支持信号槽机制
    Q_OBJECT

public:
    // 构造函数，传入初始文件名和父窗口部件
    explicit ExportHtmlDialog(const QString &fileName, QWidget *parent = nullptr);
    // 析构函数，释放界面资源
    ~ExportHtmlDialog();

    // 获取导出的文件路径
    QString fileName() const;
    // 判断是否勾选包含CSS样式
    bool includeCSS() const;
    // 判断是否勾选包含代码高亮
    bool includeCodeHighlighting() const;

public slots:
    // 导出路径输入框文本变化时触发的槽函数
    void exportToTextChanged(const QString &text);
    // 选择文件按钮点击时触发的槽函数
    void chooseFileButtonClicked();

private:
    // 指向Qt Designer生成的界面对象指针
    Ui::ExportHtmlDialog *ui;
};

#endif // EXPORTHTMLDIALOG_H

