// 文件说明：app\optionsdialog.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

// 包含Qt对话框基类头文件
#include <QDialog>

// UI命名空间，存放Qt Designer生成的界面类
namespace Ui {
class OptionsDialog;
}

// 前置声明配置项类
class Options;
// 前置声明代码片段集合类
class SnippetCollection;

// 设置对话框类
// 用于软件的全局配置、代理设置、快捷键管理、代码片段管理等功能
class OptionsDialog : public QDialog
{
    // Qt元对象宏，启用信号槽、属性等核心功能
    Q_OBJECT

public:
    // 构造函数
    // opt：应用配置对象  collection：代码片段集合  acts：动作列表  parent：父窗口
    OptionsDialog(Options *opt, SnippetCollection *collection, const QList<QAction*> &acts, QWidget *parent = nullptr);

    // 析构函数，释放界面及相关资源
    ~OptionsDialog();

protected:
    // 重写对话框关闭方法，关闭时执行保存/清理操作
    void done(int result);

private slots:
    // 手动代理单选框状态切换时触发
    void manualProxyRadioButtonToggled(bool checked);

    // 代码片段列表当前选中项改变时触发
    void currentSnippetChanged(const QModelIndex &current, const QModelIndex &previous);

    // 代码片段文本内容改变时触发
    void snippetTextChanged();

    // 添加代码片段按钮点击时触发
    void addSnippetButtonClicked();

    // 删除代码片段按钮点击时触发
    void removeSnippetButtonClicked();

    // 验证快捷键输入是否合法
    void validateShortcut(int row, int column);

private:
    // 初始化快捷键表格
    void setupShortcutsTable();

    // 从配置对象读取数据，更新界面显示
    void readState();

    // 将界面上的修改保存到配置对象
    void saveState();

private:
    Ui::OptionsDialog *ui;           // Qt Designer生成的界面对象
    Options *options;                 // 应用全局配置对象
    SnippetCollection *snippetCollection; // 代码片段集合对象
    QList<QAction*> actions;          // 应用的动作列表（用于快捷键管理）
};

#endif // OPTIONSDIALOG_H

