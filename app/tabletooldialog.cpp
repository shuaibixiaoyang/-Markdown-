// 文件说明：app\tabletooldialog.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "tabletooldialog.h"
#include "ui_tabletooldialog.h"

#include <QComboBox>
#include <QLineEdit>

// Make QPoint in QMap work
bool operator<(const QPoint& lhs, const QPoint& rhs)
{
    if (lhs.x() < rhs.x())
        return true;
    else if (lhs.x() == rhs.x())
        return lhs.y() < rhs.y();
    else
        return false;
}

// 函数说明：构造 TableToolDialog 对象，初始化本模块需要的状态、界面和资源。
TableToolDialog::TableToolDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TableToolDialog),
    previousRowCount(0),
    previousColumnCount(0)
{
    ui->setupUi(this);
    tableSizeChanged();
}

// 函数说明：销毁 TableToolDialog 对象，释放本模块持有的资源。
TableToolDialog::~TableToolDialog()
{
    delete ui;
}

// 函数说明：实现 TableToolDialog::rows 的核心逻辑，供当前模块调用。
int TableToolDialog::rows() const
{
    return ui->rowsSpinBox->value();
}

// 函数说明：实现 TableToolDialog::columns 的核心逻辑，供当前模块调用。
int TableToolDialog::columns() const
{
    return ui->columnsSpinBox->value();
}

// 函数说明：实现 TableToolDialog::alignments 的核心逻辑，供当前模块调用。
QList<Qt::Alignment> TableToolDialog::alignments() const
{
    QList<Qt::Alignment> alignments;

    for (QComboBox *cb : alignmentComboBoxList) {
        Qt::Alignment alignment = (Qt::Alignment)cb->itemData(cb->currentIndex()).toInt();
        alignments.append(alignment);
    }

    return alignments;
}

// 函数说明：实现 TableToolDialog::tableCells 的核心逻辑，供当前模块调用。
QList<QStringList> TableToolDialog::tableCells() const
{
    QList<QStringList> table;

    for (int row = 0; row < rows(); ++row) {
        QStringList rowData;

        for (int col = 0; col < columns(); ++col) {
            rowData << cellEditorMap[QPoint(col, row)]->text();
        }

        table.append(rowData);
    }

    return table;
}

// 函数说明：实现 TableToolDialog::tableSizeChanged 的核心逻辑，供当前模块调用。
void TableToolDialog::tableSizeChanged()
{
    int rowDiff = rows() - previousRowCount;
    int columnDiff = columns() - previousColumnCount;

    if (columnDiff > 0) {
        addColumns(columnDiff);
    } else if (columnDiff < 0) {
        removeColumns(columnDiff);
    } else if (rowDiff > 0) {
        addRows(rowDiff);
    } else if (rowDiff < 0) {
        removeRows(rowDiff);
    }

    updateTabOrder();

    previousColumnCount = columns();
    previousRowCount = rows();
}

// 函数说明：向 TableToolDialog 管理的数据集合中添加一项内容。
void TableToolDialog::addColumns(int newColumns)
{
    for (int col = 0; col < newColumns; ++col) {
        // add combo box to choose alignment of column
        QComboBox *cb = new QComboBox(ui->tableGroupBox);
        cb->addItem(tr("Left"), Qt::AlignLeft);
        cb->addItem(tr("Center"), Qt::AlignCenter);
        cb->addItem(tr("Right"), Qt::AlignRight);
        ui->tableGridLayout->addWidget(cb, 0, col+previousColumnCount+1);

        alignmentComboBoxList << cb;

        // add a column of new line edits
        for (int row = 0; row < rows(); ++row) {
            QLineEdit *edit = new QLineEdit(ui->tableGroupBox);
            ui->tableGridLayout->addWidget(edit, row+1, col+previousColumnCount+1);

            cellEditorMap.insert(QPoint(col+previousColumnCount, row), edit);
        }
    }
}

// 函数说明：从 TableToolDialog 管理的数据集合中移除指定内容。
void TableToolDialog::removeColumns(int removedColumns)
{
    for (int col = 0; col < qAbs(removedColumns); ++col) {
        // remove alignment combo box in last column
        QComboBox *cb = alignmentComboBoxList.last();

        alignmentComboBoxList.removeLast();
        ui->tableGridLayout->removeWidget(cb);

        cb->deleteLater();

        // remove all line edits in last column
        for (int row = 0; row < rows(); ++row) {
            QLineEdit *edit = cellEditorMap[QPoint(previousColumnCount-col-1, row)];

            cellEditorMap.remove(QPoint(previousColumnCount-col-1, row));
            ui->tableGridLayout->removeWidget(edit);

            edit->deleteLater();
        }
    }
}

// 函数说明：向 TableToolDialog 管理的数据集合中添加一项内容。
void TableToolDialog::addRows(int newRows)
{
    for (int row = 0; row < newRows; ++row) {
        // add a new row of line edits
        for (int col = 0; col < columns(); ++col) {
            QLineEdit *edit = new QLineEdit(ui->tableGroupBox);
            ui->tableGridLayout->addWidget(edit, row+previousRowCount+1, col+1);

            cellEditorMap.insert(QPoint(col, row+previousRowCount), edit);
        }
    }
}

// 函数说明：从 TableToolDialog 管理的数据集合中移除指定内容。
void TableToolDialog::removeRows(int removedRows)
{
    for (int row = 0; row < qAbs(removedRows); ++row) {
        // remove all line edits in current row
        for (int col = 0; col < columns(); ++col) {
            QLineEdit *edit = cellEditorMap[QPoint(col, previousRowCount-row-1)];

            cellEditorMap.remove(QPoint(col, previousRowCount-row-1));
            ui->tableGridLayout->removeWidget(edit);

            edit->deleteLater();
        }
    }
}

// 函数说明：刷新 TableToolDialog 的内部状态，并同步到相关界面。
void TableToolDialog::updateTabOrder()
{
    // tab between spin boxes
    this->setTabOrder(ui->rowsSpinBox, ui->columnsSpinBox);

    QWidget *first = ui->columnsSpinBox;
    for (QComboBox *cb : alignmentComboBoxList) {
        this->setTabOrder(first, cb);
        first = cb;
    }

    // tab between line edits from left-to-right
    for (int row = 0; row < rows(); ++row) {
        for (int col = 0; col < columns(); ++col) {
            this->setTabOrder(first, cellEditorMap[QPoint(col, row)]);
            first = cellEditorMap[QPoint(col, row)];
        }
    }

    // tab between last line edit and okay button
    this->setTabOrder(first, ui->buttonBox);
}

