// 文件说明：app\imagetooldialog.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "imagetooldialog.h"
#include "ui_imagetooldialog.h"

#include <QFileDialog>
#include <QUrl>

// 函数说明：构造 ImageToolDialog 对象，初始化本模块需要的状态、界面和资源。
ImageToolDialog::ImageToolDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ImageToolDialog)
{
    ui->setupUi(this);
}

// 函数说明：销毁 ImageToolDialog 对象，释放本模块持有的资源。
ImageToolDialog::~ImageToolDialog()
{
    delete ui;
}

// 函数说明：实现 ImageToolDialog::alternateText 的核心逻辑，供当前模块调用。
QString ImageToolDialog::alternateText() const
{
    return ui->alternateTextEdit->text();
}

// 函数说明：实现 ImageToolDialog::imageSourceLink 的核心逻辑，供当前模块调用。
QString ImageToolDialog::imageSourceLink() const
{
    return ui->imageLinkEdit->text();
}

// 函数说明：实现 ImageToolDialog::optionalTitle 的核心逻辑，供当前模块调用。
QString ImageToolDialog::optionalTitle() const
{
    return ui->optionalTitleEdit->text();
}

// 函数说明：实现 ImageToolDialog::chooseFileButtonClicked 的核心逻辑，供当前模块调用。
void ImageToolDialog::chooseFileButtonClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                    QString(),
                                                    tr("Images (*.bmp *.gif *.jpg *.jpe *.jpeg *.png *.tif *.tiff *.xpm);;All Files (*)"));
    if (!fileName.isEmpty()) {
        ui->imageLinkEdit->setText(QUrl::fromLocalFile(fileName).toDisplayString());
    }
}

