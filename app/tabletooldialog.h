// 文件说明：app\tabletooldialog.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef TABLETOOLDIALOG_H
#define TABLETOOLDIALOG_H

// 包含Qt对话框基类头文件
#include <QDialog>
// 包含Qt列表容器
#include <QList>
// 包含Qt键值对映射容器
#include <QMap>

// UI命名空间，存放Qt Designer生成的界面类
namespace Ui {
class TableToolDialog;
}

// 前置声明下拉选择框类
class QComboBox;
// 前置声明单行输入框类
class QLineEdit;

// 表格工具对话框类
// 用于可视化配置表格参数：行列数、对齐方式、单元格内容
class TableToolDialog : public QDialog
{
    // Qt元对象宏，支持信号槽机制
    Q_OBJECT

public:
    // 构造函数，parent为父窗口部件
    explicit TableToolDialog(QWidget *parent = nullptr);
    // 析构函数，释放界面及动态创建的控件
    ~TableToolDialog();

    // 获取用户设置的表格行数
    int rows() const;
    // 获取用户设置的表格列数
    int columns() const;

    // 获取每列的对齐方式列表
    QList<Qt::Alignment> alignments() const;
    // 获取所有单元格的内容集合
    QList<QStringList> tableCells() const;

private slots:
    // 表格行列数发生改变时触发
    void tableSizeChanged();

private:
    // 动态添加指定数量的列
    void addColumns(int newColumns);
    // 动态移除指定数量的列
    void removeColumns(int removedColumns);
    // 动态添加指定数量的行
    void addRows(int newRows);
    // 动态移除指定数量的行
    void removeRows(int removedRows);

    // 更新控件的Tab切换顺序
    void updateTabOrder();

private:
    Ui::TableToolDialog *ui;           // Qt Designer生成的界面对象
    int previousRowCount;              // 记录上一次的行数
    int previousColumnCount;          // 记录上一次的列数
    QList<QComboBox *> alignmentComboBoxList;  // 存储列对齐方式下拉框
    QMap<QPoint, QLineEdit *> cellEditorMap;   // 存储单元格输入框，坐标为键
};

#endif // TABLETOOLDIALOG_H

