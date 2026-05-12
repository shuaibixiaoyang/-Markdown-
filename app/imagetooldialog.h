// 文件说明：app\imagetooldialog.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef IMAGETOOLDIALOG_H
#define IMAGETOOLDIALOG_H

// 包含对话框基类头文件
#include <QDialog>

// UI命名空间，存放Qt Designer生成的界面类
namespace Ui {
class ImageToolDialog;
}

// 图片工具对话框类，用于配置插入图片的相关参数
class ImageToolDialog : public QDialog
{
    // Qt元对象宏，支持信号槽机制
    Q_OBJECT

public:
    // 构造函数，parent为父窗口部件
    explicit ImageToolDialog(QWidget *parent = nullptr);
    // 析构函数，释放界面资源
    ~ImageToolDialog();

    // 获取替换文本（alt属性）
    QString alternateText() const;
    // 获取图片源路径/链接（src属性）
    QString imageSourceLink() const;
    // 获取可选标题（title属性）
    QString optionalTitle() const;

private slots:
    // 选择文件按钮点击时触发的槽函数
    void chooseFileButtonClicked();

private:
    // 指向Qt Designer生成的界面对象
    Ui::ImageToolDialog *ui;
};

#endif // IMAGETOOLDIALOG_H

