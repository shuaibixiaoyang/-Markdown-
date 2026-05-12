// 文件说明：app\exportpdfdialog.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "exportpdfdialog.h"
#include "ui_exportpdfdialog.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QPrinter>
#include <QPageSize>
#include <QPageLayout>

// 函数说明：构造 ExportPdfDialog 对象，初始化本模块需要的状态、界面和资源。
ExportPdfDialog::ExportPdfDialog(const QString &fileName, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ExportPdfDialog)
{
    ui->setupUi(this);

    // change button text of standard Ok button
    QPushButton *okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
    okButton->setText("Export PDF");

    if (!fileName.isEmpty()) {
        QFileInfo info(fileName);
        QString exportFileName = info.absoluteFilePath().replace(info.suffix(), "pdf");
        ui->exportToLineEdit->setText(exportFileName);
    }

    // fill paper size combobox
    ui->paperSizeComboBox->addItem(tr("A4 (210 x 297 mm, 8.26 x 11.69 inches)"), QPageSize::A4);
    ui->paperSizeComboBox->addItem(tr("Letter (8.5 x 11 inches, 215.9 x 279.4 mm)"), QPageSize::Letter);
    ui->paperSizeComboBox->addItem(tr("Legal (8.5 x 14 inches, 215.9 x 355.6 mm)"), QPageSize::Legal);
    ui->paperSizeComboBox->addItem(tr("A3 (297 x 420 mm)"), QPageSize::A3);
    ui->paperSizeComboBox->addItem(tr("A5 (148 x 210 mm)"), QPageSize::A5);
    ui->paperSizeComboBox->addItem(tr("A6 (105 x 148 mm)"), QPageSize::A6);
    ui->paperSizeComboBox->addItem(tr("B4 (250 x 353 mm)"), QPageSize::B4);
    ui->paperSizeComboBox->addItem(tr("B5 (176 x 250 mm, 6.93 x 9.84 inches)"), QPageSize::B5);

    // initialize Ok button state
    exportToTextChanged(fileName);
}

// 函数说明：销毁 ExportPdfDialog 对象，释放本模块持有的资源。
ExportPdfDialog::~ExportPdfDialog()
{
    delete ui;
}

// 函数说明：处理主窗口的文件菜单动作，衔接文档读写和界面状态。
QString ExportPdfDialog::fileName() const
{
    return ui->exportToLineEdit->text();
}

QPrinter *ExportPdfDialog::printer()
{
    QString fileName = ui->exportToLineEdit->text();

    QPageLayout::Orientation orientation;
    if (ui->portraitRadioButton->isChecked()) {
        orientation = QPageLayout::Portrait;
    } else {
        orientation = QPageLayout::Landscape;
    }

    QVariant v = ui->paperSizeComboBox->itemData(ui->paperSizeComboBox->currentIndex());
    QPageSize::PageSizeId sizeId = (QPageSize::PageSizeId)v.toInt();

    QPrinter *p = new QPrinter();
    p->setOutputFileName(fileName);
    p->setOutputFormat(QPrinter::PdfFormat);
    p->setPageOrientation(orientation);
    p->setPageSize(QPageSize(sizeId));

    return p;
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
void ExportPdfDialog::exportToTextChanged(const QString &text)
{
    // only enable ok button if a filename was provided
    QPushButton *okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
    okButton->setEnabled(!text.isEmpty());
}

// 函数说明：实现 ExportPdfDialog::chooseFileButtonClicked 的核心逻辑，供当前模块调用。
void ExportPdfDialog::chooseFileButtonClicked()
{
    QString fileName = ui->exportToLineEdit->text();

    fileName = QFileDialog::getSaveFileName(this, tr("Export to PDF..."), fileName,
                                                  tr("PDF Files (*.pdf);;All Files (*)"));
    if (!fileName.isEmpty()) {
        ui->exportToLineEdit->setText(fileName);
    }
}

