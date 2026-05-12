// 文件说明：app\aboutdialog.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "aboutdialog.h"
#include "ui_aboutdialog.h"

// 版权信息常量
static const QString COPYRIGHT = QStringLiteral("Copyright 2013-2016 Christian Loose");
// 主页链接常量
static const QString HOMEPAGE = QStringLiteral("<a href=\"http://cloose.github.io/CuteMarkEd\">http://cloose.github.io/CuteMarkEd</a>");

// 函数说明：构造 AboutDialog 对象，初始化本模块需要的状态、界面和资源。
AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    // ========== 动态填充软件名称和版本 ==========
    // 替换占位符 %1 = 应用名, %2 = 版本号
    QString appInfo = ui->messageLabel->text()
                          .arg(qApp->applicationDisplayName())
                          .arg(qApp->applicationVersion());
    ui->messageLabel->setText(appInfo);

    // ========== 填充描述信息 ==========
    QString description = QString("<p>%1<br>%2</p><p>%3</p>")
                              .arg(tr("Qt-based, free and open source markdown editor with live HTML preview"))
                              .arg(COPYRIGHT)
                              .arg(HOMEPAGE);

    ui->descriptionLabel->setText(description);
}

// 函数说明：销毁 AboutDialog 对象，释放本模块持有的资源。
AboutDialog::~AboutDialog()
{
    delete ui;
}

