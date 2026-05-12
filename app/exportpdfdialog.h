// 文件说明：app\exportpdfdialog.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef EXPORTPDFDIALOG_H
#define EXPORTPDFDIALOG_H

// 包含QDialog基类头文件
#include <QDialog>

// UI命名空间，存放Qt设计师生成的界面类
namespace Ui {
class ExportPdfDialog;
}

// 前置声明打印类，避免头文件直接包含
class QPrinter;

// 导出PDF对话框类，用于配置PDF导出相关参数
class ExportPdfDialog : public QDialog
{
    // Qt元对象宏，支持信号槽、属性等功能
    Q_OBJECT

public:
    // 构造函数，传入初始文件名和父窗口部件
    explicit ExportPdfDialog(const QString &fileName, QWidget *parent = nullptr);
    // 析构函数，释放界面对象资源
    ~ExportPdfDialog();

    // 获取用户设置的导出文件路径
    QString fileName() const;
    // 获取打印设备对象，用于执行PDF打印/导出
    QPrinter *printer();

public slots:
    // 导出路径输入框文本改变时触发
    void exportToTextChanged(const QString &text);
    // 选择文件按钮被点击时触发
    void chooseFileButtonClicked();

private:
    // 指向Qt设计师生成的界面对象
    Ui::ExportPdfDialog *ui;
};

#endif // EXPORTPDFDIALOG_H

