// 文件说明：app\main.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。

/*
这段代码是Markdown编辑器CuteMarkEd的Qt程序入口，核心实现跨平台初始化与适配：
1. **Windows专属**：通过QSettings操作注册表，注册.md/.markdown等文件类型，关联程序打开方式，让系统跳转列表显示最近文件；
2. **通用逻辑**：初始化Qt应用，设置组织/应用名等元信息，加载Qt自身和应用的国际化翻译文件，适配多语言；
3. **命令行解析**：支持--help/--version参数，可接收启动时要打开的文件路径；
4. **平台适配**：macOS下安装事件过滤器，处理Finder双击文件打开逻辑；
5. 最后创建主窗口并启动Qt事件循环，支撑程序交互运行。
*/
#include "mainwindow.h" // 中文注释：引入当前文件需要的依赖头文件。

#include <QApplication> // 中文注释：引入当前文件需要的依赖头文件。
#include <QCommandLineParser> // 中文注释：引入当前文件需要的依赖头文件。
#include <QLibraryInfo> // 中文注释：引入当前文件需要的依赖头文件。
#include <QTranslator> // 中文注释：引入当前文件需要的依赖头文件。
#include <QFileOpenEvent> // 中文注释：引入当前文件需要的依赖头文件。

#ifdef Q_OS_MAC // 中文注释：根据编译条件启用对应代码。
// macOS 文件打开事件过滤器
// 当用户从 Finder 双击 .md 文件时，macOS 通过 QFileOpenEvent 通知应用
class FileOpenEventFilter : public QObject // 中文注释：声明类类型或前置声明类。
{ // 中文注释：进入当前代码块。
public: // 中文注释：声明类成员的访问权限区域。
    FileOpenEventFilter(MainWindow *window, QObject *parent = nullptr) // 中文注释：更新变量或对象状态。
        : QObject(parent), m_window(window) {} // 中文注释：保留当前代码结构。

protected: // 中文注释：声明类成员的访问权限区域。
    bool eventFilter(QObject *obj, QEvent *event) override // 中文注释：声明变量、对象或函数。
    { // 中文注释：进入当前代码块。
        if (event->type() == QEvent::FileOpen) { // 中文注释：判断条件是否成立。
            QFileOpenEvent *fileEvent = static_cast<QFileOpenEvent *>(event); // 中文注释：声明变量、对象或函数。
            if (!fileEvent->file().isEmpty() && m_window) { // 中文注释：判断条件是否成立。
                m_window->openFile(fileEvent->file()); // 中文注释：执行当前语句。
                return true; // 中文注释：返回函数处理结果。
            } // 中文注释：结束当前代码块。
        } // 中文注释：结束当前代码块。
        return QObject::eventFilter(obj, event); // 中文注释：返回函数处理结果。
    } // 中文注释：结束当前代码块。

private: // 中文注释：声明类成员的访问权限区域。
    MainWindow *m_window; // 中文注释：执行当前语句。
}; // 中文注释：结束类型或作用域声明。
#endif // 中文注释：结束条件编译或头文件保护。

#ifdef Q_OS_WIN // 中文注释：根据编译条件启用对应代码。
#include <QDir>//路径处理相关类
#include <QFileInfo>//文件信息解析类
#include <QSettings>//Qt配置文件操作类

// Helper function to register supported file types
// This is needed to enable the application jump list to show the desired recent files
/**
 * @brief 辅助函数：注册支持的文件类型到 Windows 注册表
 * @param fileTypes 要关联的文件后缀列表（如 .md、.markdown）
 * @note 该函数的核心目的是让 Windows 应用跳转列表（Jump List）能识别并显示最近打开的指定类型文件
 *       同时也会配置这些文件类型的默认打开程序为当前应用
 */
static void associateFileTypes(const QStringList &fileTypes) // 中文注释：声明变量、对象或函数。
{ // 中文注释：进入当前代码块。
    //获取应用的显示名称
    QString displayName = QGuiApplication::applicationDisplayName(); // 中文注释：声明变量、对象或函数。
    //获取当前应用可执行文件的完整路径
    QString filePath = QCoreApplication::applicationFilePath(); // 中文注释：声明变量、对象或函数。
    //从完整路径中提取可执行文件名
    QString fileName = QFileInfo(filePath).fileName(); // 中文注释：声明变量、对象或函数。
    // 打开 Windows 注册表指定路径（仅对当前用户生效，无需管理员权限）
    // 注册表路径：HKEY_CURRENT_USER\Software\Classes\Applications\CuteMarkEd.exe
    QSettings settings("HKEY_CURRENT_USER\\Software\\Classes\\Applications\\" + fileName, QSettings::NativeFormat); // 中文注释：声明变量、对象或函数。
    //设置应用的友好名称（显示给用户的名称）
    settings.setValue("FriendlyAppName", displayName); // 中文注释：执行当前语句。
    //开始操作SupportedTypes注册表项（标记应用支持的文件类型）
    settings.beginGroup("SupportedTypes"); // 中文注释：执行当前语句。
    //遍历所有要关联的文件类型，逐个写入注册表
    for (const QString& fileType : fileTypes) // 中文注释：开始循环遍历数据。
        //给每个文件类型设置空值
        settings.setValue(fileType, QString()); // 中文注释：执行当前语句。
    //结束SupportedTypes操作
    settings.endGroup(); // 中文注释：执行当前语句。
    //开始配置shell/open注册表项(设置文件双击打开时的行为)
    settings.beginGroup("shell"); // 中文注释：执行当前语句。
    settings.beginGroup("open"); // 中文注释：执行当前语句。
    //设置打开操作的友好名称
    settings.setValue("FriendlyAppName", displayName); // 中文注释：执行当前语句。
    //配置打开文件时的命令行参数
    settings.beginGroup("Command"); // 中文注释：执行当前语句。
    // 命令行格式："应用路径" "%1" （%1 是 Windows 传递的选中文件路径）
    // QDir::toNativeSeparators 转换路径分隔符为 Windows 风格（\ 而非 /）
    settings.setValue(".", QChar('"') + QDir::toNativeSeparators(filePath) + QString("\" \"%1\"")); // 中文注释：执行当前语句。
} // 中文注释：结束当前代码块。
#endif // 中文注释：结束条件编译或头文件保护。


/**
 * @brief 程序入口函数
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return 程序退出码
 */
int main(int argc, char *argv[]) // 中文注释：声明变量、对象或函数。
{ // 中文注释：进入当前代码块。
    // 1. 初始化 Qt 应用对象（QApplication 用于 GUI 程序，处理事件循环、控件初始化等）
    QApplication app(argc, argv); // 中文注释：声明变量、对象或函数。
    //设置应用元信息
    app.setOrganizationName("CuteMarkEd Project");//组织名称
    app.setApplicationName("CuteMarkEd");//应用名称
    app.setApplicationDisplayName("CuteMarkEd");//应用显示名称*（给用户看的）
    app.setApplicationVersion("0.11.3");//应用版本号
// Windows 平台专属：注册 Markdown 相关文件类型
#ifdef Q_OS_WIN // 中文注释：根据编译条件启用对应代码。
    QStringList fileTypes; // 中文注释：声明变量、对象或函数。
    fileTypes << ".markdown" << ".md" << ".mdown";//要关联的文件后缀
    associateFileTypes(fileTypes);//调用注册表写入函数
#endif // 中文注释：结束条件编译或头文件保护。

    // load translation for Qt
    //加载Qt自身的国际化翻译软件（适配 Qt 内置控件的语言，如按钮、对话框）
    QTranslator qtTranslator; // 中文注释：声明变量、对象或函数。
    //优先从Qt安装目录的translations文件夹加载文件
    if (!qtTranslator.load("qt_" + QLocale::system().name(), // 中文注释：判断条件是否成立。
                           QLibraryInfo::location(QLibraryInfo::TranslationsPath))) { // 中文注释：声明变量、对象或函数。
        //备用路径，当前程序目录下的translations
        qtTranslator.load("qt_" + QLocale::system().name(), "translations"); // 中文注释：声明变量、对象或函数。
    } // 中文注释：结束当前代码块。
    //安装Qt翻译器
    app.installTranslator(&qtTranslator); // 中文注释：执行当前语句。

    // try to load translation for current locale from resource file
    //加载应用自身的国际化翻译软件
    QTranslator translator; // 中文注释：声明变量、对象或函数。
    // 从资源文件（:/translations）加载对应系统语言的翻译文件（如 cutemarked_zh_CN.qm）
    translator.load("cutemarked_" + QLocale::system().name(), ":/translations"); // 中文注释：执行当前语句。
    //安装应用翻译器
    app.installTranslator(&translator); // 中文注释：执行当前语句。

    // setup command line parser
    //配置命令行参数解析器
    QCommandLineParser parser; // 中文注释：声明变量、对象或函数。
    parser.addHelpOption();// 添加 --help/-h 帮助选项
    parser.addVersionOption();  // 添加 --version/-v 版本选项
    //添加位置参数：file（指定启动时要打开的文件），并翻译参数描述
    parser.addPositionalArgument("file", QApplication::translate("main", "The file to open.")); // 中文注释：执行当前语句。
    parser.process(app);//解析命令行参数

    // get filename from command line arguments
       // 6. 提取命令行中指定的要打开的文件路径
    QString fileName; // 中文注释：声明变量、对象或函数。
    const QStringList cmdLineArgs = parser.positionalArguments(); // 中文注释：声明变量、对象或函数。
    if (!cmdLineArgs.isEmpty()) { // 中文注释：判断条件是否成立。
        fileName = cmdLineArgs.at(0);//取第一个位置参数作为要打开的文件
    } // 中文注释：结束当前代码块。
    //创建主窗口并显示
    MainWindow w(fileName);// 传入要打开的文件路径，初始化主窗口
    w.show(); // 显示主窗口（非阻塞，需依赖事件循环）

#ifdef Q_OS_MAC // 中文注释：根据编译条件启用对应代码。
    // 安装 macOS 文件打开事件过滤器，处理 Finder 双击打开
    FileOpenEventFilter *fileFilter = new FileOpenEventFilter(&w, &app); // 中文注释：创建新的对象实例。
    app.installEventFilter(fileFilter); // 中文注释：执行当前语句。
#endif // 中文注释：结束条件编译或头文件保护。
    // 8. 启动 Qt 事件循环（程序进入等待用户交互状态，直到主窗口关闭）
    return app.exec(); // 中文注释：返回函数处理结果。
} // 中文注释：结束当前代码块。

