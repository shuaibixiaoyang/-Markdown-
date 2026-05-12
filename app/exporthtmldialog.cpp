// 文件说明：app\exporthtmldialog.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "exporthtmldialog.h"
#include "ui_exporthtmldialog.h"

#include <QFileDialog>
#include <QFileInfo>

// 函数说明：构造 ExportHtmlDialog 对象，初始化本模块需要的状态、界面和资源。
ExportHtmlDialog::ExportHtmlDialog(const QString &fileName, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ExportHtmlDialog)
{
    ui->setupUi(this);

    // change button text of standard Ok button
    QPushButton *okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
    okButton->setText("Export HTML");

    if (!fileName.isEmpty()) {
        QFileInfo info(fileName);
        QString exportFileName = info.absoluteFilePath().replace(info.suffix(), "html");
        ui->exportToLineEdit->setText(exportFileName);
    }

    // initialize Ok button state
    exportToTextChanged(fileName);
}

// 函数说明：销毁 ExportHtmlDialog 对象，释放本模块持有的资源。
ExportHtmlDialog::~ExportHtmlDialog()
{
    delete ui;
}

// 函数说明：处理主窗口的文件菜单动作，衔接文档读写和界面状态。
QString ExportHtmlDialog::fileName() const
{
    return ui->exportToLineEdit->text();
}

// 函数说明：实现 ExportHtmlDialog::includeCSS 的核心逻辑，供当前模块调用。
bool ExportHtmlDialog::includeCSS() const
{
    return ui->styleCheckBox->isChecked();
}

// 函数说明：实现 ExportHtmlDialog::includeCodeHighlighting 的核心逻辑，供当前模块调用。
bool ExportHtmlDialog::includeCodeHighlighting() const
{
    return ui->highlightCheckBox->isChecked();
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
void ExportHtmlDialog::exportToTextChanged(const QString &text)
{
    // only enable ok button if a filename was provided
    QPushButton *okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
    okButton->setEnabled(!text.isEmpty());
}

// 函数说明：实现 ExportHtmlDialog::chooseFileButtonClicked 的核心逻辑，供当前模块调用。
void ExportHtmlDialog::chooseFileButtonClicked()
{
    QString fileName = ui->exportToLineEdit->text();

    fileName = QFileDialog::getSaveFileName(this, tr("Export to HTML..."), fileName,
                                                  tr("HTML Files (*.html *.htm);;All Files (*)"));
    if (!fileName.isEmpty()) {
        ui->exportToLineEdit->setText(fileName);
    }
}

