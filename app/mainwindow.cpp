// 文件说明：app\mainwindow.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
 #include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QFileInfo>
#ifndef Q_OS_MACOS
#include <QIcon>
#endif
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QNetworkProxy>
#include <QPrintDialog>
#include <QPrinter>
#include <QProcess>
#include <QScrollBar>
#include <QSettings>
#include <QTabBar>
#include <QStandardPaths>
#include <QTextDocumentWriter>
#include <QTimer>
#include <QUuid>
#include "compat/webenginecompat.h"
#include <QActionGroup>
#include <QMouseEvent>
#include <QPushButton>
#include <QWindow>

#ifdef Q_OS_WIN
// QWinJumpList was removed in Qt 6 (QtWinExtras module)
// #include <QWinJumpList>
// #include <QWinJumpListCategory>
#endif

#include <jsonfile.h>
#include <snippets/jsonsnippettranslatorfactory.h>
#include <snippets/snippetcollection.h>
#include <spellchecker/dictionary.h>
#include <themes/stylemanager.h>
#include <themes/themecollection.h>
#include <datalocation.h>
#include "controls/activelabel.h"
#include "controls/findreplacewidget.h"
#include "controls/languagemenu.h"
#include "controls/recentfilesmenu.h"
#include "aboutdialog.h"
#include "htmlpreviewcontroller.h"
#include "htmlpreviewgenerator.h"
#include "htmlviewsynchronizer.h"
#include "htmlhighlighter.h"
#include "imagetooldialog.h"
#include "markdownmanipulator.h"
#include "exporthtmldialog.h"
#include "exportpdfdialog.h"
#include "options.h"
#include "optionsdialog.h"
#include "revealviewsynchronizer.h"
#include "savefileadapter.h"
#include "snippetcompleter.h"
#include "tabletooldialog.h"
#include "statusbarwidget.h"
#include <datavisualization/datavisualizationpanel.h>
#include <security/securitymanager.h>
#include <security/securedocument.h>
#include <security/biometricauth.h>
#include <security/fileencryption.h>
#include <security/securitywidgets.h>
#include <publishing/epubexporter.h>
#include <publishing/blogpublisher.h>
#include <publishing/slideshowpresenter.h>
#include <publishing/staticsitegenerator.h>
#include <academic/citationmanager.h>
#include <input/voiceinput.h>
#include <translation/translationmanager.h>
#include <export/wordexporter.h>
#include <export/latexexporter.h>
#include <export/mindmapview.h>
#include <library/noteslibrary.h>
#include <vcs/gitmanager.h>
#include <sync/cloudsync.h>
#include <editor/focusmode.h>
#include <template/templatemanager.h>
#include <editor/multitabeditor.h>
#include <sharing/documentsharing.h>
#include <annotation/commentmanager.h>
#include <revision/revisiontracker.h>
#include <export/imageexporter.h>
#include <writing/wordcountpanel.h>
#include <writing/outlinenavigator.h>
#include <writing/bookmarkmanager.h>
#include <writing/enhancedspellchecker.h>
#include <writing/writinggoal.h>
#include <preview/previewthememanager.h>
#include <preview/presentationmode.h>
#include <preview/printpreviewdialog.h>
#include <preview/tocfloatingwindow.h>
#include <extension/pluginmanager.h>
#include <extension/scriptengine.h>
#include <extension/shortcutmanager.h>
#include <extension/editortheme.h>
#include <collaboration/collaborationmanager.h>
#include <collaboration/collaborationpanel.h>
#include <collaboration/remotecursoroverlay.h>
#include <ai/aiwritingassistant.h>
#include <ai/aiwritingpanel.h>
#include <search/searchindexmanager.h>
#include <search/advancedsearchpanel.h>
#include <revision/timelineview.h>
#include <editor/largefilemanager.h>
#include <editor/memoryoptimizer.h>
#include <QDockWidget>
#include <QGroupBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QMenu>
#include <QLineEdit>
#include <QTextEdit>
#include <QCoreApplication>
#include <QComboBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QListWidget>
#include <QTreeWidget>
#include <QPlainTextEdit>

namespace {

QString blogPublisherConfigPath()
{
    return QDir(DataLocation::writableLocation())
        .filePath(QStringLiteral("blog-publisher-configs.json"));
}

QString cloudSyncConfigPath()
{
    return QDir(DataLocation::writableLocation())
        .filePath(QStringLiteral("cloud-sync-config.json"));
}

bool loadCloudSyncConfig(CloudSync *sync)
{
    if (!sync) {
        return false;
    }

    const QString path = cloudSyncConfigPath();
    if (!QFileInfo::exists(path)) {
        return true;
    }

    return sync->loadConfig(path);
}

bool saveCloudSyncConfig(CloudSync *sync)
{
    if (!sync) {
        return false;
    }

    QDir dataDir(DataLocation::writableLocation());
    if (!dataDir.exists() && !dataDir.mkpath(QStringLiteral("."))) {
        return false;
    }

    return sync->saveConfig(cloudSyncConfigPath());
}

QString translationSampleDictionaryPath()
{
    const QString relativePath =
        QStringLiteral("examples/translation/offline-dictionary-v1.json");

    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        QDir::current().absoluteFilePath(relativePath),
        QDir(appDir).absoluteFilePath(relativePath),
        QDir(appDir).absoluteFilePath(QStringLiteral("../") + relativePath),
        QDir(appDir).absoluteFilePath(QStringLiteral("../../") + relativePath),
        QDir(appDir).absoluteFilePath(QStringLiteral("../../../") + relativePath),
        QDir(appDir).absoluteFilePath(QStringLiteral("../../../../") + relativePath)
    };

    for (const QString &candidate : candidates) {
        const QString clean = QDir::cleanPath(candidate);
        if (QFileInfo::exists(clean) && QFileInfo(clean).isFile()) {
            return clean;
        }
    }

    return QString();
}

QList<BlogPublisher::BlogConfig> loadBlogPublisherConfigs()
{
    return BlogPublisher::loadConfigs(blogPublisherConfigPath());
}

bool saveBlogPublisherConfigs(const QList<BlogPublisher::BlogConfig> &configs)
{
    return BlogPublisher::saveConfigs(configs, blogPublisherConfigPath());
}

QString profileDisplayName(const BlogPublisher::BlogConfig &config, int index)
{
    const QString explicitName = config.name.trimmed();
    if (!explicitName.isEmpty()) {
        return explicitName;
    }

    QString platform = BlogPublisher::platformName(config.platform);
    if (platform.isEmpty()) {
        platform = QStringLiteral("Profile");
    }
    return QStringLiteral("%1 %2").arg(platform).arg(index + 1);
}

bool loadBlogPublisherConfig(BlogPublisher *publisher)
{
    if (!publisher) {
        return false;
    }

    const QList<BlogPublisher::BlogConfig> configs = loadBlogPublisherConfigs();
    if (configs.isEmpty()) {
        return false;
    }

    publisher->setConfig(configs.first());
    return true;
}

bool saveBlogPublisherConfig(BlogPublisher *publisher)
{
    if (!publisher) {
        return false;
    }

    QList<BlogPublisher::BlogConfig> configs = loadBlogPublisherConfigs();
    BlogPublisher::BlogConfig current = publisher->config();

    int index = -1;
    const QString currentName = current.name.trimmed();
    if (!currentName.isEmpty()) {
        for (int i = 0; i < configs.size(); ++i) {
            if (configs.at(i).name.trimmed() == currentName) {
                index = i;
                break;
            }
        }
    }

    if (index < 0) {
        configs.prepend(current);
    } else {
        configs[index] = current;
        if (index > 0) {
            configs.move(index, 0);
        }
    }

    return saveBlogPublisherConfigs(configs);
}

} // namespace

MainWindow::MainWindow(const QString &fileName, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    options(new Options(this)),
    stylesGroup(new QActionGroup(this)),
    statusBarWidget(0),
    generator(new HtmlPreviewGenerator(options, this)),
    snippetCollection(new SnippetCollection(this)),
    viewSynchronizer(0),
    htmlPreviewController(0),
    themeCollection(new ThemeCollection()),
    splitFactor(0.5),
    rightViewCollapsed(false),
    dataVisualizationDock(nullptr),
    dataVisualizationPanel(nullptr),
    secureDocument(nullptr),
    isEncryptedMode(false),
    actionEncrypt(nullptr),
    actionDecrypt(nullptr),
    actionSecuritySettings(nullptr),
    securityMenu(nullptr),
    epubExporter(nullptr),
    blogPublisher(nullptr),
    slideshowPresenter(nullptr),
    staticSiteGenerator(nullptr),
    publishMenu(nullptr),
    actionExportEpub(nullptr),
    actionPublishBlog(nullptr),
    actionSlideshow(nullptr),
    actionStaticSite(nullptr),
    citationManager(nullptr),
    voiceInput(nullptr),
    translationManager(nullptr),
    toolsMenu(nullptr),
    actionCitationManager(nullptr),
    actionInsertCitation(nullptr),
    actionVoiceInput(nullptr),
    actionTranslate(nullptr),
    wordExporter(nullptr),
    latexExporter(nullptr),
    mindMapView(nullptr),
    mindMapDock(nullptr),
    actionExportWord(nullptr),
    actionExportLaTeX(nullptr),
    actionShowMindMap(nullptr),
    notesLibrary(nullptr),
    gitManager(nullptr),
    cloudSync(nullptr),
    libraryDock(nullptr),
    libraryMenu(nullptr),
    gitMenu(nullptr),
    syncMenu(nullptr),
    actionLibraryManager(nullptr),
    actionQuickSearch(nullptr),
    actionGitHistory(nullptr),
    actionGitDiff(nullptr),
    actionGitCommit(nullptr),
    actionGitEnableLfs(nullptr),
    actionGitManageLfs(nullptr),
    actionGitQuickTrackLargeFiles(nullptr),
    actionSyncNow(nullptr),
    actionSyncSettings(nullptr),
    focusMode(nullptr),
    actionFocusMode(nullptr),
    actionFocusScope(nullptr),
    actionFocusScrollMode(nullptr),
    focusMenu(nullptr),
    templateManager(nullptr),
    templateMenu(nullptr),
    actionNewFromTemplate(nullptr),
    actionManageTemplates(nullptr),
    multiTabEditor(nullptr),
    tabMenu(nullptr),
    actionTabNew(nullptr),
    actionTabClose(nullptr),
    actionTabCloseAll(nullptr),
    actionTabNext(nullptr),
    actionTabPrevious(nullptr),
    actionTabRestoreClosed(nullptr),
    documentSharing(nullptr),
    commentManager(nullptr),
    revisionTracker(nullptr),
    imageExporter(nullptr),
    shareMenu(nullptr),
    commentMenu(nullptr),
    revisionMenu(nullptr),
    actionShareDocument(nullptr),
    actionShareManager(nullptr),
    actionAddComment(nullptr),
    actionShowComments(nullptr),
    actionExportComments(nullptr),
    actionRevisionHistory(nullptr),
    actionCreateRevision(nullptr),
    actionCompareRevisions(nullptr),
    actionRestoreRevision(nullptr),
    actionExportToPng(nullptr),
    actionExportToSvg(nullptr),
    wordCountPanel(nullptr),
    outlineNavigator(nullptr),
    bookmarkManager(nullptr),
    enhancedSpellChecker(nullptr),
    writingGoal(nullptr),
    wordCountDock(nullptr),
    outlineDock(nullptr),
    writingGoalDock(nullptr),
    writingMenu(nullptr),
    bookmarkMenu(nullptr),
    actionToggleWordCount(nullptr),
    actionToggleOutline(nullptr),
    actionBookmarkAdd(nullptr),
    actionBookmarkRemove(nullptr),
    actionBookmarkNext(nullptr),
    actionBookmarkPrevious(nullptr),
    actionBookmarkList(nullptr),
    actionSpellCheck(nullptr),
    actionAddToDict(nullptr),
    actionManageDict(nullptr),
    actionSetDailyGoal(nullptr),
    actionSetSessionGoal(nullptr),
    actionWritingStats(nullptr),
    previewThemeManager(nullptr),
    presentationMode(nullptr),
    printPreviewDialog(nullptr),
    tocFloatingWindow(nullptr),
    previewMenu(nullptr),
    previewThemeMenu(nullptr),
    actionPreviewChangeTheme(nullptr),
    actionPreviewEditTheme(nullptr),
    actionPreviewImportTheme(nullptr),
    actionPreviewExportTheme(nullptr),
    actionStartPresentation(nullptr),
    actionPrintPreview(nullptr),
    actionToggleTocFloat(nullptr),
    pluginManager(nullptr),
    scriptEngine(nullptr),
    shortcutManager(nullptr),
    editorThemeManager(nullptr),
    extensionMenu(nullptr),
    pluginMenu(nullptr),
    scriptMenu(nullptr),
    editorThemeMenu(nullptr),
    actionPluginManager(nullptr),
    actionScriptManager(nullptr),
    actionShortcutEditor(nullptr),
    actionEditorThemeEditor(nullptr),
    aiWritingAssistant(nullptr),
    aiWritingPanel(nullptr),
    aiWritingDock(nullptr),
    aiMenu(nullptr),
    actionAICompletion(nullptr),
    actionAIGrammarCheck(nullptr),
    actionAISummary(nullptr),
    actionAITitle(nullptr),
    actionAIShowPanel(nullptr),
    actionAISettings(nullptr),
    searchIndexManager(nullptr),
    advancedSearchPanel(nullptr),
    searchDock(nullptr),
    searchMenu(nullptr),
    actionSearchInFiles(nullptr),
    actionSearchRebuildIndex(nullptr),
    actionSearchHistory(nullptr),
    actionSearchClearHistory(nullptr),
    timelineView(nullptr),
    timelineMiniView(nullptr),
    timelineDock(nullptr),
    timelineMenu(nullptr),
    actionTimelineShowPanel(nullptr),
    actionTimelineRefresh(nullptr),
    actionTimelineCompareMode(nullptr),
    largeFileManager(nullptr),
    virtualScrollHelper(nullptr),
    largeFileStatusLabel(nullptr),
    memoryOptimizer(nullptr),
    memoryStatusLabel(nullptr),
    actionMemoryOptimize(nullptr),
    actionMemoryStatus(nullptr)
{
    ui->setupUi(this);//将ui文件里的控件实例hua
    setupUi();

    setFileName(fileName);

    QTimer::singleShot(0, this, &MainWindow::initializeApp);
}

// 函数说明：销毁 MainWindow 对象，释放本模块持有的资源。
MainWindow::~MainWindow()
{
    // 如果 AI 面板已脱离停靠窗口（无父对象），需要手动清理
    if (aiWritingPanel && aiWritingPanel->parent() == nullptr) {
         aiWritingPanel->close();
        delete aiWritingPanel;
        aiWritingPanel = nullptr;
    }

    delete viewSynchronizer;

    // Stop the preview thread before the main window is fully torn down.
    // This avoids queued preview updates targeting MainWindow during shutdown.
    if (generator) {
        disconnect(generator, nullptr, this, nullptr);
        if (generator->isRunning()) {
            generator->markdownTextChanged(QString());
            generator->wait();
        }
        delete generator;
    }

    delete ui;
}

// 函数说明：关闭 MainWindow 相关窗口或资源，并处理必要的保存确认。
void MainWindow::closeEvent(QCloseEvent *e)
{
    // 先保存当前标签的状态
    saveCurrentTabState();

    // 检查所有标签是否有未保存的修改
    for (int i = 0; i < m_tabs.size(); ++i) {
        if (m_tabs[i].isModified) {
            // 切换到未保存的标签
            m_tabBar->setCurrentIndex(i);
            if (!maybeSave()) {
                e->ignore();
                return;
            }
            // maybeSave 成功后更新状态
            saveCurrentTabState();
        }
    }

    if (cloudSync && !saveCloudSyncConfig(cloudSync)) {
        const QString detail = cloudSync->lastError().isEmpty()
                                   ? tr("无法写入配置文件: %1").arg(cloudSyncConfigPath())
                                   : cloudSync->lastError();
        QMessageBox::warning(this, tr("云同步"),
                             tr("保存云同步配置失败。\n%1").arg(detail));
    }

    // Stop preview updates before settings are written and QWidget teardown begins.
    if (generator) {
        disconnect(generator, nullptr, this, nullptr);
        if (generator->isRunning()) {
            generator->markdownTextChanged(QString());
            generator->wait();
        }
    }

    // Unload plugins while the QMainWindow dock layout is still valid.
    if (pluginManager) {
        pluginManager->unloadAllPlugins();
    }

    writeSettings();
    e->accept();
}

// 函数说明：响应尺寸变化，重新计算 MainWindow 的布局。
void MainWindow::resizeEvent(QResizeEvent *e)
{
    Q_UNUSED(e)
    updateSplitter();
}

//初始化APP
void MainWindow::initializeApp()
{
    // In Qt WebEngine, link handling is done via acceptNavigationRequest
    // We handle this via signals from the page

    themeCollection->load(":/builtin-htmlpreview-themes.json");
    loadCustomStyles();
    setupHtmlPreviewThemes();

    // apply last used theme
    lastUsedTheme();

    ui->plainTextEdit->tabWidthChanged(options->tabWidth());
    ui->plainTextEdit->rulerEnabledChanged(options->isRulerEnabled());
    ui->plainTextEdit->rulerPosChanged(options->rulerPos());

    // init extension flags
    ui->actionAutolink->setChecked(options->isAutolinkEnabled());
    ui->actionStrikethroughOption->setChecked(options->isStrikethroughEnabled());
    ui->actionAlphabeticLists->setChecked(options->isAlphabeticListsEnabled());
    ui->actionDefinitionLists->setChecked(options->isDefinitionListsEnabled());
    ui->actionSmartyPants->setChecked(options->isSmartyPantsEnabled());
    ui->actionFootnotes->setChecked(options->isFootnotesEnabled());
    ui->actionSuperscript->setChecked(options->isSuperscriptEnabled());

    // init option flags
    ui->actionMathSupport->setChecked(options->isMathSupportEnabled());
    ui->actionDiagramSupport->setChecked(options->isDiagramSupportEnabled());
    ui->actionCodeHighlighting->setChecked(options->isCodeHighlightingEnabled());
    ui->actionShowSpecialCharacters->setChecked(options->isShowSpecialCharactersEnabled());
    ui->actionWordWrap->setChecked(options->isWordWrapEnabled());
    ui->actionCheckSpelling->setChecked(options->isSpellingCheckEnabled());
    ui->plainTextEdit->setSpellingCheckEnabled(options->isSpellingCheckEnabled());
    ui->actionYamlHeaderSupport->setChecked(options->isYamlHeaderSupportEnabled());

    // set url to markdown syntax help
    ui->webView_2->setUrl(tr("qrc:/syntax.html"));

    // Qt WebEngine handles remote content loading automatically
    // No need for QWebSettings in Qt 6

    // Qt WebEngine has built-in developer tools
    // Access via right-click context menu

    ui->menuLanguages->loadDictionaries(options->dictionaryLanguage());

    //: path to built-in snippets resource.
    JsonFile<Snippet>::load(":/markdown-snippets.json", snippetCollection);
    QString path = DataLocation::writableLocation();
    JsonFile<Snippet>::load(path + "/user-snippets.json", snippetCollection);

    // setup file explorer
    connect(ui->fileExplorerDockContents, &FileExplorerWidget::fileSelected,
            this, &MainWindow::openRecentFile);

    // setup jump list on windows (disabled in Qt 6, QWinJumpList removed)
#if 0
    QWinJumpList jumplist;
    jumplist.recent()->setVisible(true);
#endif

    // load file passed to application on start
    if (!fileName.isEmpty()) {
        load(fileName);
    }
}

// 函数说明：打开 MainWindow 对应的文件、资源或功能入口。
void MainWindow::openRecentFile(const QString &fileName)
{
    openFile(fileName);
}

// 函数说明：实现 MainWindow::languageChanged 的核心逻辑，供当前模块调用。
void MainWindow::languageChanged(const Dictionary &dictionary)
{
    options->setDictionaryLanguage(dictionary.language());
    ui->plainTextEdit->setSpellingDictionary(dictionary);
}

// 函数说明：处理主窗口的文件菜单动作，衔接文档读写和界面状态。
void MainWindow::fileNew()
{
    tabNew();
}

//打开文件
void MainWindow::fileOpen()
{
    QString name = QFileDialog::getOpenFileName(this, tr("Open File..."),
                                                QString(), tr("Markdown Files (*.markdown *.md *.mdown);;All Files (*)"));
    if (name.isEmpty())
        return;

    // 检查文件是否已在某个标签中打开
    QString absPath = QFileInfo(name).absoluteFilePath();
    for (int i = 0; i < m_tabs.size(); ++i) {
        if (!m_tabs[i].filePath.isEmpty() &&
            QFileInfo(m_tabs[i].filePath).absoluteFilePath() == absPath) {
            m_tabBar->setCurrentIndex(i);
            return;
        }
    }

    // 如果当前标签是空白未修改的，直接在当前标签中打开
    if (fileName.isEmpty() && !ui->plainTextEdit->document()->isModified()
        && ui->plainTextEdit->toPlainText().isEmpty()) {
        load(name);
        return;
    }

    // 否则在新标签中打开
    saveCurrentTabState();
    updateTabTitle(m_currentTabIndex);

    TabData newTab;
    m_tabs.append(newTab);
    QString tabTitle = QFileInfo(name).fileName();
    int newIndex = m_tabBar->addTab(tabTitle);

    // 切换到新标签并加载文件
    m_tabBar->blockSignals(true);
    m_currentTabIndex = newIndex;
    m_tabBar->setCurrentIndex(newIndex);
    m_tabBar->blockSignals(false);

    // 清空编辑器，加载新文件
    ui->plainTextEdit->blockSignals(true);
    ui->plainTextEdit->clear();
    ui->plainTextEdit->blockSignals(false);

    load(name);
}

// 函数说明：打开 MainWindow 对应的文件、资源或功能入口。
void MainWindow::openFile(const QString &filePath)
{
    if (filePath.isEmpty()) {
        return;
    }

    // 检查文件是否已在某个标签中打开
    QString absPath = QFileInfo(filePath).absoluteFilePath();
    for (int i = 0; i < m_tabs.size(); ++i) {
        if (!m_tabs[i].filePath.isEmpty() &&
            QFileInfo(m_tabs[i].filePath).absoluteFilePath() == absPath) {
            m_tabBar->setCurrentIndex(i);
            return;
        }
    }

    // 如果当前标签是空白未修改的，直接加载
    if (fileName.isEmpty() && !ui->plainTextEdit->document()->isModified()
        && ui->plainTextEdit->toPlainText().isEmpty()) {
        load(filePath);
        return;
    }

    // 在新标签中打开
    saveCurrentTabState();
    updateTabTitle(m_currentTabIndex);

    TabData newTab;
    m_tabs.append(newTab);
    int newIndex = m_tabBar->addTab(QFileInfo(filePath).fileName());

    m_tabBar->blockSignals(true);
    m_currentTabIndex = newIndex;
    m_tabBar->setCurrentIndex(newIndex);
    m_tabBar->blockSignals(false);

    ui->plainTextEdit->blockSignals(true);
    ui->plainTextEdit->clear();
    ui->plainTextEdit->blockSignals(false);

    load(filePath);
}

//文件保存
bool MainWindow::fileSave()
{
    // file has no name yet?
    if (fileName.isEmpty()) {
        return fileSaveAs();
    }

    // 如果是加密模式，使用安全文档保存（保持加密状态）
    if (isEncryptedMode && secureDocument && secureDocument->isOpen()) {
        secureDocument->setContent(ui->plainTextEdit->toPlainText());
        if (secureDocument->save()) {
            ui->plainTextEdit->document()->setModified(false);
            setWindowModified(false);
            recentFilesMenu->addFile(fileName);

            // 更新标签状态
            if (m_currentTabIndex >= 0 && m_currentTabIndex < m_tabs.size()) {
                m_tabs[m_currentTabIndex].isModified = false;
                updateTabTitle(m_currentTabIndex);
            }
            return true;
        }
        return false;
    }

    SaveFileAdapter file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextDocumentWriter writer(&file, "plaintext");
    bool success = writer.write(ui->plainTextEdit->document());
    if (success) {
        file.commit();

        // set status to unmodified
        ui->plainTextEdit->document()->setModified(false);
        setWindowModified(false);

        // add to recent file list
        recentFilesMenu->addFile(fileName);

        // 更新标签状态
        if (m_currentTabIndex >= 0 && m_currentTabIndex < m_tabs.size()) {
            m_tabs[m_currentTabIndex].isModified = false;
            m_tabs[m_currentTabIndex].filePath = fileName;
            updateTabTitle(m_currentTabIndex);
        }
    }

    return success;
}

//保存文件
bool MainWindow::fileSaveAs()
{
    QString name = QFileDialog::getSaveFileName(this, tr("Save as..."), QString(),
                                              tr("Markdown Files (*.markdown *.md *.mdown);;All Files (*)"));
    if (name.isEmpty()) {
        return false;
    }

    // Add default extension ".md" if the file name as no extension yet.
    if (QFileInfo(name).suffix().isEmpty()) {
        name.append(".md");
    }

    setFileName(name);

    // 更新标签的文件路径
    if (m_currentTabIndex >= 0 && m_currentTabIndex < m_tabs.size()) {
        m_tabs[m_currentTabIndex].filePath = name;
    }

    return fileSave();
}

// 函数说明：处理主窗口的文件菜单动作，衔接文档读写和界面状态。
void MainWindow::fileExportToHtml()
{
    ExportHtmlDialog dialog(fileName);
    if (dialog.exec() == QDialog::Accepted) {

        QString cssStyle;
        if (dialog.includeCSS()) {
            // In Qt WebEngine, we need to track the stylesheet path ourselves
            // For now, use the current theme's stylesheet
            QString previewStylesheet = StyleManager::previewStylesheetPath(currentTheme);
            
            // get resource or file name from url
            QString cssFileName;
            if (previewStylesheet.startsWith("qrc:")) {
                cssFileName = previewStylesheet.mid(3);
            } else if (previewStylesheet.startsWith("file://")) {
                cssFileName = QUrl(previewStylesheet).toLocalFile();
            } else {
                cssFileName = previewStylesheet;
            }

            // read currently used css stylesheet file
            QFile f(cssFileName);
            if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                cssStyle = f.readAll();
            }
        }

        QString highlightJs;
        if (dialog.includeCodeHighlighting()) {
            QFile f(":/scripts/highlight.js/highlight.pack.js");
            if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                highlightJs = f.readAll();
            }
        }

        QString html = generator->exportHtml(cssStyle, highlightJs);

        // write HTML source to disk
        QFile f(dialog.fileName());
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&f);
            out.setEncoding(QStringConverter::Utf8);
            out << html;
        }
    }
}

// 函数说明：处理主窗口的文件菜单动作，衔接文档读写和界面状态。
void MainWindow::fileExportToPdf()
{
    ExportPdfDialog dialog(fileName);
    if (dialog.exec() == QDialog::Accepted) {
        // Qt WebEngine uses printToPdf for PDF export
        ui->webView->page()->printToPdf(dialog.fileName());
    }
}

// 函数说明：处理主窗口的文件菜单动作，衔接文档读写和界面状态。
void MainWindow::filePrint()
{
    QPrinter printer;
    QPrintDialog *dlg = new QPrintDialog(&printer, this);
    dlg->setWindowTitle(tr("Print Document"));

    // Qt 6: addEnabledOption has been removed
    // Print selection is now handled automatically by QPrintDialog
    // if (ui->webView->hasSelection())
    //     dlg->addEnabledOption(QAbstractPrintDialog::PrintSelection);

    if (dlg->exec() == QDialog::Accepted)
        ui->webView->print(&printer);

    delete dlg;
}

// 函数说明：处理编辑命令，将用户操作转换为 Markdown 文本变更。
void MainWindow::editUndo()
{
    if (ui->plainTextEdit->document()->isUndoAvailable()) {
        ui->plainTextEdit->document()->undo();
    }
}

// 函数说明：处理编辑命令，将用户操作转换为 Markdown 文本变更。
void MainWindow::editRedo()
{
    if (ui->plainTextEdit->document()->isRedoAvailable()) {
        ui->plainTextEdit->document()->redo();
    }
}

// 函数说明：处理编辑命令，将用户操作转换为 Markdown 文本变更。
void MainWindow::editCopyHtml()
{
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(ui->htmlSourceTextEdit->toPlainText());
}

// 函数说明：处理编辑命令，将用户操作转换为 Markdown 文本变更。
void MainWindow::editGotoLine()
{
    const int STEP = 1;
    const int MIN_VALUE = 1;

    QTextCursor cursor = ui->plainTextEdit->textCursor();
    int currentLine = cursor.blockNumber()+1;
    int maxValue = ui->plainTextEdit->document()->blockCount();

    bool ok;
    int line = QInputDialog::getInt(this, tr("Go to..."),
                                          tr("Line: ", "Line number in the Markdown editor"), currentLine, MIN_VALUE, maxValue, STEP, &ok);
    if (!ok) return;
    ui->plainTextEdit->gotoLine(line);
}

// 函数说明：处理编辑命令，将用户操作转换为 Markdown 文本变更。
void MainWindow::editFindReplace()
{
    ui->findReplaceWidget->setTextEdit(ui->plainTextEdit);
    ui->findReplaceWidget->show();
    ui->findReplaceWidget->setFocus();
}

// 函数说明：处理编辑命令，将用户操作转换为 Markdown 文本变更。
void MainWindow::editStrong()
{
    MarkdownManipulator manipulator(ui->plainTextEdit);
    manipulator.wrapSelectedText("**");
}

// 函数说明：处理编辑命令，将用户操作转换为 Markdown 文本变更。
void MainWindow::editEmphasize()
{
    MarkdownManipulator manipulator(ui->plainTextEdit);
    manipulator.wrapSelectedText("*");
}

// 函数说明：处理编辑命令，将用户操作转换为 Markdown 文本变更。
void MainWindow::editStrikethrough()
{
    MarkdownManipulator manipulator(ui->plainTextEdit);
    manipulator.wrapSelectedText("~~");
}

// 函数说明：处理编辑命令，将用户操作转换为 Markdown 文本变更。
void MainWindow::editInlineCode()
{
    MarkdownManipulator manipulator(ui->plainTextEdit);
    manipulator.wrapSelectedText("`");
}

// 函数说明：处理编辑命令，将用户操作转换为 Markdown 文本变更。
void MainWindow::editCenterParagraph()
{
    MarkdownManipulator manipulator(ui->plainTextEdit);
    manipulator.wrapCurrentParagraph("->", "<-");
}

// 函数说明：处理编辑命令，将用户操作转换为 Markdown 文本变更。
void MainWindow::editHardLinebreak()
{
    MarkdownManipulator manipulator(ui->plainTextEdit);
    manipulator.appendToLine("  \n");
}

// 函数说明：处理编辑命令，将用户操作转换为 Markdown 文本变更。
void MainWindow::editBlockquote()
{
    MarkdownManipulator manipulator(ui->plainTextEdit);
    manipulator.formatTextAsQuote();
}

// 函数说明：处理编辑命令，将用户操作转换为 Markdown 文本变更。
void MainWindow::editIncreaseHeaderLevel()
{
    MarkdownManipulator manipulator(ui->plainTextEdit);
    manipulator.increaseHeadingLevel();
}

// 函数说明：处理编辑命令，将用户操作转换为 Markdown 文本变更。
void MainWindow::editDecreaseHeaderLevel()
{
    MarkdownManipulator manipulator(ui->plainTextEdit);
    manipulator.decreaseHeadingLevel();
}

// 函数说明：处理编辑命令，将用户操作转换为 Markdown 文本变更。
void MainWindow::editInsertTable()
{
    TableToolDialog dialog;
    if (dialog.exec() == QDialog::Accepted) {
        MarkdownManipulator manipulator(ui->plainTextEdit);
        manipulator.insertTable(dialog.rows(), dialog.columns(),
                                dialog.alignments(), dialog.tableCells());
    }
}

// 函数说明：处理编辑命令，将用户操作转换为 Markdown 文本变更。
void MainWindow::editInsertImage()
{
    ImageToolDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        MarkdownManipulator manipulator(ui->plainTextEdit);
        manipulator.insertImageLink(dialog.alternateText(), dialog.imageSourceLink(), dialog.optionalTitle());
    }
}

// 函数说明：处理视图切换逻辑，同步窗口布局和可见状态。
void MainWindow::viewChangeSplit()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (action->objectName() == ui->actionSplit_1_1->objectName()) {
        splitFactor = 0.5;
    } else if (action->objectName() == ui->actionSplit_2_1->objectName()) {
        splitFactor = 0.666;
    } else if (action->objectName() == ui->actionSplit_1_2->objectName()) {
        splitFactor = 0.333;
    } else if (action->objectName() == ui->actionSplit_3_1->objectName()) {
        splitFactor = 0.75;
    } else if (action->objectName() == ui->actionSplit_1_3->objectName()) {
        splitFactor = 0.25;
    }

    updateSplitter();

    // web view was collapsed and is now visible again, so update it
    if (rightViewCollapsed) {
        syncWebViewToHtmlSource();
    }
}

// 函数说明：实现 MainWindow::lastUsedTheme 的核心逻辑，供当前模块调用。
void MainWindow::lastUsedTheme()
{
    QString themeName = options->lastUsedTheme();

    currentTheme = themeCollection->theme(themeName);
    applyCurrentTheme();

    for (auto action : stylesGroup->actions()) {
        if (action->text() == themeName) {
            action->setChecked(true);
            stylesGroup->triggered(action);
            break;
        }
    }
}

// 函数说明：实现 MainWindow::themeChanged 的核心逻辑，供当前模块调用。
void MainWindow::themeChanged()
{
    QAction *action = qobject_cast<QAction*>(sender());
    QString themeName = action->text();

    currentTheme = themeCollection->theme(themeName);
    applyCurrentTheme();

    options->setLastUsedTheme(themeName);
}

// 函数说明：处理编辑命令，将用户操作转换为 Markdown 文本变更。
void MainWindow::editorStyleChanged()
{
    QString markdownHighlighting = StyleManager::markdownHighlightingPath(currentTheme);
    ui->plainTextEdit->loadStyleFromStylesheet(stylePath(markdownHighlighting));
}

// 函数说明：应用 MainWindow 当前配置，让编辑器或预览立即生效。
void MainWindow::applyCurrentTheme()
{
    QString markdownHighlighting = StyleManager::markdownHighlightingPath(currentTheme);
    QString codeHighlighting = StyleManager::codeHighlightingPath(currentTheme);
    QString previewStylesheet = StyleManager::previewStylesheetPath(currentTheme);

    generator->setCodeHighlightingStyle(codeHighlighting);
    ui->plainTextEdit->loadStyleFromStylesheet(stylePath(markdownHighlighting));

    // 读取预览样式表 CSS 内容
    QString cssPath = previewStylesheet;
    if (cssPath.startsWith("qrc:")) {
        cssPath = cssPath.mid(3);  // "qrc:/css/..." -> ":/css/..."
    }
    QFile cssFile(cssPath);
    if (cssFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        currentPreviewCss = QString::fromUtf8(cssFile.readAll());
    }

    // 通过 JS 立即注入 CSS 到当前页面（实时切换效果）
    applyPreviewCss();

    // 触发预览刷新，使 htmlResultReady 中的 CSS 注入生效
    plainTextChanged();
}

// 函数说明：应用 MainWindow 当前配置，让编辑器或预览立即生效。
void MainWindow::applyPreviewCss()
{
    if (!ui->webView || currentPreviewCss.isEmpty()) return;

    // 用 JSON 编码 CSS 字符串，确保特殊字符正确转义
    QString escapedCss = currentPreviewCss;
    escapedCss.replace('\\', "\\\\");
    escapedCss.replace('\'', "\\'");
    escapedCss.replace('\n', "\\n");
    escapedCss.replace('\r', "");

    QString js = QString(
        "(function() {"
        "  var el = document.getElementById('cutemarked-preview-style');"
        "  if (!el) { el = document.createElement('style'); el.id = 'cutemarked-preview-style'; document.head.appendChild(el); }"
        "  el.textContent = '%1';"
        "})();"
    ).arg(escapedCss);

    ui->webView->page()->runJavaScript(js);
}

// 函数说明：处理视图切换逻辑，同步窗口布局和可见状态。
void MainWindow::viewFullScreenMode()
{
    if (ui->actionFullScreenMode->isChecked()) {
        showFullScreen();
    } else {
        showNormal();
    }
}

// 函数说明：处理视图切换逻辑，同步窗口布局和可见状态。
void MainWindow::viewHorizontalLayout(bool checked)
{
    if (checked) {
        ui->splitter->setOrientation(Qt::Vertical);
    } else {
        ui->splitter->setOrientation(Qt::Horizontal);
    }
}

// 函数说明：实现 MainWindow::extrasShowSpecialCharacters 的核心逻辑，供当前模块调用。
void MainWindow::extrasShowSpecialCharacters(bool checked)
{
    options->setShowSpecialCharactersEnabled(checked);
    ui->plainTextEdit->setShowSpecialCharacters(checked);
}

// 函数说明：实现 MainWindow::extrasYamlHeaderSupport 的核心逻辑，供当前模块调用。
void MainWindow::extrasYamlHeaderSupport(bool checked)
{
    options->setYamlHeaderSupportEnabled(checked);
    ui->plainTextEdit->setYamlHeaderSupportEnabled(checked);
    plainTextChanged();
}

// 函数说明：实现 MainWindow::extrasWordWrap 的核心逻辑，供当前模块调用。
void MainWindow::extrasWordWrap(bool checked)
{
    options->setWordWrapEnabled(checked);
    ui->plainTextEdit->setLineWrapMode(checked ? MarkdownEditor::WidgetWidth : MarkdownEditor::NoWrap);
}

// 函数说明：实现 MainWindow::extensionsAutolink 的核心逻辑，供当前模块调用。
void MainWindow::extensionsAutolink(bool checked)
{
    options->setAutolinkEnabled(checked);
    plainTextChanged();
}

// 函数说明：实现 MainWindow::extensionsStrikethrough 的核心逻辑，供当前模块调用。
void MainWindow::extensionsStrikethrough(bool checked)
{
    options->setStrikethroughEnabled(checked);
    plainTextChanged();
}

// 函数说明：实现 MainWindow::extensionsAlphabeticLists 的核心逻辑，供当前模块调用。
void MainWindow::extensionsAlphabeticLists(bool checked)
{
    options->setAlphabeticListsEnabled(checked);
    plainTextChanged();
}

// 函数说明：实现 MainWindow::extensionsDefinitionLists 的核心逻辑，供当前模块调用。
void MainWindow::extensionsDefinitionLists(bool checked)
{
    options->setDefinitionListsEnabled(checked);
    plainTextChanged();
}

// 函数说明：实现 MainWindow::extensionsSmartyPants 的核心逻辑，供当前模块调用。
void MainWindow::extensionsSmartyPants(bool checked)
{
    options->setSmartyPantsEnabled(checked);
    plainTextChanged();
}

// 函数说明：实现 MainWindow::extensionsFootnotes 的核心逻辑，供当前模块调用。
void MainWindow::extensionsFootnotes(bool enabled)
{
    options->setFootnotesEnabled(enabled);
    plainTextChanged();
}

// 函数说明：实现 MainWindow::extensionsSuperscript 的核心逻辑，供当前模块调用。
void MainWindow::extensionsSuperscript(bool enabled)
{
    options->setSuperscriptEnabled(enabled);
    plainTextChanged();
}

// 函数说明：实现 MainWindow::extrasCheckSpelling 的核心逻辑，供当前模块调用。
void MainWindow::extrasCheckSpelling(bool checked)
{
    ui->plainTextEdit->setSpellingCheckEnabled(checked);
    options->setSpellingCheckEnabled(checked);
}

// 函数说明：实现 MainWindow::extrasOptions 的核心逻辑，供当前模块调用。
void MainWindow::extrasOptions()
{
    QList<QAction*> actions;
    // file menu
    actions << ui->actionNew
            << ui->actionOpen
            << ui->actionSave
            << ui->actionSaveAs
            << ui->actionExportToHTML
            << ui->actionExportToPDF
            << ui->action_Print
            << ui->actionExit;
    // edit menu
    actions << ui->actionUndo
            << ui->actionRedo
            << ui->actionCut
            << ui->actionCopy
            << ui->actionPaste
            << ui->actionStrong
            << ui->actionCopyHtmlToClipboard
            << ui->actionEmphasize
            << ui->actionStrikethrough
            << ui->actionInline_Code
            << ui->actionCenterParagraph
            << ui->actionBlockquote
            << ui->actionIncreaseHeaderLevel
            << ui->actionDecreaseHeaderLevel
            << ui->actionInsertTable
            << ui->actionInsertImage
            << ui->actionFindReplace
            << ui->actionFindNext
            << ui->actionFindPrevious
            << ui->actionGotoLine;
    // view menu
    actions << ui->dockWidget->toggleViewAction()
            << ui->fileExplorerDockWidget->toggleViewAction()
            << ui->actionHtmlSource
            << ui->actionSplit_1_1
            << ui->actionSplit_2_1
            << ui->actionSplit_1_2
            << ui->actionSplit_3_1
            << ui->actionSplit_1_3
            << ui->actionFullScreenMode
            << ui->actionHorizontalLayout;

    // snippet complete
    actions << ui->plainTextEdit->actions();

    OptionsDialog dialog(options, snippetCollection, actions, this);
    if (dialog.exec() == QDialog::Accepted) {
        options->writeSettings();

        QString path = DataLocation::writableLocation();
        QSharedPointer<SnippetCollection> userDefinedSnippets = snippetCollection->userDefinedSnippets();
        JsonFile<Snippet>::save(path + "/user-snippets.json", userDefinedSnippets.data());

        // update shortcuts
        setupCustomShortcuts();
    }
}

// 函数说明：实现 MainWindow::helpMarkdownSyntax 的核心逻辑，供当前模块调用。
void MainWindow::helpMarkdownSyntax()
{
    ui->dockWidget_2->show();
}

// 函数说明：实现 MainWindow::helpAbout 的核心逻辑，供当前模块调用。
void MainWindow::helpAbout()
{
    AboutDialog dialog;
    dialog.exec();
}

// 函数说明：设置 MainWindow 的运行参数，并触发必要的界面或数据刷新。
void MainWindow::setHtmlSource(bool enabled)
{
    if (enabled) {
        ui->stackedWidget->setCurrentWidget(ui->htmlSourcePage);

        // activate HTML highlighter
        htmlHighlighter->setEnabled(true);
        htmlHighlighter->rehighlight();
    } else {
        ui->stackedWidget->setCurrentWidget(ui->webViewPage);

        // deactivate HTML highlighter
        htmlHighlighter->setEnabled(false);

        // update webView now since it was not updated while hidden
        syncWebViewToHtmlSource();
    }

    // sync view menu action
    if (ui->actionHtmlSource->isChecked() != enabled)
        ui->actionHtmlSource->setChecked(enabled);

    updateSplitter();
}

// 函数说明：实现 MainWindow::plainTextChanged 的核心逻辑，供当前模块调用。
void MainWindow::plainTextChanged()
{
    // 如果正在切换标签，不处理
    if (m_switchingTab)
        return;

    QString code = ui->plainTextEdit->toPlainText();

    // generate HTML from markdown
    generator->markdownTextChanged(code);

    // 更新数据可视化面板（延迟更新以提升性能）
    updateDataVisualization();

    // show modification indicator in window title
    bool modified = ui->plainTextEdit->document()->isModified();
    setWindowModified(modified);

    // 更新标签修改状态
    if (m_currentTabIndex >= 0 && m_currentTabIndex < m_tabs.size()) {
        m_tabs[m_currentTabIndex].isModified = modified;
        updateTabTitle(m_currentTabIndex);
    }
}

// 函数说明：实现 MainWindow::htmlResultReady 的核心逻辑，供当前模块调用。
void MainWindow::htmlResultReady(const QString &html)
{
    // 将当前预览主题 CSS 注入到 HTML <head> 中
    QString styledHtml = html;
    if (!currentPreviewCss.isEmpty()) {
        QString styleTag = QString("<style id=\"cutemarked-preview-style\">%1</style>\n").arg(currentPreviewCss);
        styledHtml.replace("</head>", styleTag + "</head>");
    }

    // show html preview
    QUrl baseUrl;
    if (fileName.isEmpty()) {
        baseUrl = QUrl::fromLocalFile(qApp->applicationDirPath());
    } else {
        baseUrl = QUrl::fromLocalFile(QFileInfo(fileName).absolutePath() + "/");
    }

    QList<int> childSizes = ui->splitter->sizes();
    if (ui->webView->isVisible() && childSizes[1] != 0) {
        ui->webView->setHtml(styledHtml, baseUrl);
    }

    // show html source
    ui->htmlSourceTextEdit->setPlainText(styledHtml);
}

// 函数说明：实现 MainWindow::tocResultReady 的核心逻辑，供当前模块调用。
void MainWindow::tocResultReady(const QString &toc)
{
    ui->tocWebView->setHtml(toc);
}

// 函数说明：更新预览相关状态，保持 Markdown、HTML 和目录视图一致。
void MainWindow::previewLinkClicked(const QUrl &url)
{
    if (url.isLocalFile()) {
        // directories are not supported
        if (QFileInfo(url.toLocalFile()).isDir()) return;

        QString filePath = url.toLocalFile();
        // Links to markdown files open new instance
        if (filePath.endsWith(".md") || filePath.endsWith(".markdown") || filePath.endsWith(".mdown")) {
            QProcess::startDetached(qApp->applicationFilePath(), QStringList() << filePath);
            return;
        }
    }

    QDesktopServices::openUrl(url);
}

// 函数说明：实现 MainWindow::tocLinkClicked 的核心逻辑，供当前模块调用。
void MainWindow::tocLinkClicked(const QUrl &url)
{
    QString anchor = url.toString().remove("#");
    // In Qt WebEngine, use JavaScript to scroll to anchor
    QString script = QString("window.location.hash = '%1';").arg(anchor);
    ui->webView->page()->runJavaScript(script);
}

// 函数说明：实现 MainWindow::splitterMoved 的核心逻辑，供当前模块调用。
void MainWindow::splitterMoved(int pos, int index)
{
    Q_UNUSED(index)

    int maxViewWidth = ui->splitter->size().width() - ui->splitter->handleWidth();
    splitFactor = (float)pos / maxViewWidth;

    // web view was collapsed and is now visible again, so update it
    if (rightViewCollapsed && ui->splitter->sizes().at(1) > 0) {
        syncWebViewToHtmlSource();
    }
    rightViewCollapsed = (ui->splitter->sizes().at(1) == 0);
}

// 函数说明：向 MainWindow 管理的数据集合中添加一项内容。
void MainWindow::addJavaScriptObject()
{
    // Qt WebEngine uses QWebChannel for JavaScript communication
    // This functionality needs to be reimplemented using QWebChannel
    // For now, we'll comment this out as it requires significant refactoring
    // TODO: Implement using QWebChannel
}

//文件加载入口，顺手串上了加密文件判断和大文件优化
bool MainWindow::load(const QString &fileName)
{
    if (!QFile::exists(fileName)) {
        return false;
    }

    // 如果是加密文件，走加密打开流程（弹出密码输入框）
    if (FileEncryption::isEncryptedFile(fileName)) {
        return openEncryptedFile(fileName);
    }

    // 检测文件大小，决定是否使用大文件管理器
    QFileInfo fileInfo(fileName);
    qint64 fileSize = fileInfo.size();

    // 如果文件较大，使用大文件管理器
    if (largeFileManager && fileSize >= largeFileManager->config().largeFileThreshold) {
        bool success = largeFileManager->openFile(fileName, ui->plainTextEdit);
        if (success) {
            // remember name of new file
            setFileName(fileName);

            // add to recent files
            recentFilesMenu->addFile(fileName);

            // 更新当前标签数据
            if (m_currentTabIndex >= 0 && m_currentTabIndex < m_tabs.size()) {
                m_tabs[m_currentTabIndex].filePath = fileName;
                m_tabs[m_currentTabIndex].content = ui->plainTextEdit->toPlainText();
                m_tabs[m_currentTabIndex].isModified = false;
                updateTabTitle(m_currentTabIndex);
            }

            return true;
        }
        // 如果大文件管理器加载失败，回退到普通加载
    }

    // 普通文件加载
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QIODevice::Text)) {
        return false;
    }

    // read content from file
    QByteArray content = file.readAll();
    QString text = QString::fromUtf8(content);

    ui->plainTextEdit->resetHighlighting();
    ui->plainTextEdit->setPlainText(text);

    // remember name of new file
    setFileName(fileName);

    // add to recent files
    recentFilesMenu->addFile(fileName);

    // 更新当前标签数据
    if (m_currentTabIndex >= 0 && m_currentTabIndex < m_tabs.size()) {
        m_tabs[m_currentTabIndex].filePath = fileName;
        m_tabs[m_currentTabIndex].content = text;
        m_tabs[m_currentTabIndex].isModified = false;
        updateTabTitle(m_currentTabIndex);
    }

    return true;
}

// 函数说明：实现 MainWindow::proxyConfigurationChanged 的核心逻辑，供当前模块调用。
void MainWindow::proxyConfigurationChanged()
{
    if (options->proxyMode() == Options::SystemProxy) {
        qDebug() << "Use system proxy configuration";
        QNetworkProxyFactory::setUseSystemConfiguration(true);
    } else if (options->proxyMode() == Options::ManualProxy) {
        qDebug() << "Use proxy" << options->proxyHost();
        QNetworkProxyFactory::setUseSystemConfiguration(false);

        QNetworkProxy proxy;
        proxy.setType(QNetworkProxy::HttpProxy);
        proxy.setHostName(options->proxyHost());
        proxy.setPort(options->proxyPort());
        proxy.setUser(options->proxyUser());
        proxy.setPassword(options->proxyPassword());
        QNetworkProxy::setApplicationProxy(proxy);
    } else {
        qDebug() << "Don't use a proxy";
        QNetworkProxyFactory::setUseSystemConfiguration(false);

        QNetworkProxy proxy;
        proxy.setType(QNetworkProxy::NoProxy);
        QNetworkProxy::setApplicationProxy(proxy);
    }
}

// 函数说明：实现 MainWindow::markdownConverterChanged 的核心逻辑，供当前模块调用。
void MainWindow::markdownConverterChanged()
{
    // regenerate HTML
    plainTextChanged();

    // disable unsupported extensions
    updateExtensionStatus();

    delete viewSynchronizer;
    switch (options->markdownConverter()) {
#ifdef ENABLE_HOEDOWN
    case Options::HoedownMarkdownConverter:
#endif
    case Options::DiscountMarkdownConverter:
    {
        auto *htmlViewSynchronizer = new HtmlViewSynchronizer(ui->webView, ui->plainTextEdit);
        viewSynchronizer = htmlViewSynchronizer;
        connect(generator, &HtmlPreviewGenerator::htmlResultReady,
                htmlViewSynchronizer, &HtmlViewSynchronizer::rememberScrollBarPos);
        break;
    }
    case Options::RevealMarkdownConverter:
        viewSynchronizer = new RevealViewSynchronizer(ui->webView, ui->plainTextEdit);
        break;
    default:
        viewSynchronizer = 0;
        break;
    }
}

// 函数说明：初始化 MainWindow 的 setupUi 相关界面、动作或服务连接。
void MainWindow::setupUi()
{
    htmlPreviewController = new HtmlPreviewController(ui->webView, this);

    setupActions();
    setupMarkdownEditor();
    setupHtmlPreview();
    setupHtmlSourceView();
    setupStatusBar();

    // hide find/replace widget on startup
    ui->findReplaceWidget->hide();
    connect(ui->findReplaceWidget, &FindReplaceWidget::dialogClosed,
            ui->plainTextEdit, QOverload<>::of(&QWidget::setFocus));

    // close table of contents dockwidget
    ui->dockWidget->close();

    // hide markdown syntax help dockwidget
    ui->dockWidget_2->hide();
    ui->dockWidget_2->setFloating(true);
    ui->dockWidget_2->resize(550, 400);

    // show HTML preview on right panel
    setHtmlSource(ui->actionHtmlSource->isChecked());
    //连接进主窗口
    connect(options, &Options::proxyConfigurationChanged,
            this, &MainWindow::proxyConfigurationChanged);
    connect(options, &Options::markdownConverterChanged,
            this, &MainWindow::markdownConverterChanged);
    connect(options, &Options::editorStyleChanged,
            this, &MainWindow::editorStyleChanged);

    // 设置数据可视化面板
    setupDataVisualizationPanel();

    // 设置安全功能
    setupSecurityFeatures();

    // 设置发布功能
    setupPublishingFeatures();

    // 设置工具功能
    setupToolsFeatures();

    // 设置导出扩展功能
    setupExportExtensions();

    // 设置内容管理功能
    setupContentManagement();

    // 设置焦点模式
    setupFocusMode();

    // 设置模板系统
    setupTemplateSystem();

    // 设置多标签编辑
    setupMultiTabEditor();

    // 设置协作和分享功能
    setupCollaborationFeatures();

    // 设置写作辅助功能
    setupWritingAssistance();

    // 设置预览和渲染功能
    setupPreviewFeatures();

    // 设置扩展性功能
    setupExtensionFeatures();

    // 设置 AI 智能写作助手
    setupAIWritingAssistant();

    // 设置高级搜索功能
    setupAdvancedSearch();

    // 设置时间线/历史记录可视化
    setupTimelineView();

    // 设置大文件处理优化
    setupLargeFileManager();

    // 设置内存优化
    setupMemoryOptimizer();

    readSettings();

    // 统一调整 Dock 布局，避免标签重叠（必须在 readSettings 之后，否则 restoreState 会覆盖）
    setupDockLayout();
    setupCustomShortcuts();

    ui->actionFullScreenMode->setChecked(this->isFullScreen());
}

void SetActionShortcut(QAction *action, const QKeySequence &shortcut)
{
    action->setShortcut(shortcut);
    action->setProperty("defaultshortcut", shortcut);
}

// 函数说明：初始化 MainWindow 的 setupActions 相关界面、动作或服务连接。
void MainWindow::setupActions()
{
    // file menu
    SetActionShortcut(ui->actionNew, QKeySequence::New);
    SetActionShortcut(ui->actionOpen, QKeySequence::Open);
    SetActionShortcut(ui->actionSave, QKeySequence::Save);
    SetActionShortcut(ui->actionSaveAs, QKeySequence::SaveAs);
    SetActionShortcut(ui->action_Print, QKeySequence::Print);
    SetActionShortcut(ui->actionExit, QKeySequence::Quit);

    recentFilesMenu = new RecentFilesMenu(ui->menuFile);
    ui->menuFile->insertMenu(ui->actionSave, recentFilesMenu);

    connect(recentFilesMenu, QOverload<const QString &>::of(&RecentFilesMenu::recentFileTriggered),
            this, &MainWindow::openRecentFile);

    // edit menu
    SetActionShortcut(ui->actionUndo, QKeySequence::Undo);
    SetActionShortcut(ui->actionRedo, QKeySequence::Redo);

    SetActionShortcut(ui->actionCut, QKeySequence::Cut);
    SetActionShortcut(ui->actionCopy, QKeySequence::Copy);
    SetActionShortcut(ui->actionPaste, QKeySequence::Paste);

    SetActionShortcut(ui->actionStrong, QKeySequence::Bold);
    SetActionShortcut(ui->actionEmphasize, QKeySequence::Italic);

    SetActionShortcut(ui->actionFindReplace, QKeySequence::Find);
    SetActionShortcut(ui->actionFindNext, QKeySequence::FindNext);
    SetActionShortcut(ui->actionFindPrevious, QKeySequence::FindPrevious);

    connect(ui->actionFindNext, &QAction::triggered,
            ui->findReplaceWidget, &FindReplaceWidget::findNextClicked);

    connect(ui->actionFindPrevious, &QAction::triggered,
            ui->findReplaceWidget, &FindReplaceWidget::findPreviousClicked);

    // view menu
    ui->menuView->insertAction(ui->menuView->actions()[0], ui->dockWidget->toggleViewAction());
    ui->menuView->insertAction(ui->menuView->actions()[1], ui->fileExplorerDockWidget->toggleViewAction());
    SetActionShortcut(ui->fileExplorerDockWidget->toggleViewAction(), QKeySequence(Qt::ALT | Qt::Key_E));
    SetActionShortcut(ui->actionFullScreenMode, QKeySequence::FullScreen);

    // extras menu
    connect(ui->actionMathSupport, &QAction::triggered,
            generator, &HtmlPreviewGenerator::setMathSupportEnabled);
    connect(ui->actionDiagramSupport, &QAction::triggered,
            generator, &HtmlPreviewGenerator::setDiagramSupportEnabled);
    connect(ui->actionCodeHighlighting, &QAction::triggered,
            generator, &HtmlPreviewGenerator::setCodeHighlightingEnabled);
    connect(ui->menuLanguages, QOverload<const Dictionary &>::of(&LanguageMenu::languageTriggered),
            this, &MainWindow::languageChanged);

    // help menu
    ui->actionMarkdownSyntax->setShortcut(QKeySequence::HelpContents);

    // set actions icons
    setActionsIcons();

    // set names for dock widget actions
    ui->dockWidget->toggleViewAction()->setObjectName("actionTableOfContents");
    ui->fileExplorerDockWidget->toggleViewAction()->setObjectName("actionFileExplorer");

    // setup default shortcuts
    ui->actionGotoLine->setProperty("defaultshortcut", ui->actionGotoLine->shortcut());
    ui->actionBlockquote->setProperty("defaultshortcut", ui->actionBlockquote->shortcut());
    ui->actionIncreaseHeaderLevel->setProperty("defaultshortcut", ui->actionIncreaseHeaderLevel->shortcut());
    ui->actionDecreaseHeaderLevel->setProperty("defaultshortcut", ui->actionDecreaseHeaderLevel->shortcut());
    ui->actionInsertTable->setProperty("defaultshortcut", ui->actionInsertTable->shortcut());
    ui->actionInsertImage->setProperty("defaultshortcut", ui->actionInsertImage->shortcut());
    ui->dockWidget->toggleViewAction()->setProperty("defaultshortcut", ui->dockWidget->toggleViewAction()->shortcut());
    ui->fileExplorerDockWidget->toggleViewAction()->setProperty("defaultshortcut", ui->fileExplorerDockWidget->toggleViewAction()->shortcut());
    ui->actionHtmlSource->setProperty("defaultshortcut", ui->actionHtmlSource->shortcut());
}

// 函数说明：设置 MainWindow 的运行参数，并触发必要的界面或数据刷新。
void MainWindow::setActionsIcons()
{
#ifndef Q_OS_MACOS
  // file menu
  ui->actionSave->setIcon(QIcon("fa-floppy-o.fontawesome"));
  ui->actionExportToPDF->setIcon(QIcon("fa-file-pdf-o.fontawesome"));
  ui->action_Print->setIcon(QIcon("fa-print.fontawesome"));

  // edit menu
  ui->actionUndo->setIcon(QIcon("fa-undo.fontawesome"));
  ui->actionRedo->setIcon(QIcon("fa-repeat.fontawesome"));

  ui->actionCut->setIcon(QIcon("fa-scissors.fontawesome"));
  ui->actionCopy->setIcon(QIcon("fa-files-o.fontawesome"));
  ui->actionPaste->setIcon(QIcon("fa-clipboard.fontawesome"));

  ui->actionStrong->setIcon(QIcon("fa-bold.fontawesome"));
  ui->actionEmphasize->setIcon(QIcon("fa-italic.fontawesome"));
  ui->actionStrikethrough->setIcon(QIcon("fa-strikethrough.fontawesome"));
  ui->actionCenterParagraph->setIcon(QIcon("fa-align-center.fontawesome"));
  ui->actionIncreaseHeaderLevel->setIcon(QIcon("fa-level-up.fontawesome"));
  ui->actionBlockquote->setIcon(QIcon("fa-quote-left.fontawesome"));
  ui->actionDecreaseHeaderLevel->setIcon(QIcon("fa-level-down.fontawesome"));

  ui->actionInsertTable->setIcon(QIcon("fa-table.fontawesome"));
  ui->actionInsertImage->setIcon(QIcon("fa-picture-o.fontawesome"));

  ui->actionFindReplace->setIcon(QIcon("fa-search.fontawesome"));

  // view menu
  ui->actionFullScreenMode->setIcon(QIcon("fa-arrows-alt.fontawesome"));

  ui->webView->pageAction(QWebEnginePage::Copy)->setIcon(QIcon("fa-copy.fontawesome"));
#endif
}

// 函数说明：初始化 MainWindow 的 setupStatusBar 相关界面、动作或服务连接。
void MainWindow::setupStatusBar()
{
    statusBarWidget = new StatusBarWidget(ui->plainTextEdit);
    statusBarWidget->setHtmlAction(ui->actionHtmlSource);

    connect(options, &Options::lineColumnEnabledChanged,
            statusBarWidget, &StatusBarWidget::showLineColumn);

    statusBarWidget->update();

    // remove border around statusbar widgets
    statusBar()->setStyleSheet("QStatusBar::item { border: 0px solid black }; ");
    statusBar()->addPermanentWidget(statusBarWidget, 1);
}

// 函数说明：初始化 MainWindow 的 setupMarkdownEditor 相关界面、动作或服务连接。
void MainWindow::setupMarkdownEditor()
{
    ui->plainTextEdit->setSnippetCompleter(new SnippetCompleter(snippetCollection, ui->plainTextEdit));

    // load file that are dropped on the editor
    connect(ui->plainTextEdit, &MarkdownEditor::loadDroppedFile,
            this, &MainWindow::load);

    connect(options, &Options::editorFontChanged,
            ui->plainTextEdit, &MarkdownEditor::editorFontChanged);
    connect(options, &Options::tabWidthChanged,
            ui->plainTextEdit, &MarkdownEditor::tabWidthChanged);
    connect(options, &Options::rulerEnabledChanged,
            ui->plainTextEdit, &MarkdownEditor::rulerEnabledChanged);
    connect(options, &Options::rulerPosChanged,
            ui->plainTextEdit, &MarkdownEditor::rulerPosChanged);
}

// 函数说明：初始化 MainWindow 的 setupHtmlPreview 相关界面、动作或服务连接。
void MainWindow::setupHtmlPreview()
{
    // Qt WebEngine doesn't have javaScriptWindowObjectCleared signal
    // JavaScript communication should use QWebChannel instead
    // For now, we'll skip this setup
    // TODO: Implement using QWebChannel

    // start background HTML preview generator
    connect(generator, &HtmlPreviewGenerator::htmlResultReady,
            this, &MainWindow::htmlResultReady);
    connect(generator, &HtmlPreviewGenerator::tocResultReady,
            this, &MainWindow::tocResultReady);
    generator->start();
}

// 函数说明：初始化 MainWindow 的 setupHtmlSourceView 相关界面、动作或服务连接。
void MainWindow::setupHtmlSourceView()
{
    QFont font("Monospace", 10);
    font.setStyleHint(QFont::TypeWriter);
    ui->htmlSourceTextEdit->setFont(font);
    htmlHighlighter = new HtmlHighlighter(ui->htmlSourceTextEdit->document());
}

// 函数说明：初始化 MainWindow 的 setupCustomShortcuts 相关界面、动作或服务连接。
void MainWindow::setupCustomShortcuts()
{
    setCustomShortcut(ui->menuFile);
    setCustomShortcut(ui->menuEdit);
    setCustomShortcut(ui->menuView);

    for (QAction *action : ui->plainTextEdit->actions()) {
        setCustomShortcut(action);
    }
}

// 函数说明：设置 MainWindow 的运行参数，并触发必要的界面或数据刷新。
void MainWindow::setCustomShortcut(QMenu *menu)
{
    for (QAction *action : menu->actions()) {
        if (action->menu()) {
            // recurse into submenu
            setCustomShortcut(action->menu());
        } else {
            setCustomShortcut(action);
        }
    }
}

// 函数说明：设置 MainWindow 的运行参数，并触发必要的界面或数据刷新。
void MainWindow::setCustomShortcut(QAction *action)
{
    if (options->hasCustomShortcut(action->objectName())) {
        action->setShortcut(options->customShortcut(action->objectName()));
    }
}

// 函数说明：刷新 MainWindow 的内部状态，并同步到相关界面。
void MainWindow::updateExtensionStatus()
{
    ui->actionAutolink->setEnabled(generator->isSupported(MarkdownConverter::AutolinkOption));
    ui->actionAlphabeticLists->setEnabled(generator->isSupported(MarkdownConverter::NoAlphaListOption));
    ui->actionDefinitionLists->setEnabled(generator->isSupported(MarkdownConverter::NoDefinitionListOption));
    ui->actionFootnotes->setEnabled(generator->isSupported(MarkdownConverter::ExtraFootnoteOption));
    ui->actionSmartyPants->setEnabled(generator->isSupported(MarkdownConverter::NoSmartypantsOption));
    ui->actionStrikethroughOption->setEnabled(generator->isSupported(MarkdownConverter::NoStrikethroughOption));
    ui->actionSuperscript->setEnabled(generator->isSupported(MarkdownConverter::NoSuperscriptOption));
}

// 函数说明：处理云同步操作，把本地文档状态同步到配置的远端。
void MainWindow::syncWebViewToHtmlSource()
{
    htmlResultReady(ui->htmlSourceTextEdit->toPlainText());
}

// 函数说明：实现 MainWindow::maybeSave 的核心逻辑，供当前模块调用。
bool MainWindow::maybeSave()
{
    if (!ui->plainTextEdit->document()->isModified())
        return true;

    if (fileName.startsWith(QLatin1String(":/")))
        return true;

    QMessageBox::StandardButton ret;
    ret = QMessageBox::warning(this, tr("Save Changes"),
                               tr("The document has been modified.<br>"
                                  "Do you want to save your changes?"),
                               QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (ret == QMessageBox::Save)
        return fileSave();
    else if (ret == QMessageBox::Cancel)
        return false;

    return true;
}

// 函数说明：设置 MainWindow 的运行参数，并触发必要的界面或数据刷新。
void MainWindow::setFileName(const QString &fileName)
{
    this->fileName = fileName;

    // set to unmodified
    ui->plainTextEdit->document()->setModified(false);
    setWindowModified(false);

    // update window title
    QString shownName = fileName;
    if (shownName.isEmpty()) {
        //: default file name for new markdown documents
        shownName = tr("untitled.md");
    }
    setWindowFilePath(shownName);
}

// 函数说明：刷新 MainWindow 的内部状态，并同步到相关界面。
void MainWindow::updateSplitter()
{
    // not fully initialized yet?
    if (centralWidget()->size() != ui->splitter->size()) {
        return;
    }

    // calculate new width of left and right pane
    QList<int> childSizes = ui->splitter->sizes();
    childSizes[0] = ui->splitter->width() * splitFactor;
    childSizes[1] = ui->splitter->width() * (1 - splitFactor);

    ui->splitter->setSizes(childSizes);
}

// 函数说明：初始化 MainWindow 的 setupHtmlPreviewThemes 相关界面、动作或服务连接。
void MainWindow::setupHtmlPreviewThemes()
{
    ui->menuStyles->clear();

    delete stylesGroup;
    stylesGroup = new QActionGroup(this);

    int key = 1;
    bool separatorAdded = false;
    for (const QString &themeName : themeCollection->themeNames()) {
        if (!separatorAdded && !themeCollection->theme(themeName).isBuiltIn()) {
            addSeparatorAfterBuiltInThemes();
            separatorAdded = true;
        }

        QAction *action = ui->menuStyles->addAction(themeName);
        action->setShortcut(QKeySequence(tr("Ctrl+%1").arg(key++)));
        action->setCheckable(true);
        action->setActionGroup(stylesGroup);
        connect(action, &QAction::triggered,
                this, &MainWindow::themeChanged);
    }

    if (statusBarWidget)
        statusBarWidget->setStyleActions(stylesGroup);
}

// 函数说明：向 MainWindow 管理的数据集合中添加一项内容。
void MainWindow::addSeparatorAfterBuiltInThemes()
{
    ui->menuStyles->addSeparator();

    QAction *separator = new QAction(stylesGroup);
    separator->setSeparator(true);
    stylesGroup->addAction(separator);
}

// 函数说明：加载 MainWindow 需要的数据、配置或外部资源。
void MainWindow::loadCustomStyles()
{
    QStringList paths = DataLocation::standardLocations();
    qDebug() << paths;
    QDir dataPath(paths.first() + QDir::separator() + "styles");
    dataPath.setFilter(QDir::Files);
    if (dataPath.exists()) {
        // iterate over all files in the styles subdirectory
        QDirIterator it(dataPath);
        while (it.hasNext()) {
            it.next();

            QString fileName = it.fileName();
            QString styleName = QFileInfo(fileName).baseName();
            QString stylePath = QUrl::fromLocalFile(it.filePath()).toString();

            Theme customTheme { styleName, "Default", "Default", styleName };
            themeCollection->insert(customTheme);

            StyleManager styleManager;
            styleManager.insertCustomPreviewStylesheet(styleName, stylePath);
        }
    }
}
// 读取程序配置：窗口状态、标签页、最近文件等
void MainWindow::readSettings()
{
    // restore window size, position and state
    // /恢复窗口大小、位置、最大化/最小化状态
    QSettings settings; // Qt配置对象，自动读取保存的配置
    // 恢复窗口几何信息（位置+大小）
    restoreGeometry(settings.value("mainWindow/geometry").toByteArray());
    //恢复窗口状态
    restoreState(settings.value("mainWindow/windowState").toByteArray());

    // restore recent files menu
    //恢复最近打开文件菜单
    recentFilesMenu->readState();

    //读取全局配置（字体、样式、编辑器选项）
    options->readSettings();

    // 恢复标签会话
    QStringList tabPaths = settings.value("tabs/filePaths").toStringList();
    int savedCurrentIndex = settings.value("tabs/currentIndex", 0).toInt();

    if (!tabPaths.isEmpty()) {
        // 清除初始的空白标签
        m_tabBar->blockSignals(true);

        // 加载保存的标签
        bool firstTab = true;
        for (const QString &path : tabPaths) {
            if (path.isEmpty()) {
                if (firstTab) {
                    // 第一个空白标签已经存在
                    firstTab = false;
                    continue;
                }
                // 添加新的空白标签
                TabData newTab;
                m_tabs.append(newTab);
                m_tabBar->addTab(tr("untitled.md"));
            } else if (QFile::exists(path)) {
                if (firstTab) {
                    // 使用第一个标签加载文件
                    firstTab = false;
                    m_currentTabIndex = 0;
                    load(path);
                } else {
                    // 添加新标签并加载文件
                    TabData newTab;
                    m_tabs.append(newTab);
                    int newIndex = m_tabBar->addTab(QFileInfo(path).fileName());
                    m_currentTabIndex = newIndex;
                    // 保存上一个标签状态，加载新文件
                    load(path);
                }
            }
        }

        // 恢复到保存的当前标签索引
        if (savedCurrentIndex >= 0 && savedCurrentIndex < m_tabs.size()) {
            m_currentTabIndex = savedCurrentIndex;
            m_tabBar->setCurrentIndex(savedCurrentIndex);
            restoreTabState(savedCurrentIndex);
        } else if (!m_tabs.isEmpty()) {
            m_currentTabIndex = 0;
            m_tabBar->setCurrentIndex(0);
            restoreTabState(0);
        }

        m_tabBar->blockSignals(false);
    }

    // restore blog publishing config from user data file
    //恢复博客发布配置
    loadBlogPublisherConfig(blogPublisher);
}
// 保存程序配置：窗口、标签、最近文件、全局设置等
void MainWindow::writeSettings()
{
    //保存全局配置（字体、样式、编辑器设置）
    options->writeSettings();

    // save recent files menu
    // 保存最近打开文件列表
    recentFilesMenu->saveState();

    // save window size, position and state
    //保存窗口大小、位置、状态
    QSettings settings;
    settings.setValue("mainWindow/geometry", saveGeometry());
    settings.setValue("mainWindow/windowState", saveState());

    // 保存标签会话
    saveCurrentTabState();
    // 把所有标签的文件路径收集起来，保存到配置
    QStringList tabPaths;
    for (const TabData &tab : m_tabs) {
        tabPaths.append(tab.filePath);
    }
    settings.setValue("tabs/filePaths", tabPaths);// 保存所有标签路径
    settings.setValue("tabs/currentIndex", m_currentTabIndex);// 保存当前激活标签索引

    // persist blog publishing config to user data file
    //保存博客发布配置
    saveBlogPublisherConfig(blogPublisher);
}

// 函数说明：实现 MainWindow::stylePath 的核心逻辑，供当前模块调用。
QString MainWindow::stylePath(const QString &styleName)
{
    QString suffix = options->isSourceAtSingleSizeEnabled() ? "" : "+";
    return QString(":/theme/%1%2.txt").arg(styleName).arg(suffix);
}

// 函数说明：初始化 MainWindow 的 setupDataVisualizationPanel 相关界面、动作或服务连接。
void MainWindow::setupDataVisualizationPanel()
{
    // 创建数据可视化面板
    dataVisualizationPanel = new DataVisualizationPanel(this);
    
    // 创建停靠窗口
    dataVisualizationDock = new QDockWidget(tr("数据可视化"), this);
    dataVisualizationDock->setObjectName("dataVisualizationDock");
    dataVisualizationDock->setWidget(dataVisualizationPanel);
    dataVisualizationDock->setAllowedAreas(Qt::LeftDockWidgetArea | 
                                           Qt::RightDockWidgetArea | 
                                           Qt::BottomDockWidgetArea);
    
    // 添加到右侧
    addDockWidget(Qt::RightDockWidgetArea, dataVisualizationDock);
    
    // 初始隐藏
    dataVisualizationDock->hide();
    
    // 添加菜单动作
    QAction *toggleAction = dataVisualizationDock->toggleViewAction();
    toggleAction->setText(tr("数据可视化面板"));
    toggleAction->setShortcut(QKeySequence("Ctrl+Shift+D"));
    ui->menuView->addSeparator();
    ui->menuView->addAction(toggleAction);
    
    // 连接信号
    connect(dataVisualizationPanel, &DataVisualizationPanel::blockClicked,
            this, &MainWindow::onDataVisualizationBlockClicked);
    connect(dataVisualizationPanel, &DataVisualizationPanel::requestLocateLine,
            this, [this](int line) {
                // 跳转到编辑器对应行
                QTextCursor cursor = ui->plainTextEdit->textCursor();
                cursor.movePosition(QTextCursor::Start);
                cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, line - 1);
                ui->plainTextEdit->setTextCursor(cursor);
                ui->plainTextEdit->centerCursor();
            });
}

// 函数说明：处理视图切换逻辑，同步窗口布局和可见状态。
void MainWindow::viewToggleDataVisualization(bool checked)
{
    if (dataVisualizationDock) {
        dataVisualizationDock->setVisible(checked);
        if (checked) {
            updateDataVisualization();
        }
    }
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onDataVisualizationBlockClicked(int startLine, int endLine)
{
    // 跳转到编辑器对应行
    QTextCursor cursor = ui->plainTextEdit->textCursor();
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, startLine - 1);
    ui->plainTextEdit->setTextCursor(cursor);
    ui->plainTextEdit->centerCursor();
    ui->plainTextEdit->setFocus();
}

// 函数说明：刷新 MainWindow 的内部状态，并同步到相关界面。
void MainWindow::updateDataVisualization()
{
    if (dataVisualizationPanel && dataVisualizationDock && 
        dataVisualizationDock->isVisible()) {
        QString code = ui->plainTextEdit->toPlainText();
        dataVisualizationPanel->setMarkdownContent(code);
    }
}

// ==================== 安全功能 ====================

void MainWindow::setupSecurityFeatures()
{
    // 创建安全文档管理器
    secureDocument = new SecureDocument(this);
    
    connect(secureDocument, &SecureDocument::documentExpired,
            this, &MainWindow::onDocumentExpired);
    connect(secureDocument, &SecureDocument::idleTimeout,
            this, &MainWindow::onDocumentLocked);
    connect(secureDocument, &SecureDocument::expirationWarning,
            this, [this](int remainingSeconds) {
                statusBar()->showMessage(
                    tr("警告: 文档将在 %1 秒后过期").arg(remainingSeconds), 5000);
            });

    // 创建安全菜单
    securityMenu = new QMenu(tr("安全(&S)"), this);
    menuBar()->insertMenu(ui->menuHelp->menuAction(), securityMenu);

    // 加密当前文件
    actionEncrypt = new QAction(tr("加密文件..."), this);
    actionEncrypt->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E));
    actionEncrypt->setIcon(QIcon("fa-lock.fontawesome"));
    connect(actionEncrypt, &QAction::triggered, this, &MainWindow::fileEncrypt);
    securityMenu->addAction(actionEncrypt);

    // 解密文件
    actionDecrypt = new QAction(tr("解密文件..."), this);
    actionDecrypt->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_D));
    actionDecrypt->setIcon(QIcon("fa-unlock.fontawesome"));
    connect(actionDecrypt, &QAction::triggered, this, &MainWindow::fileDecrypt);
    securityMenu->addAction(actionDecrypt);

    securityMenu->addSeparator();

    // 打开加密文件
    QAction *actionOpenEncrypted = new QAction(tr("打开加密文件..."), this);
    actionOpenEncrypted->setIcon(QIcon("fa-folder-open.fontawesome"));
    connect(actionOpenEncrypted, &QAction::triggered, this, &MainWindow::fileOpenEncrypted);
    securityMenu->addAction(actionOpenEncrypted);

    securityMenu->addSeparator();

    // 生物特征设置
    if (BiometricAuth::instance().isHardwareAvailable()) {
        QAction *actionBiometric = new QAction(
            tr("使用 %1 验证").arg(BiometricAuth::platformName()), this);
        actionBiometric->setCheckable(true);
        actionBiometric->setChecked(true);
        securityMenu->addAction(actionBiometric);
    }

    securityMenu->addSeparator();

    // 安全设置
    actionSecuritySettings = new QAction(tr("安全设置..."), this);
    actionSecuritySettings->setIcon(QIcon("fa-cog.fontawesome"));
    connect(actionSecuritySettings, &QAction::triggered, 
            this, &MainWindow::securitySettings);
    securityMenu->addAction(actionSecuritySettings);

    // 检查生物特征可用性
    checkBiometricAvailability();
}

// 函数说明：处理主窗口的文件菜单动作，衔接文档读写和界面状态。
void MainWindow::fileEncrypt()
{
    if (fileName.isEmpty()) {
        QMessageBox::warning(this, tr("加密文件"),
            tr("请先保存文件后再进行加密。"));
        return;
    }

    // 先保存当前内容
    if (!fileSave()) {
        return;
    }

    // 显示加密对话框
    EncryptFileDialog dialog(fileName, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QString password = dialog.password();
    FileEncryption::EncryptionOptions options = dialog.options();

    // 执行加密
    bool success = SecurityManager::instance().encryptFile(fileName, password, options);

    if (success) {
        isEncryptedMode = true;

        // 如果启用了生物特征解锁，将密码保存到系统钥匙串
        if (options.requireBiometric) {
            QString storageKey = QString("file:%1").arg(QFileInfo(fileName).absoluteFilePath());
            SecurityManager::instance().savePassword(storageKey, password);
        }

        // 设置安全文档管理器，使后续保存可以正常工作
        if (secureDocument->state() != SecureDocument::State::Closed) {
            secureDocument->close();
        }
        secureDocument->create(fileName, password);
        secureDocument->setContent(ui->plainTextEdit->toPlainText());

        statusBar()->showMessage(tr("文件已加密"), 3000);

        // 更新窗口标题
        setWindowTitle(tr("%1 [已加密] - CuteMarkEd").arg(QFileInfo(fileName).fileName()));
    } else {
        QMessageBox::critical(this, tr("加密失败"),
            tr("无法加密文件: %1").arg(SecurityManager::instance().lastError()));
    }
}

// 函数说明：处理主窗口的文件菜单动作，衔接文档读写和界面状态。
void MainWindow::fileDecrypt()
{
    QString filePath = QFileDialog::getOpenFileName(this, 
        tr("选择加密文件"),
        QString(),
        tr("加密文件 (*.md *.markdown);;所有文件 (*)"));

    if (filePath.isEmpty()) {
        return;
    }

    if (!FileEncryption::isEncryptedFile(filePath)) {
        QMessageBox::information(this, tr("解密文件"),
            tr("所选文件未加密。"));
        return;
    }

    // 显示解密对话框
    DecryptFileDialog dialog(filePath, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QString outputPath = filePath + ".decrypted";
    bool success = SecurityManager::instance().decryptFile(
        filePath, dialog.password(), outputPath);

    if (success) {
        QMessageBox::information(this, tr("解密成功"),
            tr("文件已解密并保存为:\n%1").arg(outputPath));
    } else {
        QMessageBox::critical(this, tr("解密失败"),
            tr("无法解密文件: %1").arg(SecurityManager::instance().lastError()));
    }
}

// 函数说明：处理主窗口的文件菜单动作，衔接文档读写和界面状态。
void MainWindow::fileOpenEncrypted()
{
    if (!maybeSave()) {
        return;
    }

    QString filePath = QFileDialog::getOpenFileName(this,
        tr("打开加密文件"),
        QString(),
        tr("加密文件 (*.md *.markdown);;所有文件 (*)"));

    if (filePath.isEmpty()) {
        return;
    }

    openEncryptedFile(filePath);
}

// 函数说明：打开 MainWindow 对应的文件、资源或功能入口。
bool MainWindow::openEncryptedFile(const QString &filePath)
{
    if (!FileEncryption::isEncryptedFile(filePath)) {
        // 普通文件，使用标准打开
        return load(filePath);
    }

    // 显示解密对话框
    DecryptFileDialog dialog(filePath, this);
    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    // 获取密码：Touch ID 模式从钥匙串获取，普通模式从输入框获取
    QString password;
    if (dialog.useBiometric()) {
        QString storageKey = QString("file:%1").arg(QFileInfo(filePath).absoluteFilePath());
        password = SecurityManager::instance().loadPassword(storageKey);
        if (password.isEmpty()) {
            QMessageBox::warning(this, tr("解密失败"),
                tr("无法从系统钥匙串获取密码。\n"
                   "可能是首次加密时未启用 Touch ID，请使用密码解密。"));
            return false;
        }
    } else {
        password = dialog.password();
    }

    // 尝试打开加密文档
    if (!secureDocument->open(filePath, password)) {
        QMessageBox::critical(this, tr("打开失败"),
            tr("无法打开加密文件: %1").arg(secureDocument->lastError()));
        return false;
    }

    // 设置编辑器内容
    ui->plainTextEdit->setPlainText(secureDocument->content());
    
    // 更新状态
    isEncryptedMode = true;
    setFileName(filePath);
    setWindowTitle(tr("%1 [已加密] - CuteMarkEd")
        .arg(QFileInfo(filePath).fileName()));

    // 连接内容变化信号
    connect(ui->plainTextEdit, &QPlainTextEdit::textChanged,
            this, [this]() {
                if (isEncryptedMode && secureDocument->isOpen()) {
                    secureDocument->setContent(ui->plainTextEdit->toPlainText());
                }
            });

    return true;
}

// 函数说明：保存 MainWindow 当前状态，保证用户修改可以持久化。
bool MainWindow::saveAsEncrypted(const QString &password)
{
    if (!secureDocument) {
        return false;
    }

    secureDocument->setContent(ui->plainTextEdit->toPlainText());
    return secureDocument->save();
}

// 函数说明：实现 MainWindow::securitySettings 的核心逻辑，供当前模块调用。
void MainWindow::securitySettings()
{
    // 创建安全设置对话框
    QDialog dialog(this);
    dialog.setWindowTitle(tr("安全设置"));
    dialog.setMinimumWidth(450);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    // 加密设置组
    QGroupBox *encryptGroup = new QGroupBox(tr("加密设置"), &dialog);
    QFormLayout *encryptLayout = new QFormLayout(encryptGroup);

    QSpinBox *iterationsSpin = new QSpinBox(&dialog);
    iterationsSpin->setRange(10000, 1000000);
    iterationsSpin->setSingleStep(10000);
    iterationsSpin->setValue(SecurityManager::instance().settings().pbkdf2Iterations);
    iterationsSpin->setToolTip(tr("更高的迭代次数更安全，但解密更慢"));
    encryptLayout->addRow(tr("PBKDF2 迭代次数:"), iterationsSpin);

    QCheckBox *secureDeleteCheck = new QCheckBox(tr("使用安全删除（多次覆写）"), &dialog);
    secureDeleteCheck->setChecked(SecurityManager::instance().settings().secureDelete);
    encryptLayout->addRow(secureDeleteCheck);

    QSpinBox *deletePassesSpin = new QSpinBox(&dialog);
    deletePassesSpin->setRange(1, 7);
    deletePassesSpin->setValue(SecurityManager::instance().settings().secureDeletePasses);
    encryptLayout->addRow(tr("覆写次数:"), deletePassesSpin);

    layout->addWidget(encryptGroup);

    // 生物特征设置组
    QGroupBox *biometricGroup = new QGroupBox(tr("生物特征认证"), &dialog);
    QVBoxLayout *biometricLayout = new QVBoxLayout(biometricGroup);

    BiometricAuth::Availability avail = BiometricAuth::instance().checkAvailability();
    QString availText;
    switch (avail) {
        case BiometricAuth::Availability::Available:
            availText = tr("✓ %1 可用").arg(BiometricAuth::platformName());
            break;
        case BiometricAuth::Availability::NotAvailable:
            availText = tr("✗ 硬件不支持");
            break;
        case BiometricAuth::Availability::NotConfigured:
            availText = tr("⚠ 未配置生物特征");
            break;
        default:
            availText = tr("? 未知状态");
            break;
    }
    biometricLayout->addWidget(new QLabel(availText, &dialog));

    QCheckBox *enableBiometricCheck = new QCheckBox(tr("启用生物特征认证"), &dialog);
    enableBiometricCheck->setChecked(SecurityManager::instance().settings().enableBiometric);
    enableBiometricCheck->setEnabled(avail == BiometricAuth::Availability::Available);
    biometricLayout->addWidget(enableBiometricCheck);

    if (avail == BiometricAuth::Availability::Available) {
        QPushButton *testBiometricBtn = new QPushButton(tr("测试生物特征"), &dialog);
        connect(testBiometricBtn, &QPushButton::clicked, this, [this]() {
            BiometricAuth::instance().authenticate(tr("测试生物特征认证"),
                [this](BiometricAuth::AuthResult result, const QString &) {
                    if (result == BiometricAuth::AuthResult::Success) {
                        QMessageBox::information(this, tr("测试成功"),
                            tr("生物特征认证成功！"));
                    } else {
                        QMessageBox::warning(this, tr("测试失败"),
                            tr("生物特征认证失败。"));
                    }
                });
        });
        biometricLayout->addWidget(testBiometricBtn);
    }

    layout->addWidget(biometricGroup);

    // 自动锁定设置
    QGroupBox *lockGroup = new QGroupBox(tr("自动锁定"), &dialog);
    QFormLayout *lockLayout = new QFormLayout(lockGroup);

    QCheckBox *rememberPasswordCheck = new QCheckBox(tr("记住密码"), &dialog);
    rememberPasswordCheck->setChecked(SecurityManager::instance().settings().rememberPassword);
    lockLayout->addRow(rememberPasswordCheck);

    QSpinBox *passwordTimeoutSpin = new QSpinBox(&dialog);
    passwordTimeoutSpin->setRange(1, 60);
    passwordTimeoutSpin->setSuffix(tr(" 分钟"));
    passwordTimeoutSpin->setValue(SecurityManager::instance().settings().passwordTimeout);
    lockLayout->addRow(tr("密码超时:"), passwordTimeoutSpin);

    layout->addWidget(lockGroup);

    // 按钮
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    if (dialog.exec() == QDialog::Accepted) {
        // 保存设置
        SecurityManager::SecuritySettings settings;
        settings.pbkdf2Iterations = iterationsSpin->value();
        settings.secureDelete = secureDeleteCheck->isChecked();
        settings.secureDeletePasses = deletePassesSpin->value();
        settings.enableBiometric = enableBiometricCheck->isChecked();
        settings.rememberPassword = rememberPasswordCheck->isChecked();
        settings.passwordTimeout = passwordTimeoutSpin->value();
        
        SecurityManager::instance().setSettings(settings);
        
        statusBar()->showMessage(tr("安全设置已保存"), 3000);
    }
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onDocumentExpired()
{
    QMessageBox::warning(this, tr("文档已过期"),
        tr("此加密文档已过期，将被关闭。"));
    
    fileNew();
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onDocumentLocked()
{
    if (!isEncryptedMode) {
        return;
    }

    QMessageBox::information(this, tr("文档已锁定"),
        tr("由于空闲超时，文档已被锁定。\n请重新输入密码以继续编辑。"));

    // 显示解锁对话框
    DecryptFileDialog dialog(fileName, this);
    dialog.setWindowTitle(tr("解锁文档"));
    
    if (dialog.exec() == QDialog::Accepted) {
        if (secureDocument->unlock(dialog.password())) {
            statusBar()->showMessage(tr("文档已解锁"), 3000);
        } else {
            QMessageBox::critical(this, tr("解锁失败"),
                tr("密码错误，文档仍处于锁定状态。"));
        }
    }
}

// 函数说明：实现 MainWindow::checkBiometricAvailability 的核心逻辑，供当前模块调用。
void MainWindow::checkBiometricAvailability()
{
    BiometricAuth::Availability avail = BiometricAuth::instance().checkAvailability();

    if (avail == BiometricAuth::Availability::Available) {
        statusBar()->showMessage(
            tr("%1 可用").arg(BiometricAuth::platformName()), 3000);
    }
}

// ==================== 发布功能 ====================

void MainWindow::setupPublishingFeatures()
{
    // 初始化发布组件
    epubExporter = new EpubExporter(this);
    blogPublisher = new BlogPublisher(this);
    slideshowPresenter = new SlideshowPresenter();
    staticSiteGenerator = new StaticSiteGenerator(this);

    // 连接信号
    connect(blogPublisher, &BlogPublisher::postPublished,
            this, &MainWindow::onBlogPublished);
    connect(slideshowPresenter, &SlideshowPresenter::presentationEnded,
            this, &MainWindow::onSlideshowEnded);

    // 创建发布菜单
    publishMenu = new QMenu(tr("发布(&P)"), this);
    menuBar()->insertMenu(ui->menuHelp->menuAction(), publishMenu);

    // 导出为 EPUB
    actionExportEpub = new QAction(tr("导出为 EPUB..."), this);
    actionExportEpub->setIcon(QIcon("fa-book.fontawesome"));
    actionExportEpub->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_B));
    connect(actionExportEpub, &QAction::triggered, this, &MainWindow::publishExportToEpub);
    publishMenu->addAction(actionExportEpub);

    publishMenu->addSeparator();

    // 发布到博客
    actionPublishBlog = new QAction(tr("发布到博客..."), this);
    actionPublishBlog->setIcon(QIcon("fa-rss.fontawesome"));
    actionPublishBlog->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P));
    connect(actionPublishBlog, &QAction::triggered, this, &MainWindow::publishToBlog);
    publishMenu->addAction(actionPublishBlog);

    // 博客设置
    QAction *actionBlogSettings = new QAction(tr("博客设置..."), this);
    actionBlogSettings->setIcon(QIcon("fa-cog.fontawesome"));
    connect(actionBlogSettings, &QAction::triggered, this, &MainWindow::publishShowBlogSettings);
    publishMenu->addAction(actionBlogSettings);

    publishMenu->addSeparator();

    // 幻灯片演示
    actionSlideshow = new QAction(tr("开始演示"), this);
    actionSlideshow->setIcon(QIcon("fa-desktop.fontawesome"));
    actionSlideshow->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_F5));
    connect(actionSlideshow, &QAction::triggered, this, &MainWindow::publishStartSlideshow);
    publishMenu->addAction(actionSlideshow);

    publishMenu->addSeparator();

    // 发布到静态网站
    actionStaticSite = new QAction(tr("发布到静态网站..."), this);
    actionStaticSite->setIcon(QIcon("fa-globe.fontawesome"));
    connect(actionStaticSite, &QAction::triggered, this, &MainWindow::publishToStaticSite);
    publishMenu->addAction(actionStaticSite);

    // 静态网站管理
    QAction *actionStaticSiteManager = new QAction(tr("静态网站管理..."), this);
    actionStaticSiteManager->setIcon(QIcon("fa-cogs.fontawesome"));
    connect(actionStaticSiteManager, &QAction::triggered, this, &MainWindow::publishShowStaticSiteManager);
    publishMenu->addAction(actionStaticSiteManager);
}

// 函数说明：处理发布流程，将当前文档输出到电子书、博客或静态站点。
void MainWindow::publishExportToEpub()
{
    if (ui->plainTextEdit->document()->isEmpty()) {
        QMessageBox::warning(this, tr("导出 EPUB"),
            tr("请先编写一些内容再导出。"));
        return;
    }

    // 显示 EPUB 导出对话框
    QDialog dialog(this);
    dialog.setWindowTitle(tr("导出为 EPUB"));
    dialog.setMinimumWidth(400);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    // 元数据组
    QGroupBox *metadataGroup = new QGroupBox(tr("书籍信息"), &dialog);
    QFormLayout *metadataLayout = new QFormLayout(metadataGroup);

    QLineEdit *titleEdit = new QLineEdit(metadataGroup);
    titleEdit->setText(QFileInfo(fileName).baseName());
    metadataLayout->addRow(tr("标题:"), titleEdit);

    QLineEdit *authorEdit = new QLineEdit(metadataGroup);
    metadataLayout->addRow(tr("作者:"), authorEdit);

    QLineEdit *languageEdit = new QLineEdit(metadataGroup);
    languageEdit->setText("zh-CN");
    metadataLayout->addRow(tr("语言:"), languageEdit);

    QLineEdit *publisherEdit = new QLineEdit(metadataGroup);
    metadataLayout->addRow(tr("出版社:"), publisherEdit);

    QLineEdit *identifierEdit = new QLineEdit(metadataGroup);
    identifierEdit->setPlaceholderText(tr("URN/ISBN（留空自动生成）"));
    auto makeUuidIdentifier = []() -> QString {
        return QStringLiteral("urn:uuid:%1")
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    };
    identifierEdit->setText(makeUuidIdentifier());

    QPushButton *regenUuidButton = new QPushButton(tr("生成 UUID"), metadataGroup);
    QWidget *identifierRow = new QWidget(metadataGroup);
    QHBoxLayout *identifierLayout = new QHBoxLayout(identifierRow);
    identifierLayout->setContentsMargins(0, 0, 0, 0);
    identifierLayout->setSpacing(6);
    identifierLayout->addWidget(identifierEdit, 1);
    identifierLayout->addWidget(regenUuidButton);
    metadataLayout->addRow(tr("标识符:"), identifierRow);
    connect(regenUuidButton, &QPushButton::clicked, identifierEdit, [identifierEdit, makeUuidIdentifier]() {
        identifierEdit->setText(makeUuidIdentifier());
    });

    QTextEdit *descriptionEdit = new QTextEdit(metadataGroup);
    descriptionEdit->setMaximumHeight(80);
    metadataLayout->addRow(tr("简介:"), descriptionEdit);

    layout->addWidget(metadataGroup);

    // 选项组
    QGroupBox *optionsGroup = new QGroupBox(tr("导出选项"), &dialog);
    QFormLayout *optionsLayout = new QFormLayout(optionsGroup);

    QCheckBox *splitByHeadingCheck = new QCheckBox(tr("按标题拆分章节"), optionsGroup);
    splitByHeadingCheck->setChecked(true);
    optionsLayout->addRow(splitByHeadingCheck);

    QSpinBox *splitLevelSpin = new QSpinBox(optionsGroup);
    splitLevelSpin->setRange(1, 3);
    splitLevelSpin->setValue(2);
    optionsLayout->addRow(tr("拆分级别:"), splitLevelSpin);

    QCheckBox *generateTocCheck = new QCheckBox(tr("生成目录"), optionsGroup);
    generateTocCheck->setChecked(true);
    optionsLayout->addRow(generateTocCheck);

    QComboBox *cssProfileCombo = new QComboBox(optionsGroup);
    cssProfileCombo->addItem(tr("严格兼容（推荐）"),
                             static_cast<int>(EpubExporter::CssProfile::StrictCompatibility));
    cssProfileCombo->addItem(tr("现代样式"),
                             static_cast<int>(EpubExporter::CssProfile::Modern));
    optionsLayout->addRow(tr("CSS 模式:"), cssProfileCombo);

    layout->addWidget(optionsGroup);

    // 按钮
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    // 选择输出文件
    QString outputPath = QFileDialog::getSaveFileName(this,
        tr("保存 EPUB"),
        QFileInfo(fileName).baseName() + ".epub",
        tr("EPUB 文件 (*.epub)"));

    if (outputPath.isEmpty()) {
        return;
    }

    // 设置元数据
    EpubExporter::BookMetadata metadata;
    metadata.title = titleEdit->text();
    metadata.author = authorEdit->text();
    metadata.language = languageEdit->text();
    metadata.publisher = publisherEdit->text();
    metadata.description = descriptionEdit->toPlainText();
    metadata.identifier = identifierEdit->text().trimmed();
    epubExporter->setMetadata(metadata);

    // 设置选项
    EpubExporter::ExportOptions exportOptions;
    exportOptions.splitByHeading = splitByHeadingCheck->isChecked();
    exportOptions.splitLevel = splitLevelSpin->value();
    exportOptions.generateTOC = generateTocCheck->isChecked();
    exportOptions.cssProfile = static_cast<EpubExporter::CssProfile>(
        cssProfileCombo->currentData().toInt());
    epubExporter->setOptions(exportOptions);

    // 导出
    QString markdown = ui->plainTextEdit->toPlainText();
    if (epubExporter->exportFromMarkdown(markdown, outputPath)) {
        const QString finalIdentifier = epubExporter->metadata().identifier;
        QMessageBox::information(this, tr("导出成功"),
            tr("EPUB 已成功导出到:\n%1\n\n标识符: %2").arg(outputPath, finalIdentifier));
    } else {
        QMessageBox::critical(this, tr("导出失败"),
            tr("无法导出 EPUB: %1").arg(epubExporter->lastError()));
    }
}

// 函数说明：处理发布流程，将当前文档输出到电子书、博客或静态站点。
void MainWindow::publishToBlog()
{
    if (ui->plainTextEdit->document()->isEmpty()) {
        QMessageBox::warning(this, tr("发布到博客"),
            tr("请先编写一些内容再发布。"));
        return;
    }

    // 显示发布对话框
    QDialog dialog(this);
    dialog.setWindowTitle(tr("发布到博客"));
    dialog.setMinimumWidth(420);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QFormLayout *formLayout = new QFormLayout();

    QComboBox *platformCombo = new QComboBox(&dialog);
    platformCombo->addItem("CSDN", static_cast<int>(BlogPublisher::Platform::CSDN));
    platformCombo->addItem(tr("知乎"), static_cast<int>(BlogPublisher::Platform::Zhihu));
    platformCombo->addItem(tr("牛客"), static_cast<int>(BlogPublisher::Platform::Nowcoder));

    int idx = platformCombo->findData(static_cast<int>(blogPublisher->config().platform));
    if (idx >= 0) platformCombo->setCurrentIndex(idx);
    formLayout->addRow(tr("平台:"), platformCombo);

    // 说明
    QLabel *hintLabel = new QLabel(&dialog);
    hintLabel->setWordWrap(true);
    hintLabel->setStyleSheet("color: #666; font-size: 12px;");

    auto updateHint = [&]() {
        BlogPublisher::Platform platform = static_cast<BlogPublisher::Platform>(
            platformCombo->currentData().toInt());
        hintLabel->setText(tr("点击「发布」后，文章 Markdown 内容将复制到剪贴板，"
                              "同时打开 %1 编辑器页面。\n\n"
                              "在浏览器中粘贴内容（Cmd+V），填写标题等信息后发布即可。")
            .arg(BlogPublisher::platformName(platform)));
    };

    connect(platformCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            &dialog, updateHint);
    updateHint();

    formLayout->addRow(QString(), hintLabel);
    layout->addLayout(formLayout);

    // 按钮
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("发布"));
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    // 更新平台配置
    BlogPublisher::BlogConfig config = blogPublisher->config();
    config.platform = static_cast<BlogPublisher::Platform>(platformCombo->currentData().toInt());
    blogPublisher->setConfig(config);

    // 执行发布
    statusBar()->showMessage(tr("正在发布到 %1...")
        .arg(BlogPublisher::platformName(config.platform)));
    blogPublisher->publishPost(ui->plainTextEdit->toPlainText());
}

// 函数说明：处理发布流程，将当前文档输出到电子书、博客或静态站点。
void MainWindow::publishShowBlogSettings()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("博客设置"));
    dialog.setMinimumWidth(450);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    // 默认平台选择
    QFormLayout *formLayout = new QFormLayout();

    QComboBox *platformCombo = new QComboBox(&dialog);
    platformCombo->addItem("CSDN", static_cast<int>(BlogPublisher::Platform::CSDN));
    platformCombo->addItem(tr("知乎"), static_cast<int>(BlogPublisher::Platform::Zhihu));
    platformCombo->addItem(tr("牛客"), static_cast<int>(BlogPublisher::Platform::Nowcoder));

    int idx = platformCombo->findData(static_cast<int>(blogPublisher->config().platform));
    if (idx >= 0) platformCombo->setCurrentIndex(idx);
    formLayout->addRow(tr("默认平台:"), platformCombo);

    layout->addLayout(formLayout);

    // 平台编辑器链接预览
    QLabel *urlLabel = new QLabel(&dialog);
    urlLabel->setWordWrap(true);
    urlLabel->setStyleSheet("color: #666; font-size: 12px;");
    urlLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto updateUrlPreview = [&]() {
        BlogPublisher::Platform platform = static_cast<BlogPublisher::Platform>(
            platformCombo->currentData().toInt());
        urlLabel->setText(tr("发布时将打开: %1").arg(BlogPublisher::editorUrl(platform)));
    };
    connect(platformCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            &dialog, updateUrlPreview);
    updateUrlPreview();
    layout->addWidget(urlLabel);

    // 按钮
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    BlogPublisher::BlogConfig config;
    config.platform = static_cast<BlogPublisher::Platform>(platformCombo->currentData().toInt());
    config.name = BlogPublisher::platformName(config.platform);
    blogPublisher->setConfig(config);

    QList<BlogPublisher::BlogConfig> configs;
    configs.append(config);
    if (saveBlogPublisherConfigs(configs)) {
        statusBar()->showMessage(tr("博客设置已保存"), 3000);
    } else {
        statusBar()->showMessage(tr("博客设置保存失败"), 5000);
    }
}

// 函数说明：处理发布流程，将当前文档输出到电子书、博客或静态站点。
void MainWindow::publishStartSlideshow()
{
    qDebug() << "[Slideshow] publishStartSlideshow called";

    if (ui->plainTextEdit->document()->isEmpty()) {
        QMessageBox::warning(this, tr("幻灯片演示"),
            tr("请先编写一些内容再开始演示。"));
        return;
    }

    // 加载 Markdown 为幻灯片
    QString markdown = ui->plainTextEdit->toPlainText();
    qDebug() << "[Slideshow] markdown length:" << markdown.length();

    if (!slideshowPresenter->loadFromMarkdown(markdown)) {
        QMessageBox::warning(this, tr("幻灯片演示"),
            tr("无法解析 Markdown 为幻灯片。\n"
               "提示: 使用 \"---\" 分隔幻灯片，或使用一级/二级标题。"));
        return;
    }

    qDebug() << "[Slideshow] loaded slides:" << slideshowPresenter->slideCount();

    // 设置演示选项
    SlideshowPresenter::PresentationOptions options;
    options.showProgress = true;
    options.showSlideNumber = true;
    options.showTimer = true;
    options.theme = "dark";
    options.transitionType = "fade";
    options.transitionDuration = 300;
    slideshowPresenter->setOptions(options);

    // 开始演示
    qDebug() << "[Slideshow] starting presentation...";
    slideshowPresenter->startPresentation();
    qDebug() << "[Slideshow] presentation started, isPresenting:" << slideshowPresenter->isPresenting();
}

// 函数说明：处理发布流程，将当前文档输出到电子书、博客或静态站点。
void MainWindow::publishToStaticSite()
{
    if (ui->plainTextEdit->document()->isEmpty()) {
        QMessageBox::warning(this, tr("发布到静态网站"),
            tr("请先编写一些内容再发布。"));
        return;
    }

    // 检查是否已打开站点
    if (!staticSiteGenerator->isValidSite()) {
        QMessageBox::information(this, tr("发布到静态网站"),
            tr("请先在\"静态网站管理\"中打开一个站点。"));
        publishShowStaticSiteManager();
        return;
    }

    // 显示发布对话框
    QDialog dialog(this);
    dialog.setWindowTitle(tr("发布到静态网站"));
    dialog.setMinimumWidth(400);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QFormLayout *formLayout = new QFormLayout();

    QLineEdit *titleEdit = new QLineEdit(&dialog);
    titleEdit->setText(QFileInfo(fileName).baseName());
    formLayout->addRow(tr("标题:"), titleEdit);

    QLineEdit *tagsEdit = new QLineEdit(&dialog);
    tagsEdit->setPlaceholderText(tr("用逗号分隔"));
    formLayout->addRow(tr("标签:"), tagsEdit);

    QLineEdit *categoriesEdit = new QLineEdit(&dialog);
    categoriesEdit->setPlaceholderText(tr("用逗号分隔"));
    formLayout->addRow(tr("分类:"), categoriesEdit);

    QCheckBox *draftCheck = new QCheckBox(tr("保存为草稿"), &dialog);
    formLayout->addRow(draftCheck);

    layout->addLayout(formLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    // 设置 Front Matter
    StaticSiteGenerator::FrontMatter fm;
    fm.title = titleEdit->text();
    fm.tags = tagsEdit->text().split(',', Qt::SkipEmptyParts);
    fm.categories = categoriesEdit->text().split(',', Qt::SkipEmptyParts);
    fm.draft = draftCheck->isChecked();
    fm.date = QDateTime::currentDateTime().toString(Qt::ISODate);

    // 导出到站点
    QString markdown = ui->plainTextEdit->toPlainText();
    if (staticSiteGenerator->exportToSite(markdown, fm)) {
        QMessageBox::information(this, tr("发布成功"),
            tr("文章已成功发布到静态网站。"));
    } else {
        QMessageBox::critical(this, tr("发布失败"),
            tr("无法发布到静态网站: %1").arg(staticSiteGenerator->lastError()));
    }
}

// 函数说明：处理发布流程，将当前文档输出到电子书、博客或静态站点。
void MainWindow::publishShowStaticSiteManager()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("静态网站管理"));
    dialog.setMinimumSize(700, 500);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    // 站点信息
    QGroupBox *siteGroup = new QGroupBox(tr("当前站点"), &dialog);
    QFormLayout *siteLayout = new QFormLayout(siteGroup);

    QLabel *sitePathLabel = new QLabel(
        staticSiteGenerator->isValidSite() ?
            staticSiteGenerator->siteConfig().sitePath : tr("(未打开站点)"),
        siteGroup);
    siteLayout->addRow(tr("路径:"), sitePathLabel);

    QLabel *generatorLabel = new QLabel(siteGroup);
    if (staticSiteGenerator->isValidSite()) {
        switch (staticSiteGenerator->siteConfig().type) {
            case StaticSiteGenerator::GeneratorType::Hugo:
                generatorLabel->setText("Hugo");
                break;
            case StaticSiteGenerator::GeneratorType::Jekyll:
                generatorLabel->setText("Jekyll");
                break;
            case StaticSiteGenerator::GeneratorType::Hexo:
                generatorLabel->setText("Hexo");
                break;
            default:
                generatorLabel->setText(tr("未知"));
        }
    } else {
        generatorLabel->setText("-");
    }
    siteLayout->addRow(tr("生成器:"), generatorLabel);

    layout->addWidget(siteGroup);

    // 操作按钮
    QGroupBox *actionsGroup = new QGroupBox(tr("操作"), &dialog);
    QVBoxLayout *actionsLayout = new QVBoxLayout(actionsGroup);

    QPushButton *openSiteButton = new QPushButton(tr("打开现有站点..."), actionsGroup);
    connect(openSiteButton, &QPushButton::clicked, [&]() {
        QString path = QFileDialog::getExistingDirectory(
            &dialog, tr("选择站点目录"));
        if (!path.isEmpty()) {
            if (staticSiteGenerator->openSite(path)) {
                sitePathLabel->setText(path);
                QMessageBox::information(&dialog, tr("成功"),
                    tr("站点已成功打开。"));
            } else {
                QMessageBox::critical(&dialog, tr("错误"),
                    tr("无法打开站点: %1").arg(staticSiteGenerator->lastError()));
            }
        }
    });
    actionsLayout->addWidget(openSiteButton);

    QPushButton *createSiteButton = new QPushButton(tr("创建新站点..."), actionsGroup);
    connect(createSiteButton, &QPushButton::clicked, [&]() {
        // 选择生成器
        QStringList generators;
        generators << "Hugo" << "Jekyll" << "Hexo";
        bool ok;
        QString generator = QInputDialog::getItem(&dialog, tr("创建新站点"),
            tr("选择生成器:"), generators, 0, false, &ok);
        if (!ok) return;

        // 选择目录
        QString path = QFileDialog::getExistingDirectory(
            &dialog, tr("选择站点父目录"));
        if (path.isEmpty()) return;

        // 输入站点名称
        QString name = QInputDialog::getText(&dialog, tr("创建新站点"),
            tr("站点名称:"), QLineEdit::Normal, "my-blog", &ok);
        if (!ok || name.isEmpty()) return;

        // 设置生成器类型
        StaticSiteGenerator::SiteConfig config;
        if (generator == "Hugo") {
            config.type = StaticSiteGenerator::GeneratorType::Hugo;
        } else if (generator == "Jekyll") {
            config.type = StaticSiteGenerator::GeneratorType::Jekyll;
        } else {
            config.type = StaticSiteGenerator::GeneratorType::Hexo;
        }
        staticSiteGenerator->setSiteConfig(config);

        // 创建站点
        if (staticSiteGenerator->createNewSite(path, name)) {
            sitePathLabel->setText(path + "/" + name);
            QMessageBox::information(&dialog, tr("成功"),
                tr("站点已成功创建。"));
        } else {
            QMessageBox::critical(&dialog, tr("错误"),
                tr("无法创建站点: %1").arg(staticSiteGenerator->lastError()));
        }
    });
    actionsLayout->addWidget(createSiteButton);

    actionsLayout->addSpacing(10);

    QPushButton *startServerButton = new QPushButton(tr("启动本地预览"), actionsGroup);
    startServerButton->setEnabled(staticSiteGenerator->isValidSite());
    connect(startServerButton, &QPushButton::clicked, [&]() {
        if (staticSiteGenerator->startServer()) {
            QMessageBox::information(&dialog, tr("服务器已启动"),
                tr("本地预览服务器已启动:\n%1").arg(staticSiteGenerator->serverUrl()));
            QDesktopServices::openUrl(QUrl(staticSiteGenerator->serverUrl()));
        } else {
            QMessageBox::critical(&dialog, tr("错误"),
                tr("无法启动服务器: %1").arg(staticSiteGenerator->lastError()));
        }
    });
    actionsLayout->addWidget(startServerButton);

    QPushButton *buildButton = new QPushButton(tr("构建站点"), actionsGroup);
    buildButton->setEnabled(staticSiteGenerator->isValidSite());
    connect(buildButton, &QPushButton::clicked, [&]() {
        if (staticSiteGenerator->build()) {
            statusBar()->showMessage(tr("正在构建..."));
        }
    });
    actionsLayout->addWidget(buildButton);

    layout->addWidget(actionsGroup);

    // 检测已安装的生成器
    QGroupBox *detectGroup = new QGroupBox(tr("已安装的生成器"), &dialog);
    QFormLayout *detectLayout = new QFormLayout(detectGroup);

    auto installed = StaticSiteGenerator::detectInstalledGenerators();
    for (auto it = installed.begin(); it != installed.end(); ++it) {
        QString name;
        switch (it.key()) {
            case StaticSiteGenerator::GeneratorType::Hugo: name = "Hugo"; break;
            case StaticSiteGenerator::GeneratorType::Jekyll: name = "Jekyll"; break;
            case StaticSiteGenerator::GeneratorType::Hexo: name = "Hexo"; break;
            case StaticSiteGenerator::GeneratorType::MkDocs: name = "MkDocs"; break;
            default: name = "Unknown"; break;
        }
        detectLayout->addRow(name + ":", new QLabel(it.value(), detectGroup));
    }

    if (installed.isEmpty()) {
        detectLayout->addRow(new QLabel(tr("未检测到任何生成器"), detectGroup));
    }

    layout->addWidget(detectGroup);

    // 关闭按钮
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Close, &dialog);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    dialog.exec();
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onBlogPublished(const BlogPublisher::PublishResult &result)
{
    Q_UNUSED(result);
    BlogPublisher::BlogConfig config = blogPublisher->config();
    statusBar()->showMessage(tr("已复制到剪贴板"), 3000);
    QMessageBox::information(this, tr("发布到博客"),
        tr("文章内容已复制到剪贴板，%1 编辑器已在浏览器中打开。\n\n"
           "请在编辑器中粘贴（Cmd+V）内容，填写标题后发布即可。")
        .arg(BlogPublisher::platformName(config.platform)));
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onSlideshowEnded()
{
    // 演示结束后激活主窗口
    activateWindow();
    raise();
}

// ==================== 工具功能 ====================

void MainWindow::setupToolsFeatures()
{
    // 初始化工具组件
    citationManager = new CitationManager(this);
    loadDefaultCitationEntries();
    voiceInput = new VoiceInput(this);
    translationManager = new TranslationManager(this);

    // 连接信号
    connect(citationManager, &CitationManager::entryAdded,
            this, [this](const QString &key) {
                statusBar()->showMessage(tr("已添加引用: %1").arg(key), 3000);
            });

    connect(voiceInput, &VoiceInput::finalResult,
            this, [this](const VoiceInput::RecognitionResult &result) {
                onVoiceResult(result.text);
            });

    connect(voiceInput, &VoiceInput::commandRecognized,
            this, [this](VoiceInput::VoiceCommand cmd) {
                QString markdown = VoiceInput::commandToMarkdown(cmd);
                if (!markdown.isEmpty()) {
                    ui->plainTextEdit->textCursor().insertText(markdown);
                }
                if (cmd == VoiceInput::VoiceCommand::Stop) {
                    toolsStopVoiceInput();
                }
            });

    connect(voiceInput, &VoiceInput::engineFallbackApplied,
            this, [this](VoiceInput::Engine requested,
                         VoiceInput::Engine effective,
                         const QString &reason) {
                Q_UNUSED(requested)
                Q_UNUSED(effective)
                statusBar()->showMessage(reason, 6000);
            });

    connect(translationManager, &TranslationManager::translationCompleted,
            this, [this](const TranslationManager::TranslationResult &result) {
                onTranslationCompleted(result.translatedText);
            });

    connect(translationManager, &TranslationManager::errorOccurred,
            this, [this](const QString &error) {
                statusBar()->showMessage(tr("翻译错误: %1").arg(error), 5000);
            });

    // 创建工具菜单
    toolsMenu = new QMenu(tr("工具(&T)"), this);
    menuBar()->insertMenu(ui->menuHelp->menuAction(), toolsMenu);

    // 引用管理
    actionCitationManager = new QAction(tr("引用管理器..."), this);
    actionCitationManager->setIcon(QIcon("fa-book.fontawesome"));
    actionCitationManager->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C));
    connect(actionCitationManager, &QAction::triggered, this, &MainWindow::toolsShowCitationManager);
    toolsMenu->addAction(actionCitationManager);

    actionInsertCitation = new QAction(tr("插入引用..."), this);
    actionInsertCitation->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_C));
    connect(actionInsertCitation, &QAction::triggered, this, &MainWindow::toolsInsertCitation);
    toolsMenu->addAction(actionInsertCitation);

    QAction *actionBibliography = new QAction(tr("生成参考文献"), this);
    connect(actionBibliography, &QAction::triggered, this, &MainWindow::toolsGenerateBibliography);
    toolsMenu->addAction(actionBibliography);

    toolsMenu->addSeparator();

    // 语音输入
    actionVoiceInput = new QAction(tr("开始语音输入"), this);
    actionVoiceInput->setIcon(QIcon("fa-microphone.fontawesome"));
    actionVoiceInput->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_V));
    actionVoiceInput->setCheckable(true);
    connect(actionVoiceInput, &QAction::triggered, this, [this](bool checked) {
        if (checked) {
            toolsStartVoiceInput();
        } else {
            toolsStopVoiceInput();
        }
    });
    toolsMenu->addAction(actionVoiceInput);

    QAction *actionVoiceInputSettings = new QAction(tr("语音输入设置..."), this);
    actionVoiceInputSettings->setIcon(QIcon("fa-cog.fontawesome"));
    connect(actionVoiceInputSettings, &QAction::triggered,
            this, &MainWindow::toolsShowVoiceInputSettings);
    toolsMenu->addAction(actionVoiceInputSettings);

    toolsMenu->addSeparator();

    // 翻译
    actionTranslate = new QAction(tr("翻译选中文本..."), this);
    actionTranslate->setIcon(QIcon("fa-language.fontawesome"));
    actionTranslate->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T));
    connect(actionTranslate, &QAction::triggered, this, &MainWindow::toolsTranslateSelection);
    toolsMenu->addAction(actionTranslate);

    QAction *actionTranslateDoc = new QAction(tr("翻译整个文档..."), this);
    connect(actionTranslateDoc, &QAction::triggered, this, &MainWindow::toolsTranslateDocument);
    toolsMenu->addAction(actionTranslateDoc);

    QAction *actionTranslateSettings = new QAction(tr("翻译设置..."), this);
    actionTranslateSettings->setIcon(QIcon("fa-cog.fontawesome"));
    connect(actionTranslateSettings, &QAction::triggered, this, &MainWindow::toolsShowTranslationSettings);
    toolsMenu->addAction(actionTranslateSettings);
}

// 函数说明：加载 MainWindow 需要的数据、配置或外部资源。
void MainWindow::loadDefaultCitationEntries()
{
    // 预置默认示例引用条目，涵盖多种类型
    using E = CitationManager::BibEntry;
    using T = CitationManager::EntryType;

    // ---- 期刊文章 ----
    {
        E e;
        e.key = "knuth1984";
        e.type = T::Article;
        e.title = "Literate Programming";
        e.authors = {"Knuth, Donald E."};
        e.year = "1984";
        e.journal = "The Computer Journal";
        e.volume = "27";
        e.number = "2";
        e.pages = "97-111";
        e.doi = "10.1093/comjnl/27.2.97";
        e.keywords = "literate programming, documentation";
        citationManager->addEntry(e);
    }
    {
        E e;
        e.key = "turing1950";
        e.type = T::Article;
        e.title = "Computing Machinery and Intelligence";
        e.authors = {"Turing, Alan M."};
        e.year = "1950";
        e.journal = "Mind";
        e.volume = "59";
        e.number = "236";
        e.pages = "433-460";
        e.doi = "10.1093/mind/LIX.236.433";
        e.keywords = "artificial intelligence, Turing test";
        citationManager->addEntry(e);
    }
    {
        E e;
        e.key = "shannon1948";
        e.type = T::Article;
        e.title = "A Mathematical Theory of Communication";
        e.authors = {"Shannon, Claude E."};
        e.year = "1948";
        e.journal = "The Bell System Technical Journal";
        e.volume = "27";
        e.number = "3";
        e.pages = "379-423";
        e.doi = "10.1002/j.1538-7305.1948.tb01338.x";
        e.keywords = "information theory, entropy, communication";
        citationManager->addEntry(e);
    }
    {
        E e;
        e.key = "vaswani2017";
        e.type = T::Article;
        e.title = "Attention Is All You Need";
        e.authors = {"Vaswani, Ashish", "Shazeer, Noam", "Parmar, Niki",
                     "Uszkoreit, Jakob", "Jones, Llion", "Gomez, Aidan N.",
                     "Kaiser, Lukasz", "Polosukhin, Illia"};
        e.year = "2017";
        e.journal = "Advances in Neural Information Processing Systems";
        e.volume = "30";
        e.pages = "5998-6008";
        e.keywords = "transformer, attention mechanism, deep learning";
        citationManager->addEntry(e);
    }

    // ---- 书籍 ----
    {
        E e;
        e.key = "cormen2009";
        e.type = T::Book;
        e.title = "Introduction to Algorithms";
        e.authors = {"Cormen, Thomas H.", "Leiserson, Charles E.",
                     "Rivest, Ronald L.", "Stein, Clifford"};
        e.year = "2009";
        e.edition = "3";
        e.publisher = "MIT Press";
        e.address = "Cambridge, MA";
        e.isbn = "978-0-262-03384-8";
        e.keywords = "algorithms, data structures";
        citationManager->addEntry(e);
    }
    {
        E e;
        e.key = "gamma1994";
        e.type = T::Book;
        e.title = "Design Patterns: Elements of Reusable Object-Oriented Software";
        e.authors = {"Gamma, Erich", "Helm, Richard", "Johnson, Ralph", "Vlissides, John"};
        e.year = "1994";
        e.publisher = "Addison-Wesley";
        e.address = "Reading, MA";
        e.isbn = "978-0-201-63361-0";
        e.keywords = "design patterns, object-oriented, software engineering";
        citationManager->addEntry(e);
    }
    {
        E e;
        e.key = "tanenbaum2014";
        e.type = T::Book;
        e.title = "Modern Operating Systems";
        e.authors = {"Tanenbaum, Andrew S.", "Bos, Herbert"};
        e.year = "2014";
        e.edition = "4";
        e.publisher = "Pearson";
        e.isbn = "978-0-13-359162-0";
        e.keywords = "operating systems, computer science";
        citationManager->addEntry(e);
    }

    // ---- 会议论文 ----
    {
        E e;
        e.key = "dijkstra1968";
        e.type = T::InProceedings;
        e.title = "Go To Statement Considered Harmful";
        e.authors = {"Dijkstra, Edsger W."};
        e.year = "1968";
        e.journal = "Communications of the ACM";
        e.volume = "11";
        e.number = "3";
        e.pages = "147-148";
        e.doi = "10.1145/362929.362947";
        e.keywords = "structured programming, goto";
        citationManager->addEntry(e);
    }
    {
        E e;
        e.key = "lecun2015";
        e.type = T::Article;
        e.title = "Deep Learning";
        e.authors = {"LeCun, Yann", "Bengio, Yoshua", "Hinton, Geoffrey"};
        e.year = "2015";
        e.journal = "Nature";
        e.volume = "521";
        e.number = "7553";
        e.pages = "436-444";
        e.doi = "10.1038/nature14539";
        e.keywords = "deep learning, neural networks, machine learning";
        citationManager->addEntry(e);
    }

    // ---- 技术报告 ----
    {
        E e;
        e.key = "rfc2616";
        e.type = T::TechReport;
        e.title = "Hypertext Transfer Protocol -- HTTP/1.1";
        e.authors = {"Fielding, Roy T.", "Gettys, Jim", "Mogul, Jeffrey C.",
                     "Frystyk, Henrik", "Masinter, Larry", "Leach, Paul",
                     "Berners-Lee, Tim"};
        e.year = "1999";
        e.institution = "Internet Engineering Task Force (IETF)";
        e.number = "RFC 2616";
        e.keywords = "HTTP, protocol, web";
        citationManager->addEntry(e);
    }

    // ---- 硕士/博士论文 ----
    {
        E e;
        e.key = "torvalds1997";
        e.type = T::MastersThesis;
        e.title = "Linux: A Portable Operating System";
        e.authors = {"Torvalds, Linus"};
        e.year = "1997";
        e.school = "University of Helsinki";
        e.address = "Helsinki, Finland";
        e.keywords = "Linux, operating system, kernel";
        citationManager->addEntry(e);
    }

    // ---- 在线资源 ----
    {
        E e;
        e.key = "qt2024";
        e.type = T::Online;
        e.title = "Qt 6 Documentation";
        e.authors = {"The Qt Company"};
        e.year = "2024";
        e.url = "https://doc.qt.io/qt-6/";
        e.urldate = "2024-01-15";
        e.keywords = "Qt, framework, C++, GUI";
        citationManager->addEntry(e);
    }

    // ---- 中文文献 (GB/T 7714) ----
    {
        E e;
        e.key = "wangyuan2020";
        e.type = T::Article;
        e.title = QString::fromUtf8("基于深度学习的自然语言处理研究综述");
        e.authors = {QString::fromUtf8("王元"), QString::fromUtf8("李明"), QString::fromUtf8("张华")};
        e.year = "2020";
        e.journal = QString::fromUtf8("计算机学报");
        e.volume = "43";
        e.number = "6";
        e.pages = "1072-1096";
        e.language = "zh-CN";
        e.keywords = QString::fromUtf8("深度学习, 自然语言处理, 神经网络");
        citationManager->addEntry(e);
    }
    {
        E e;
        e.key = "liuwei2019";
        e.type = T::Book;
        e.title = QString::fromUtf8("数据结构与算法分析");
        e.authors = {QString::fromUtf8("刘伟"), QString::fromUtf8("陈建")};
        e.year = "2019";
        e.publisher = QString::fromUtf8("清华大学出版社");
        e.address = QString::fromUtf8("北京");
        e.isbn = "978-7-302-52000-0";
        e.language = "zh-CN";
        e.keywords = QString::fromUtf8("数据结构, 算法, 计算机科学");
        citationManager->addEntry(e);
    }
    {
        E e;
        e.key = "zhangsan2021";
        e.type = T::PhdThesis;
        e.title = QString::fromUtf8("面向边缘计算的资源调度优化研究");
        e.authors = {QString::fromUtf8("张三")};
        e.year = "2021";
        e.school = QString::fromUtf8("中国科学技术大学");
        e.address = QString::fromUtf8("合肥");
        e.language = "zh-CN";
        e.keywords = QString::fromUtf8("边缘计算, 资源调度, 优化");
        citationManager->addEntry(e);
    }
}

// 函数说明：响应工具菜单动作，调用引用、语音、翻译等扩展能力。
void MainWindow::toolsShowCitationManager()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("引用管理器"));
    dialog.setMinimumSize(700, 500);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    // 工具栏
    QHBoxLayout *toolbarLayout = new QHBoxLayout();

    QPushButton *loadLibButton = new QPushButton(tr("打开库..."), &dialog);
    connect(loadLibButton, &QPushButton::clicked, [this, &dialog]() {
        QString path = QFileDialog::getOpenFileName(&dialog,
            tr("打开 BibTeX 文件"),
            QString(),
            tr("BibTeX 文件 (*.bib);;所有文件 (*)"));
        if (!path.isEmpty()) {
            if (citationManager->loadLibrary(path)) {
                statusBar()->showMessage(tr("已加载引用库"), 3000);
            }
        }
    });
    toolbarLayout->addWidget(loadLibButton);

    QPushButton *newEntryButton = new QPushButton(tr("添加条目..."), &dialog);
    toolbarLayout->addWidget(newEntryButton);

    QPushButton *importDOIButton = new QPushButton(tr("从 DOI 导入..."), &dialog);
    connect(importDOIButton, &QPushButton::clicked, [this, &dialog]() {
        bool ok;
        QString doi = QInputDialog::getText(&dialog, tr("从 DOI 导入"),
            tr("输入 DOI:"), QLineEdit::Normal, "", &ok);
        if (ok && !doi.isEmpty()) {
            citationManager->importFromDOI(doi);
            statusBar()->showMessage(tr("正在导入..."), 3000);
        }
    });
    toolbarLayout->addWidget(importDOIButton);

    toolbarLayout->addStretch();

    // 搜索
    QLineEdit *searchEdit = new QLineEdit(&dialog);
    searchEdit->setPlaceholderText(tr("搜索..."));
    toolbarLayout->addWidget(searchEdit);

    layout->addLayout(toolbarLayout);

    // 引用列表
    QTableWidget *table = new QTableWidget(&dialog);
    table->setColumnCount(5);
    table->setHorizontalHeaderLabels({tr("键"), tr("类型"), tr("标题"), tr("作者"), tr("年份")});
    table->horizontalHeader()->setStretchLastSection(true);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // 加载条目
    auto loadEntries = [this, table]() {
        table->setRowCount(0);
        QVector<CitationManager::BibEntry> entries = citationManager->allEntries();
        for (const auto &entry : entries) {
            int row = table->rowCount();
            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(entry.key));
            table->setItem(row, 1, new QTableWidgetItem(
                entry.type == CitationManager::EntryType::Article ? "Article" :
                entry.type == CitationManager::EntryType::Book ? "Book" : "Other"));
            table->setItem(row, 2, new QTableWidgetItem(entry.title));
            table->setItem(row, 3, new QTableWidgetItem(entry.authors.join("; ")));
            table->setItem(row, 4, new QTableWidgetItem(entry.year));
        }
    };

    loadEntries();

    connect(searchEdit, &QLineEdit::textChanged, [this, table](const QString &text) {
        QVector<CitationManager::BibEntry> entries;
        if (text.isEmpty()) {
            entries = citationManager->allEntries();
        } else {
            entries = citationManager->searchEntries(text);
        }
        table->setRowCount(0);
        for (const auto &entry : entries) {
            int row = table->rowCount();
            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(entry.key));
            table->setItem(row, 1, new QTableWidgetItem(""));
            table->setItem(row, 2, new QTableWidgetItem(entry.title));
            table->setItem(row, 3, new QTableWidgetItem(entry.authors.join("; ")));
            table->setItem(row, 4, new QTableWidgetItem(entry.year));
        }
    });

    layout->addWidget(table);

    // 按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *insertButton = new QPushButton(tr("插入引用"), &dialog);
    connect(insertButton, &QPushButton::clicked, [this, table, &dialog]() {
        QList<QTableWidgetItem*> selected = table->selectedItems();
        if (!selected.isEmpty()) {
            int row = selected.first()->row();
            QString key = table->item(row, 0)->text();
            QString citation = citationManager->insertCitation(key);
            ui->plainTextEdit->textCursor().insertText(citation);
            dialog.accept();
        }
    });
    buttonLayout->addWidget(insertButton);

    buttonLayout->addStretch();

    QPushButton *closeButton = new QPushButton(tr("关闭"), &dialog);
    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    buttonLayout->addWidget(closeButton);

    layout->addLayout(buttonLayout);

    dialog.exec();
}

// 函数说明：响应工具菜单动作，调用引用、语音、翻译等扩展能力。
void MainWindow::toolsInsertCitation()
{
    if (!citationManager->isLibraryLoaded()) {
        QMessageBox::information(this, tr("插入引用"),
            tr("请先在引用管理器中打开一个 BibTeX 文件。"));
        toolsShowCitationManager();
        return;
    }

    QVector<CitationManager::BibEntry> entries = citationManager->allEntries();
    if (entries.isEmpty()) {
        QMessageBox::information(this, tr("插入引用"),
            tr("引用库为空。"));
        return;
    }

    QStringList items;
    for (const auto &entry : entries) {
        items << QString("%1 - %2 (%3)").arg(entry.key, entry.title.left(50), entry.year);
    }

    bool ok;
    QString item = QInputDialog::getItem(this, tr("插入引用"),
        tr("选择引用:"), items, 0, false, &ok);

    if (ok && !item.isEmpty()) {
        QString key = item.split(" - ").first();
        QString citation = citationManager->insertCitation(key);
        ui->plainTextEdit->textCursor().insertText(citation);
    }
}

// 函数说明：响应工具菜单动作，调用引用、语音、翻译等扩展能力。
void MainWindow::toolsGenerateBibliography()
{
    if (!citationManager->isLibraryLoaded()) {
        QMessageBox::information(this, tr("生成参考文献"),
            tr("请先在引用管理器中打开一个 BibTeX 文件。"));
        return;
    }

    // 获取文档中的所有引用
    QString text = ui->plainTextEdit->toPlainText();
    QRegularExpression citationRegex("\\[@([^\\]]+)\\]");
    QRegularExpressionMatchIterator it = citationRegex.globalMatch(text);

    QStringList keys;
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString citationKeys = match.captured(1);
        for (const QString &key : citationKeys.split(";")) {
            QString cleanKey = key.trimmed();
            if (cleanKey.startsWith("@")) {
                cleanKey = cleanKey.mid(1);
            }
            if (!keys.contains(cleanKey)) {
                keys << cleanKey;
            }
        }
    }

    if (keys.isEmpty()) {
        QMessageBox::information(this, tr("生成参考文献"),
            tr("文档中没有找到引用。\n使用 [@key] 格式插入引用。"));
        return;
    }

    QString bibliography = citationManager->generateMarkdownBibliography(keys);

    // 插入到文档末尾
    QTextCursor cursor = ui->plainTextEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText("\n\n" + bibliography);
}

// 函数说明：响应工具菜单动作，调用引用、语音、翻译等扩展能力。
void MainWindow::toolsStartVoiceInput()
{
    if (!VoiceInput::isAvailable()) {
        QMessageBox::warning(this, tr("语音输入"),
            tr("没有可用的音频输入设备。"));
        actionVoiceInput->setChecked(false);
        return;
    }

    if (voiceInput->startRecording()) {
        statusBar()->showMessage(tr("语音输入已开始..."));
        actionVoiceInput->setText(tr("停止语音输入"));
    } else {
        QMessageBox::warning(this, tr("语音输入"),
            tr("无法启动语音输入: %1").arg(voiceInput->lastError()));
        actionVoiceInput->setChecked(false);
    }
}

// 函数说明：响应工具菜单动作，调用引用、语音、翻译等扩展能力。
void MainWindow::toolsStopVoiceInput()
{
    voiceInput->stopRecording();
    statusBar()->showMessage(tr("语音输入已停止"), 3000);
    actionVoiceInput->setText(tr("开始语音输入"));
    actionVoiceInput->setChecked(false);
}

// 函数说明：响应工具菜单动作，调用引用、语音、翻译等扩展能力。
void MainWindow::toolsShowVoiceInputSettings()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("语音输入设置"));
    dialog.setMinimumWidth(560);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    VoiceInput::Config cfg = voiceInput->config();

    // 引擎配置
    QGroupBox *engineGroup = new QGroupBox(tr("识别引擎"), &dialog);
    QFormLayout *engineLayout = new QFormLayout(engineGroup);

    QComboBox *engineCombo = new QComboBox(engineGroup);
    engineCombo->addItem(tr("系统引擎"), static_cast<int>(VoiceInput::Engine::System));
    engineCombo->addItem(tr("云端 API"), static_cast<int>(VoiceInput::Engine::CloudAPI));
    engineCombo->addItem(tr("离线模型"), static_cast<int>(VoiceInput::Engine::Offline));
    engineCombo->setCurrentIndex(engineCombo->findData(static_cast<int>(cfg.engine)));
    engineLayout->addRow(tr("优先引擎:"), engineCombo);

    QComboBox *languageCombo = new QComboBox(engineGroup);
    const QStringList languages = VoiceInput::availableLanguages();
    for (const QString &lang : languages) {
        languageCombo->addItem(lang, lang);
    }
    int langIndex = languageCombo->findData(cfg.language);
    if (langIndex < 0) {
        languageCombo->addItem(cfg.language, cfg.language);
        langIndex = languageCombo->count() - 1;
    }
    languageCombo->setCurrentIndex(langIndex);
    engineLayout->addRow(tr("语言:"), languageCombo);

    QLabel *systemInfoLabel = new QLabel(engineGroup);
    if (VoiceInput::isSystemEngineAvailable()) {
        systemInfoLabel->setText(tr("系统引擎状态：可用"));
    } else {
        systemInfoLabel->setText(tr("系统引擎状态：当前平台不可用（将自动回退）"));
    }
    systemInfoLabel->setWordWrap(true);
    systemInfoLabel->setStyleSheet(QStringLiteral("color: #666;"));
    engineLayout->addRow(QString(), systemInfoLabel);

    layout->addWidget(engineGroup);

    // 录音与静音检测
    QGroupBox *audioGroup = new QGroupBox(tr("录音与静音检测"), &dialog);
    QFormLayout *audioLayout = new QFormLayout(audioGroup);

    QSpinBox *sampleRateSpin = new QSpinBox(audioGroup);
    sampleRateSpin->setRange(8000, 48000);
    sampleRateSpin->setSingleStep(1000);
    sampleRateSpin->setValue(cfg.sampleRate);
    audioLayout->addRow(tr("采样率:"), sampleRateSpin);

    QSpinBox *silenceTimeoutSpin = new QSpinBox(audioGroup);
    silenceTimeoutSpin->setRange(0, 60000);
    silenceTimeoutSpin->setSuffix(tr(" ms"));
    silenceTimeoutSpin->setValue(cfg.silenceTimeout);
    audioLayout->addRow(tr("静音超时:"), silenceTimeoutSpin);

    QDoubleSpinBox *silenceThresholdSpin = new QDoubleSpinBox(audioGroup);
    silenceThresholdSpin->setRange(0.001, 0.5);
    silenceThresholdSpin->setSingleStep(0.005);
    silenceThresholdSpin->setDecimals(3);
    silenceThresholdSpin->setValue(cfg.silenceThreshold);
    audioLayout->addRow(tr("固定静音阈值:"), silenceThresholdSpin);

    QCheckBox *adaptiveThresholdCheck = new QCheckBox(tr("启用自适应静音阈值"), audioGroup);
    adaptiveThresholdCheck->setChecked(cfg.enableAdaptiveSilenceThreshold);
    audioLayout->addRow(adaptiveThresholdCheck);

    QDoubleSpinBox *adaptiveSensitivitySpin = new QDoubleSpinBox(audioGroup);
    adaptiveSensitivitySpin->setRange(1.0, 6.0);
    adaptiveSensitivitySpin->setSingleStep(0.1);
    adaptiveSensitivitySpin->setDecimals(2);
    adaptiveSensitivitySpin->setValue(cfg.adaptiveSensitivity);
    audioLayout->addRow(tr("自适应灵敏度:"), adaptiveSensitivitySpin);

    QDoubleSpinBox *minThresholdSpin = new QDoubleSpinBox(audioGroup);
    minThresholdSpin->setRange(0.001, 0.3);
    minThresholdSpin->setSingleStep(0.001);
    minThresholdSpin->setDecimals(3);
    minThresholdSpin->setValue(cfg.minSilenceThreshold);
    audioLayout->addRow(tr("阈值下限:"), minThresholdSpin);

    QDoubleSpinBox *maxThresholdSpin = new QDoubleSpinBox(audioGroup);
    maxThresholdSpin->setRange(0.005, 0.5);
    maxThresholdSpin->setSingleStep(0.005);
    maxThresholdSpin->setDecimals(3);
    maxThresholdSpin->setValue(cfg.maxSilenceThreshold);
    audioLayout->addRow(tr("阈值上限:"), maxThresholdSpin);

    QCheckBox *continuousCheck = new QCheckBox(tr("连续识别"), audioGroup);
    continuousCheck->setChecked(cfg.enableContinuous);
    audioLayout->addRow(continuousCheck);

    QCheckBox *commandCheck = new QCheckBox(tr("启用语音命令"), audioGroup);
    commandCheck->setChecked(cfg.enableCommands);
    audioLayout->addRow(commandCheck);

    QCheckBox *punctuationCheck = new QCheckBox(tr("自动标点"), audioGroup);
    punctuationCheck->setChecked(cfg.enablePunctuation);
    audioLayout->addRow(punctuationCheck);

    auto updateThresholdWidgets = [=]() {
        const bool adaptive = adaptiveThresholdCheck->isChecked();
        silenceThresholdSpin->setEnabled(!adaptive);
        adaptiveSensitivitySpin->setEnabled(adaptive);
        minThresholdSpin->setEnabled(adaptive);
        maxThresholdSpin->setEnabled(adaptive);
    };
    connect(adaptiveThresholdCheck, &QCheckBox::toggled, &dialog, updateThresholdWidgets);
    updateThresholdWidgets();

    layout->addWidget(audioGroup);

    // 云端 API
    QGroupBox *cloudGroup = new QGroupBox(tr("云端 API"), &dialog);
    QFormLayout *cloudLayout = new QFormLayout(cloudGroup);

    QLineEdit *apiUrlEdit = new QLineEdit(cloudGroup);
    apiUrlEdit->setPlaceholderText(tr("例如：https://your-speech-api/recognize"));
    apiUrlEdit->setText(cfg.apiUrl);
    cloudLayout->addRow(tr("API URL:"), apiUrlEdit);

    QLineEdit *apiKeyEdit = new QLineEdit(cloudGroup);
    apiKeyEdit->setEchoMode(QLineEdit::Password);
    apiKeyEdit->setText(cfg.apiKey);
    cloudLayout->addRow(tr("API Key:"), apiKeyEdit);

    layout->addWidget(cloudGroup);

    // 离线模型
    QGroupBox *offlineGroup = new QGroupBox(tr("离线模型"), &dialog);
    QFormLayout *offlineLayout = new QFormLayout(offlineGroup);

    QComboBox *offlineTypeCombo = new QComboBox(offlineGroup);
    for (const QString &type : VoiceInput::supportedOfflineModelTypes()) {
        offlineTypeCombo->addItem(type, type);
    }
    int typeIndex = offlineTypeCombo->findData(cfg.offlineModelType);
    offlineTypeCombo->setCurrentIndex(typeIndex >= 0 ? typeIndex : 0);
    offlineLayout->addRow(tr("模型类型:"), offlineTypeCombo);

    QHBoxLayout *modelPathLayout = new QHBoxLayout();
    QLineEdit *offlineModelPathEdit = new QLineEdit(offlineGroup);
    offlineModelPathEdit->setText(cfg.offlineModelPath);
    QPushButton *browseModelBtn = new QPushButton(tr("选择..."), offlineGroup);
    modelPathLayout->addWidget(offlineModelPathEdit, 1);
    modelPathLayout->addWidget(browseModelBtn);
    offlineLayout->addRow(tr("模型路径:"), modelPathLayout);

    connect(browseModelBtn, &QPushButton::clicked, &dialog, [this, offlineTypeCombo, offlineModelPathEdit]() {
        const QString type = offlineTypeCombo->currentData().toString();
        QString selected;
        if (type == QStringLiteral("vosk")) {
            selected = QFileDialog::getExistingDirectory(this, tr("选择 Vosk 模型目录"));
        } else {
            selected = QFileDialog::getOpenFileName(this, tr("选择 Whisper 模型文件"));
        }
        if (!selected.isEmpty()) {
            offlineModelPathEdit->setText(selected);
        }
    });

    QHBoxLayout *decoderPathLayout = new QHBoxLayout();
    QLineEdit *offlineDecoderEdit = new QLineEdit(offlineGroup);
    offlineDecoderEdit->setText(cfg.offlineDecoderExecutable);
    QPushButton *browseDecoderBtn = new QPushButton(tr("选择..."), offlineGroup);
    decoderPathLayout->addWidget(offlineDecoderEdit, 1);
    decoderPathLayout->addWidget(browseDecoderBtn);
    offlineLayout->addRow(tr("解码器程序:"), decoderPathLayout);

    connect(browseDecoderBtn, &QPushButton::clicked, &dialog, [this, offlineDecoderEdit]() {
        const QString path = QFileDialog::getOpenFileName(this, tr("选择离线解码器程序"));
        if (!path.isEmpty()) {
            offlineDecoderEdit->setText(path);
        }
    });

    QLineEdit *offlineArgsEdit = new QLineEdit(offlineGroup);
    offlineArgsEdit->setText(cfg.offlineDecoderArguments.join(QLatin1Char(' ')));
    offlineArgsEdit->setPlaceholderText(tr("示例: -m {model} -f {audio} -l {lang}"));
    offlineLayout->addRow(tr("解码器参数:"), offlineArgsEdit);

    QSpinBox *offlineTimeoutSpin = new QSpinBox(offlineGroup);
    offlineTimeoutSpin->setRange(1000, 120000);
    offlineTimeoutSpin->setSuffix(tr(" ms"));
    offlineTimeoutSpin->setValue(cfg.offlineDecodeTimeoutMs);
    offlineLayout->addRow(tr("离线超时:"), offlineTimeoutSpin);

    QLabel *offlineSpecLabel = new QLabel(VoiceInput::offlineModelFormatSpecification(), offlineGroup);
    offlineSpecLabel->setWordWrap(true);
    offlineSpecLabel->setStyleSheet(QStringLiteral("color:#666;"));
    offlineLayout->addRow(QString(), offlineSpecLabel);

    layout->addWidget(offlineGroup);

    // 按钮
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    cfg.engine = static_cast<VoiceInput::Engine>(engineCombo->currentData().toInt());
    cfg.language = languageCombo->currentData().toString();
    cfg.sampleRate = sampleRateSpin->value();
    cfg.silenceTimeout = silenceTimeoutSpin->value();
    cfg.silenceThreshold = static_cast<float>(silenceThresholdSpin->value());
    cfg.enableAdaptiveSilenceThreshold = adaptiveThresholdCheck->isChecked();
    cfg.adaptiveSensitivity = static_cast<float>(adaptiveSensitivitySpin->value());
    cfg.minSilenceThreshold = static_cast<float>(minThresholdSpin->value());
    cfg.maxSilenceThreshold = static_cast<float>(maxThresholdSpin->value());
    cfg.enableContinuous = continuousCheck->isChecked();
    cfg.enableCommands = commandCheck->isChecked();
    cfg.enablePunctuation = punctuationCheck->isChecked();
    cfg.apiUrl = apiUrlEdit->text().trimmed();
    cfg.apiKey = apiKeyEdit->text().trimmed();
    cfg.offlineModelType = offlineTypeCombo->currentData().toString();
    cfg.offlineModelPath = offlineModelPathEdit->text().trimmed();
    cfg.offlineDecoderExecutable = offlineDecoderEdit->text().trimmed();
    cfg.offlineDecoderArguments = QProcess::splitCommand(offlineArgsEdit->text().trimmed());
    cfg.offlineDecodeTimeoutMs = offlineTimeoutSpin->value();

    voiceInput->setConfig(cfg);

    QString offlineReason;
    if ((cfg.engine == VoiceInput::Engine::Offline || !cfg.offlineModelPath.isEmpty()) &&
        !voiceInput->validateOfflineModel(&offlineReason)) {
        QMessageBox::warning(this, tr("语音输入设置"),
                             tr("离线模型配置暂不可用：\n%1\n\n"
                                "你仍可在录音时自动回退到其他可用引擎。")
                                 .arg(offlineReason));
    }

    statusBar()->showMessage(tr("语音输入设置已保存"), 3000);
}

// 函数说明：响应工具菜单动作，调用引用、语音、翻译等扩展能力。
void MainWindow::toolsTranslateSelection()
{
    QString selectedText = ui->plainTextEdit->textCursor().selectedText();
    if (selectedText.isEmpty()) {
        QMessageBox::information(this, tr("翻译"),
            tr("请先选择要翻译的文本。"));
        return;
    }

    // 显示翻译对话框
    QDialog dialog(this);
    dialog.setWindowTitle(tr("翻译"));
    dialog.setMinimumWidth(500);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    // 源语言和目标语言
    QHBoxLayout *langLayout = new QHBoxLayout();
    QComboBox *sourceLangCombo = new QComboBox(&dialog);
    QComboBox *targetLangCombo = new QComboBox(&dialog);

    sourceLangCombo->addItem(tr("自动检测"), "auto");
    targetLangCombo->addItem(tr("中文"), "zh");
    targetLangCombo->addItem(tr("英文"), "en");
    targetLangCombo->addItem(tr("日文"), "ja");
    targetLangCombo->addItem(tr("韩文"), "ko");
    targetLangCombo->addItem(tr("法文"), "fr");
    targetLangCombo->addItem(tr("德文"), "de");

    for (int i = 1; i < targetLangCombo->count(); ++i) {
        sourceLangCombo->addItem(targetLangCombo->itemText(i), targetLangCombo->itemData(i));
    }

    langLayout->addWidget(new QLabel(tr("源语言:")));
    langLayout->addWidget(sourceLangCombo);
    langLayout->addWidget(new QLabel(tr("目标语言:")));
    langLayout->addWidget(targetLangCombo);
    layout->addLayout(langLayout);

    // 原文
    QTextEdit *sourceEdit = new QTextEdit(&dialog);
    sourceEdit->setPlainText(selectedText);
    sourceEdit->setMaximumHeight(100);
    layout->addWidget(new QLabel(tr("原文:")));
    layout->addWidget(sourceEdit);

    // 译文
    QTextEdit *targetEdit = new QTextEdit(&dialog);
    targetEdit->setReadOnly(true);
    targetEdit->setMaximumHeight(100);
    layout->addWidget(new QLabel(tr("译文:")));
    layout->addWidget(targetEdit);

    // 翻译按钮
    QPushButton *translateButton = new QPushButton(tr("翻译"), &dialog);
    connect(translateButton, &QPushButton::clicked, [this, sourceEdit, targetEdit, sourceLangCombo, targetLangCombo, translateButton]() {
        QString text = sourceEdit->toPlainText();
        if (text.isEmpty()) {
            targetEdit->setPlainText(tr("请输入要翻译的文本。"));
            return;
        }
        QString source = sourceLangCombo->currentData().toString();
        QString target = targetLangCombo->currentData().toString();

        // 显示翻译中状态
        translateButton->setEnabled(false);
        translateButton->setText(tr("翻译中..."));
        targetEdit->setPlainText(tr("正在翻译，请稍候..."));

        // 使用异步翻译 + 临时连接，避免嵌套 QEventLoop 问题
        QMetaObject::Connection *connOk = new QMetaObject::Connection();
        QMetaObject::Connection *connErr = new QMetaObject::Connection();
        QTimer *timeoutTimer = new QTimer(translateButton);
        timeoutTimer->setSingleShot(true);

        // 清理回调（翻译完成/失败/超时后断开连接）
        auto cleanup = [connOk, connErr, timeoutTimer, translateButton]() {
            QObject::disconnect(*connOk);
            QObject::disconnect(*connErr);
            delete connOk;
            delete connErr;
            timeoutTimer->stop();
            timeoutTimer->deleteLater();
            translateButton->setEnabled(true);
            translateButton->setText(QObject::tr("翻译"));
        };

        *connOk = connect(translationManager, &TranslationManager::translationCompleted,
            targetEdit, [targetEdit, cleanup](const TranslationManager::TranslationResult &r) {
                cleanup();
                if (!r.translatedText.isEmpty()) {
                    targetEdit->setPlainText(r.translatedText);
                } else {
                    targetEdit->setPlainText(QObject::tr("翻译结果为空。"));
                }
            });

        *connErr = connect(translationManager, &TranslationManager::errorOccurred,
            targetEdit, [targetEdit, cleanup](const QString &error) {
                cleanup();
                targetEdit->setPlainText(QObject::tr("翻译失败: %1").arg(error));
            });

        // 超时处理（15秒）
        connect(timeoutTimer, &QTimer::timeout, targetEdit, [targetEdit, cleanup]() {
            cleanup();
            targetEdit->setPlainText(QObject::tr("翻译超时，请检查网络连接和 API 配置。"));
        });
        timeoutTimer->start(15000);

        translationManager->translate(text, target, source);
    });
    layout->addWidget(translateButton);

    // 按钮
    QDialogButtonBox *buttonBox = new QDialogButtonBox(&dialog);
    QPushButton *replaceButton = buttonBox->addButton(tr("替换原文"), QDialogButtonBox::AcceptRole);
    QPushButton *insertButton = buttonBox->addButton(tr("插入译文"), QDialogButtonBox::ActionRole);
    buttonBox->addButton(QDialogButtonBox::Cancel);

    connect(replaceButton, &QPushButton::clicked, [this, targetEdit, &dialog]() {
        QString translation = targetEdit->toPlainText();
        if (!translation.isEmpty()) {
            ui->plainTextEdit->textCursor().insertText(translation);
        }
        dialog.accept();
    });

    connect(insertButton, &QPushButton::clicked, [this, targetEdit, &dialog]() {
        QString translation = targetEdit->toPlainText();
        if (!translation.isEmpty()) {
            QTextCursor cursor = ui->plainTextEdit->textCursor();
            cursor.movePosition(QTextCursor::EndOfBlock);
            cursor.insertText("\n\n" + translation);
        }
        dialog.accept();
    });

    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    layout->addWidget(buttonBox);

    dialog.exec();
}

// 函数说明：响应工具菜单动作，调用引用、语音、翻译等扩展能力。
void MainWindow::toolsTranslateDocument()
{
    QString text = ui->plainTextEdit->toPlainText();
    if (text.isEmpty()) {
        QMessageBox::information(this, tr("翻译文档"),
            tr("文档为空。"));
        return;
    }

    // 选择目标语言
    QStringList languages;
    languages << tr("中文") << tr("英文") << tr("日文") << tr("韩文");
    QStringList codes;
    codes << "zh" << "en" << "ja" << "ko";

    bool ok;
    QString lang = QInputDialog::getItem(this, tr("翻译文档"),
        tr("选择目标语言:"), languages, 0, false, &ok);

    if (!ok) return;

    int index = languages.indexOf(lang);
    QString targetLang = codes[index];

    statusBar()->showMessage(tr("正在翻译..."));

    // 分段翻译
    QStringList paragraphs = text.split("\n\n");
    translationManager->translateBatch(paragraphs, targetLang);

    connect(translationManager, &TranslationManager::batchTranslationCompleted,
            this, [this](const QVector<TranslationManager::TranslationResult> &results) {
                QStringList translated;
                for (const auto &result : results) {
                    translated << result.translatedText;
                }
                QString fullTranslation = translated.join("\n\n");

                // 询问是否替换
                QMessageBox::StandardButton reply = QMessageBox::question(this,
                    tr("翻译完成"),
                    tr("是否用译文替换原文？"),
                    QMessageBox::Yes | QMessageBox::No);

                if (reply == QMessageBox::Yes) {
                    ui->plainTextEdit->setPlainText(fullTranslation);
                } else {
                    // 追加到末尾
                    QTextCursor cursor = ui->plainTextEdit->textCursor();
                    cursor.movePosition(QTextCursor::End);
                    cursor.insertText("\n\n---\n\n## 译文\n\n" + fullTranslation);
                }

                statusBar()->showMessage(tr("翻译完成"), 3000);
            }, Qt::SingleShotConnection);
}

// 函数说明：响应工具菜单动作，调用引用、语音、翻译等扩展能力。
void MainWindow::toolsShowTranslationSettings()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("翻译设置"));
    dialog.setMinimumWidth(400);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    // 翻译引擎
    QGroupBox *engineGroup = new QGroupBox(tr("翻译引擎"), &dialog);
    QFormLayout *engineLayout = new QFormLayout(engineGroup);

    QComboBox *engineCombo = new QComboBox(engineGroup);
    engineCombo->addItem(tr("百度翻译"), static_cast<int>(TranslationManager::Engine::Baidu));
    engineCombo->addItem(tr("有道翻译"), static_cast<int>(TranslationManager::Engine::Youdao));
    engineCombo->addItem(tr("DeepL"), static_cast<int>(TranslationManager::Engine::DeepL));
    engineCombo->addItem(tr("腾讯翻译"), static_cast<int>(TranslationManager::Engine::Tencent));
    engineCombo->addItem(tr("离线词典"), static_cast<int>(TranslationManager::Engine::Offline));
    engineLayout->addRow(tr("引擎:"), engineCombo);

    layout->addWidget(engineGroup);

    // API 配置
    QGroupBox *apiGroup = new QGroupBox(tr("API 配置"), &dialog);
    QFormLayout *apiLayout = new QFormLayout(apiGroup);

    QLabel *appIdLabel = new QLabel(tr("App ID:"), apiGroup);
    QLineEdit *appIdEdit = new QLineEdit(apiGroup);
    apiLayout->addRow(appIdLabel, appIdEdit);

    QLabel *apiKeyLabel = new QLabel(tr("API Key:"), apiGroup);
    QLineEdit *apiKeyEdit = new QLineEdit(apiGroup);
    apiKeyEdit->setEchoMode(QLineEdit::Password);
    apiLayout->addRow(apiKeyLabel, apiKeyEdit);

    QLabel *secretLabel = new QLabel(tr("密钥:"), apiGroup);
    QLineEdit *secretEdit = new QLineEdit(apiGroup);
    secretEdit->setEchoMode(QLineEdit::Password);
    apiLayout->addRow(secretLabel, secretEdit);

    // 提示标签 — 根据引擎显示不同的填写指引
    QLabel *hintLabel = new QLabel(apiGroup);
    hintLabel->setWordWrap(true);
    hintLabel->setStyleSheet("color: #666; font-size: 12px; padding: 4px 0;");
    apiLayout->addRow(hintLabel);

    // 根据引擎切换字段标签和提示
    auto updateApiFieldHints = [=](int engineInt) {
        auto engine = static_cast<TranslationManager::Engine>(engineInt);
        switch (engine) {
        case TranslationManager::Engine::Baidu:
            appIdLabel->setText(tr("App ID:"));
            appIdEdit->setPlaceholderText(tr("百度翻译 APP ID"));
            appIdEdit->setVisible(true); appIdLabel->setVisible(true);
            apiKeyLabel->setVisible(false); apiKeyEdit->setVisible(false);
            secretLabel->setText(tr("密钥:"));
            secretEdit->setPlaceholderText(tr("百度翻译密钥"));
            secretEdit->setVisible(true); secretLabel->setVisible(true);
            hintLabel->setText(tr("百度翻译: 填写「APP ID」和「密钥」\n"
                                  "从百度翻译开放平台获取: https://fanyi-api.baidu.com"));
            break;
        case TranslationManager::Engine::Youdao:
            appIdLabel->setText(tr("应用ID:"));
            appIdEdit->setPlaceholderText(tr("有道控制台「应用ID」"));
            appIdEdit->setVisible(true); appIdLabel->setVisible(true);
            apiKeyLabel->setText(tr("APIkey:"));
            apiKeyLabel->setVisible(true); apiKeyEdit->setVisible(true);
            apiKeyEdit->setPlaceholderText(tr("有道控制台「APIkey」"));
            secretLabel->setText(tr("应用秘钥:"));
            secretEdit->setPlaceholderText(tr("有道控制台「应用秘钥」"));
            secretEdit->setVisible(true); secretLabel->setVisible(true);
            hintLabel->setText(tr("有道翻译: 三个字段都需要填写！\n"
                                  "从有道智云控制台获取: https://ai.youdao.com"));
            break;
        case TranslationManager::Engine::DeepL:
            appIdLabel->setVisible(false); appIdEdit->setVisible(false);
            apiKeyLabel->setText(tr("API Key:"));
            apiKeyLabel->setVisible(true); apiKeyEdit->setVisible(true);
            apiKeyEdit->setPlaceholderText(tr("DeepL Authentication Key"));
            secretLabel->setVisible(false); secretEdit->setVisible(false);
            hintLabel->setText(tr("DeepL: 只需填写「API Key」\n"
                                  "从 DeepL 获取: https://www.deepl.com/pro-api"));
            break;
        case TranslationManager::Engine::Google:
            appIdLabel->setVisible(false); appIdEdit->setVisible(false);
            apiKeyLabel->setText(tr("API Key:"));
            apiKeyLabel->setVisible(true); apiKeyEdit->setVisible(true);
            apiKeyEdit->setPlaceholderText(tr("Google Cloud API Key"));
            secretLabel->setVisible(false); secretEdit->setVisible(false);
            hintLabel->setText(tr("Google 翻译: 只需填写「API Key」\n"
                                  "从 Google Cloud Console 获取"));
            break;
        case TranslationManager::Engine::Tencent:
            appIdLabel->setText(tr("SecretId:"));
            appIdEdit->setPlaceholderText(tr("腾讯云 SecretId"));
            appIdEdit->setVisible(true); appIdLabel->setVisible(true);
            apiKeyLabel->setVisible(false); apiKeyEdit->setVisible(false);
            secretLabel->setText(tr("SecretKey:"));
            secretEdit->setPlaceholderText(tr("腾讯云 SecretKey"));
            secretEdit->setVisible(true); secretLabel->setVisible(true);
            hintLabel->setText(tr("腾讯翻译: 填写「SecretId」和「SecretKey」\n"
                                  "从腾讯云控制台获取"));
            break;
        case TranslationManager::Engine::Offline:
            appIdLabel->setVisible(false); appIdEdit->setVisible(false);
            apiKeyLabel->setVisible(false); apiKeyEdit->setVisible(false);
            secretLabel->setVisible(false); secretEdit->setVisible(false);
            hintLabel->setText(tr("离线模式: 无需 API 凭据，使用本地词典文件"));
            break;
        }
    };

    connect(engineCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            &dialog, [=](int index) {
        updateApiFieldHints(engineCombo->itemData(index).toInt());
    });

    QHBoxLayout *offlineDictLayout = new QHBoxLayout();
    QLineEdit *offlineDictPathEdit = new QLineEdit(apiGroup);
    offlineDictPathEdit->setPlaceholderText(tr("离线词典文件路径（JSON v1 / TSV）"));
    QPushButton *loadSampleOfflineDictBtn = new QPushButton(tr("加载示例"), apiGroup);
    QPushButton *browseOfflineDictBtn = new QPushButton(tr("选择..."), apiGroup);
    offlineDictLayout->addWidget(offlineDictPathEdit, 1);
    offlineDictLayout->addWidget(loadSampleOfflineDictBtn);
    offlineDictLayout->addWidget(browseOfflineDictBtn);
    apiLayout->addRow(tr("离线词典:"), offlineDictLayout);

    connect(loadSampleOfflineDictBtn, &QPushButton::clicked, &dialog, [this, offlineDictPathEdit]() {
        const QString samplePath = translationSampleDictionaryPath();
        if (samplePath.isEmpty()) {
            QMessageBox::warning(this, tr("离线词典"),
                                 tr("未找到示例词典文件。\n"
                                    "请确认仓库中存在：examples/translation/offline-dictionary-v1.json"));
            return;
        }
        offlineDictPathEdit->setText(samplePath);
    });

    connect(browseOfflineDictBtn, &QPushButton::clicked, &dialog, [this, offlineDictPathEdit]() {
        const QString path = QFileDialog::getOpenFileName(
            this,
            tr("选择离线词典"),
            QString(),
            tr("词典文件 (*.json *.tsv *.txt);;所有文件 (*)"));
        if (!path.isEmpty()) {
            offlineDictPathEdit->setText(path);
        }
    });

    QLabel *offlineFormatHint = new QLabel(
        tr("格式说明：%1").arg(TranslationManager::offlineDictionaryFormatSpecification()),
        apiGroup);
    offlineFormatHint->setWordWrap(true);
    offlineFormatHint->setStyleSheet(QStringLiteral("color: #666;"));
    apiLayout->addRow(QString(), offlineFormatHint);

    layout->addWidget(apiGroup);

    // 默认语言
    QGroupBox *langGroup = new QGroupBox(tr("默认语言"), &dialog);
    QFormLayout *langLayout = new QFormLayout(langGroup);

    QComboBox *defaultTargetCombo = new QComboBox(langGroup);
    defaultTargetCombo->addItem(tr("中文"), "zh");
    defaultTargetCombo->addItem(tr("英文"), "en");
    langLayout->addRow(tr("目标语言:"), defaultTargetCombo);

    layout->addWidget(langGroup);

    // 速率限制
    QGroupBox *rateLimitGroup = new QGroupBox(tr("速率限制"), &dialog);
    QFormLayout *rateLimitLayout = new QFormLayout(rateLimitGroup);

    QCheckBox *enableRateLimitCheck = new QCheckBox(tr("启用 API 速率限制"), rateLimitGroup);
    QSpinBox *maxRequestsSpin = new QSpinBox(rateLimitGroup);
    maxRequestsSpin->setRange(1, 10000);
    maxRequestsSpin->setSuffix(tr(" 次/分钟"));

    QSpinBox *minIntervalSpin = new QSpinBox(rateLimitGroup);
    minIntervalSpin->setRange(0, 60000);
    minIntervalSpin->setSuffix(tr(" ms"));

    rateLimitLayout->addRow(enableRateLimitCheck);
    rateLimitLayout->addRow(tr("每分钟上限:"), maxRequestsSpin);
    rateLimitLayout->addRow(tr("最小间隔:"), minIntervalSpin);
    layout->addWidget(rateLimitGroup);

    // 加载当前配置
    TranslationManager::Config config = translationManager->config();
    engineCombo->setCurrentIndex(engineCombo->findData(static_cast<int>(config.engine)));
    appIdEdit->setText(config.appId);
    apiKeyEdit->setText(config.apiKey);
    secretEdit->setText(config.apiSecret);
    offlineDictPathEdit->setText(config.offlineDictionaryPath);
    defaultTargetCombo->setCurrentIndex(defaultTargetCombo->findData(config.defaultTargetLang));
    enableRateLimitCheck->setChecked(config.enableRateLimit);
    maxRequestsSpin->setValue(config.maxRequestsPerMinute);
    minIntervalSpin->setValue(config.minRequestIntervalMs);

    // 初始化字段提示（根据当前引擎）
    updateApiFieldHints(static_cast<int>(config.engine));

    // 按钮
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    if (dialog.exec() == QDialog::Accepted) {
        config.engine = static_cast<TranslationManager::Engine>(engineCombo->currentData().toInt());
        config.appId = appIdEdit->text().trimmed();
        config.apiKey = apiKeyEdit->text().trimmed();
        config.apiSecret = secretEdit->text().trimmed();
        config.defaultTargetLang = defaultTargetCombo->currentData().toString();
        config.enableRateLimit = enableRateLimitCheck->isChecked();
        config.maxRequestsPerMinute = maxRequestsSpin->value();
        config.minRequestIntervalMs = minIntervalSpin->value();
        config.offlineDictionaryPath = offlineDictPathEdit->text().trimmed();

        translationManager->setConfig(config);

        if (!config.offlineDictionaryPath.isEmpty()) {
            if (!translationManager->loadOfflineDictionary(config.offlineDictionaryPath)) {
                QMessageBox::warning(this, tr("翻译设置"),
                    tr("离线词典加载失败：\n%1").arg(translationManager->lastError()));
            }
        }

        statusBar()->showMessage(tr("翻译设置已保存"), 3000);
    }
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onVoiceResult(const QString &text)
{
    if (text.isEmpty()) return;

    // 插入识别的文本
    ui->plainTextEdit->textCursor().insertText(text);
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onTranslationCompleted(const QString &result)
{
    statusBar()->showMessage(tr("翻译完成"), 3000);
    Q_UNUSED(result)
}

// ==================== 导出扩展功能 ====================

void MainWindow::setupExportExtensions()
{
    // 创建导出器
    wordExporter = new WordExporter(this);
    latexExporter = new LaTeXExporter(this);

    // 创建思维导图视图
    mindMapView = new MindMapView(this);
    mindMapDock = new QDockWidget(tr("思维导图"), this);
    mindMapDock->setWidget(mindMapView);
    mindMapDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    addDockWidget(Qt::RightDockWidgetArea, mindMapDock);
    mindMapDock->hide();

    // 连接思维导图点击信号
    connect(mindMapView, &MindMapView::nodeClicked, this, [this](int lineNumber) {
        if (lineNumber >= 0) {
            QTextCursor cursor = ui->plainTextEdit->textCursor();
            cursor.movePosition(QTextCursor::Start);
            cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, lineNumber);
            ui->plainTextEdit->setTextCursor(cursor);
            ui->plainTextEdit->centerCursor();
        }
    });

    // 双向编辑：从思维导图更新 Markdown
    connect(mindMapView, &MindMapView::markdownChanged, this, [this](const QString &newMarkdown) {
        // 暂时断开文本变化信号以避免循环更新
        disconnect(ui->plainTextEdit, &QPlainTextEdit::textChanged, nullptr, nullptr);

        // 保存光标位置
        int cursorPos = ui->plainTextEdit->textCursor().position();

        // 更新文本
        ui->plainTextEdit->setPlainText(newMarkdown);

        // 恢复光标
        QTextCursor cursor = ui->plainTextEdit->textCursor();
        cursor.setPosition(qMin(cursorPos, ui->plainTextEdit->toPlainText().length()));
        ui->plainTextEdit->setTextCursor(cursor);

        // 重新连接信号
        connect(ui->plainTextEdit, &QPlainTextEdit::textChanged, this, [this]() {
            if (mindMapDock->isVisible()) {
                mindMapView->loadFromMarkdown(ui->plainTextEdit->toPlainText());
            }
        });

        statusBar()->showMessage(tr("思维导图编辑已同步"), 2000);
    });

    // 添加导出菜单项到文件菜单
    QMenu *fileMenu = ui->menuFile;

    // 在现有的导出菜单项后添加
    QAction *exportPdfAction = nullptr;
    for (QAction *action : fileMenu->actions()) {
        if (action->text().contains("PDF", Qt::CaseInsensitive)) {
            exportPdfAction = action;
            break;
        }
    }

    actionExportWord = new QAction(tr("导出为 Word (.docx)..."), this);
    actionExportWord->setIcon(QIcon::fromTheme("document-export"));
    connect(actionExportWord, &QAction::triggered, this, &MainWindow::exportToWord);

    actionExportLaTeX = new QAction(tr("导出为 LaTeX..."), this);
    actionExportLaTeX->setIcon(QIcon::fromTheme("text-x-tex"));
    connect(actionExportLaTeX, &QAction::triggered, this, &MainWindow::exportToLaTeX);

    if (exportPdfAction) {
        fileMenu->insertAction(exportPdfAction->menu() ? exportPdfAction : exportPdfAction, actionExportWord);
        fileMenu->insertAction(actionExportWord, actionExportLaTeX);
    } else {
        fileMenu->addAction(actionExportWord);
        fileMenu->addAction(actionExportLaTeX);
    }

    // 添加思维导图到视图菜单
    QMenu *viewMenu = ui->menuView;
    viewMenu->addSeparator();
    actionShowMindMap = new QAction(tr("思维导图"), this);
    actionShowMindMap->setCheckable(true);
    actionShowMindMap->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_M));
    connect(actionShowMindMap, &QAction::triggered, this, &MainWindow::exportShowMindMap);
    viewMenu->addAction(actionShowMindMap);

    // 连接文档变化更新思维导图
    connect(ui->plainTextEdit, &QPlainTextEdit::textChanged, this, [this]() {
        if (mindMapDock->isVisible()) {
            mindMapView->loadFromMarkdown(ui->plainTextEdit->toPlainText());
        }
    });
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
void MainWindow::exportToWord()
{
    QString markdown = ui->plainTextEdit->toPlainText();
    if (markdown.isEmpty()) {
        QMessageBox::information(this, tr("导出 Word"),
            tr("文档为空，无法导出。"));
        return;
    }

    // 显示导出选项对话框
    QDialog dialog(this);
    dialog.setWindowTitle(tr("导出为 Word 文档"));
    dialog.setMinimumWidth(400);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    // 文档信息
    QGroupBox *infoGroup = new QGroupBox(tr("文档信息"), &dialog);
    QFormLayout *infoLayout = new QFormLayout(infoGroup);

    QLineEdit *titleEdit = new QLineEdit(infoGroup);
    titleEdit->setText(QFileInfo(fileName).baseName());
    infoLayout->addRow(tr("标题:"), titleEdit);

    QLineEdit *authorEdit = new QLineEdit(infoGroup);
    infoLayout->addRow(tr("作者:"), authorEdit);

    layout->addWidget(infoGroup);

    // 样式选项
    QGroupBox *styleGroup = new QGroupBox(tr("样式选项"), &dialog);
    QFormLayout *styleLayout = new QFormLayout(styleGroup);

    QComboBox *fontCombo = new QComboBox(styleGroup);
    fontCombo->addItem("Times New Roman");
    fontCombo->addItem("Arial");
    fontCombo->addItem("Calibri");
    fontCombo->addItem("宋体");
    fontCombo->addItem("微软雅黑");
    styleLayout->addRow(tr("字体:"), fontCombo);

    QSpinBox *fontSizeSpin = new QSpinBox(styleGroup);
    fontSizeSpin->setRange(8, 72);
    fontSizeSpin->setValue(12);
    styleLayout->addRow(tr("字号:"), fontSizeSpin);

    layout->addWidget(styleGroup);

    // 按钮
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    // 选择保存路径
    QString defaultPath = fileName.isEmpty() ?
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/document.docx" :
        QFileInfo(fileName).path() + "/" + QFileInfo(fileName).baseName() + ".docx";

    QString path = QFileDialog::getSaveFileName(this, tr("导出 Word 文档"),
        defaultPath, tr("Word 文档 (*.docx)"));

    if (path.isEmpty()) {
        return;
    }

    // 设置导出选项
    WordExporter::DocumentMetadata metadata;
    metadata.title = titleEdit->text();
    metadata.author = authorEdit->text();
    wordExporter->setMetadata(metadata);

    WordExporter::ExportOptions options;
    options.defaultFontName = fontCombo->currentText();
    options.defaultFontSize = fontSizeSpin->value() * 2;  // 转换为半点
    wordExporter->setOptions(options);

    statusBar()->showMessage(tr("正在导出..."));

    if (wordExporter->exportFromMarkdown(markdown, path)) {
        statusBar()->showMessage(tr("已导出到: %1").arg(path), 5000);

        QMessageBox::StandardButton reply = QMessageBox::question(this,
            tr("导出成功"),
            tr("Word 文档已导出成功。\n是否打开文件？"),
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        }
    } else {
        QMessageBox::warning(this, tr("导出失败"),
            tr("无法导出 Word 文档: %1").arg(wordExporter->lastError()));
    }
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
void MainWindow::exportToLaTeX()
{
    QString markdown = ui->plainTextEdit->toPlainText();
    if (markdown.isEmpty()) {
        QMessageBox::information(this, tr("导出 LaTeX"),
            tr("文档为空，无法导出。"));
        return;
    }

    // 显示导出选项对话框
    QDialog dialog(this);
    dialog.setWindowTitle(tr("导出为 LaTeX 文档"));
    dialog.setMinimumWidth(500);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    // 文档信息
    QGroupBox *infoGroup = new QGroupBox(tr("文档信息"), &dialog);
    QFormLayout *infoLayout = new QFormLayout(infoGroup);

    QLineEdit *titleEdit = new QLineEdit(infoGroup);
    titleEdit->setText(QFileInfo(fileName).baseName());
    infoLayout->addRow(tr("标题:"), titleEdit);

    QLineEdit *authorEdit = new QLineEdit(infoGroup);
    infoLayout->addRow(tr("作者:"), authorEdit);

    QLineEdit *dateEdit = new QLineEdit(infoGroup);
    dateEdit->setText(QDate::currentDate().toString("yyyy-MM-dd"));
    infoLayout->addRow(tr("日期:"), dateEdit);

    layout->addWidget(infoGroup);

    // 文档类型
    QGroupBox *docGroup = new QGroupBox(tr("文档类型"), &dialog);
    QFormLayout *docLayout = new QFormLayout(docGroup);

    QComboBox *classCombo = new QComboBox(docGroup);
    classCombo->addItem(tr("文章 (article)"), static_cast<int>(LaTeXExporter::DocumentClass::Article));
    classCombo->addItem(tr("报告 (report)"), static_cast<int>(LaTeXExporter::DocumentClass::Report));
    classCombo->addItem(tr("书籍 (book)"), static_cast<int>(LaTeXExporter::DocumentClass::Book));
    classCombo->addItem(tr("演示文稿 (beamer)"), static_cast<int>(LaTeXExporter::DocumentClass::Beamer));
    classCombo->addItem(tr("中文文章 (ctexart)"), static_cast<int>(LaTeXExporter::DocumentClass::CTEXArticle));
    docLayout->addRow(tr("文档类:"), classCombo);

    QComboBox *paperCombo = new QComboBox(docGroup);
    paperCombo->addItem("a4paper");
    paperCombo->addItem("letterpaper");
    paperCombo->addItem("a5paper");
    docLayout->addRow(tr("纸张:"), paperCombo);

    QSpinBox *fontSizeSpin = new QSpinBox(docGroup);
    fontSizeSpin->setRange(10, 14);
    fontSizeSpin->setValue(12);
    fontSizeSpin->setSuffix("pt");
    docLayout->addRow(tr("字号:"), fontSizeSpin);

    layout->addWidget(docGroup);

    // 选项
    QGroupBox *optGroup = new QGroupBox(tr("选项"), &dialog);
    QFormLayout *optLayout = new QFormLayout(optGroup);

    QCheckBox *tocCheck = new QCheckBox(tr("生成目录"), optGroup);
    tocCheck->setChecked(true);
    optLayout->addRow(tocCheck);

    QCheckBox *numberCheck = new QCheckBox(tr("章节编号"), optGroup);
    numberCheck->setChecked(true);
    optLayout->addRow(numberCheck);

    QComboBox *codeStyleCombo = new QComboBox(optGroup);
    codeStyleCombo->addItem(tr("自动 (Auto，推荐)"), static_cast<int>(LaTeXExporter::CodeHighlight::Auto));
    codeStyleCombo->addItem(tr("listings 宏包"), static_cast<int>(LaTeXExporter::CodeHighlight::Listings));
    codeStyleCombo->addItem(tr("minted 宏包 (需要 pygments)"), static_cast<int>(LaTeXExporter::CodeHighlight::Minted));
    optLayout->addRow(tr("代码高亮:"), codeStyleCombo);

    layout->addWidget(optGroup);

    // 按钮
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    // 选择保存路径
    QString defaultPath = fileName.isEmpty() ?
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/document.tex" :
        QFileInfo(fileName).path() + "/" + QFileInfo(fileName).baseName() + ".tex";

    QString path = QFileDialog::getSaveFileName(this, tr("导出 LaTeX 文档"),
        defaultPath, tr("LaTeX 文档 (*.tex)"));

    if (path.isEmpty()) {
        return;
    }

    // 设置导出选项
    LaTeXExporter::DocumentMetadata metadata;
    metadata.title = titleEdit->text();
    metadata.author = authorEdit->text();
    metadata.date = dateEdit->text();
    latexExporter->setMetadata(metadata);

    LaTeXExporter::DocumentOptions options;
    options.documentClass = static_cast<LaTeXExporter::DocumentClass>(classCombo->currentData().toInt());
    options.paperSize = paperCombo->currentText();
    options.fontSize = QString("%1pt").arg(fontSizeSpin->value());
    options.tableOfContents = tocCheck->isChecked();
    options.numberSections = numberCheck->isChecked();
    options.codeHighlight = static_cast<LaTeXExporter::CodeHighlight>(codeStyleCombo->currentData().toInt());
    latexExporter->setOptions(options);

    statusBar()->showMessage(tr("正在导出..."));

    if (latexExporter->exportFromMarkdown(markdown, path)) {
        statusBar()->showMessage(tr("已导出到: %1").arg(path), 5000);

        QMessageBox::StandardButton reply = QMessageBox::question(this,
            tr("导出成功"),
            tr("LaTeX 文档已导出成功。\n是否打开文件？"),
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        }
    } else {
        QMessageBox::warning(this, tr("导出失败"),
            tr("无法导出 LaTeX 文档: %1").arg(latexExporter->lastError()));
    }
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
void MainWindow::exportShowMindMap()
{
    if (actionShowMindMap->isChecked()) {
        mindMapDock->show();
        mindMapView->loadFromMarkdown(ui->plainTextEdit->toPlainText());
    } else {
        mindMapDock->hide();
    }
}

// ==================== 内容管理功能 ====================

void MainWindow::setupContentManagement()
{
    // 创建管理器
    notesLibrary = new NotesLibrary(this);
    gitManager = new GitManager(this);
    cloudSync = new CloudSync(this);

    // 连接信号
    connect(gitManager, &GitManager::statusChanged,
            this, &MainWindow::onGitStatusChanged);
    connect(gitManager, &GitManager::repositoryOpened,
            this, [this](const QString &) { onGitStatusChanged(); });
    connect(gitManager, &GitManager::repositoryClosed,
            this, &MainWindow::onGitStatusChanged);
    connect(cloudSync, &CloudSync::syncProgress,
            this, &MainWindow::onSyncProgress);
    connect(cloudSync, &CloudSync::syncCompleted,
            this, [this](const QString &serviceName, int uploaded, int downloaded) {
                Q_UNUSED(serviceName);
                statusBar()->showMessage(tr("同步完成"), 3000);
                // 只有手动同步才弹窗，自动同步不打扰用户
                if (m_manualSync) {
                    m_manualSync = false;
                    QMessageBox::information(this, tr("云同步"),
                        tr("同步完成。\n上传 %1 个文件，下载 %2 个文件。")
                        .arg(uploaded).arg(downloaded));
                }
            });
    connect(cloudSync, &CloudSync::syncFailed,
            this, [this](const QString &serviceName, const QString &error) {
                statusBar()->showMessage(tr("同步失败"), 3000);
                // 只有手动同步才弹窗
                if (m_manualSync) {
                    m_manualSync = false;
                    QMessageBox::warning(this, tr("同步失败"),
                        tr("同步服务 \"%1\" 失败:\n%2").arg(serviceName, error));
                }
            });

    if (!loadCloudSyncConfig(cloudSync)) {
        const QString detail = cloudSync->lastError().isEmpty()
                                   ? tr("无法读取配置文件: %1").arg(cloudSyncConfigPath())
                                   : cloudSync->lastError();
        qWarning() << "Failed to load cloud sync config:" << detail;
    }

    // 创建笔记库菜单
    libraryMenu = new QMenu(tr("笔记库(&L)"), this);
    menuBar()->insertMenu(ui->menuHelp->menuAction(), libraryMenu);

    actionLibraryManager = new QAction(tr("笔记库管理..."), this);
    actionLibraryManager->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_L));
    connect(actionLibraryManager, &QAction::triggered, this, &MainWindow::libraryOpenManager);
    libraryMenu->addAction(actionLibraryManager);

    actionQuickSearch = new QAction(tr("快速搜索..."), this);
    actionQuickSearch->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F));
    connect(actionQuickSearch, &QAction::triggered, this, &MainWindow::libraryQuickSearch);
    libraryMenu->addAction(actionQuickSearch);

    libraryMenu->addSeparator();

    QAction *actionShowTags = new QAction(tr("标签管理..."), this);
    connect(actionShowTags, &QAction::triggered, this, &MainWindow::libraryShowTags);
    libraryMenu->addAction(actionShowTags);

    QAction *actionShowFolders = new QAction(tr("文件夹视图..."), this);
    connect(actionShowFolders, &QAction::triggered, this, &MainWindow::libraryShowFolders);
    libraryMenu->addAction(actionShowFolders);

    // 创建 Git 菜单
    gitMenu = new QMenu(tr("版本控制(&G)"), this);
    menuBar()->insertMenu(ui->menuHelp->menuAction(), gitMenu);

    actionGitHistory = new QAction(tr("版本历史..."), this);
    actionGitHistory->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_H));
    connect(actionGitHistory, &QAction::triggered, this, &MainWindow::gitShowHistory);
    gitMenu->addAction(actionGitHistory);

    actionGitDiff = new QAction(tr("查看变更..."), this);
    actionGitDiff->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_D));
    connect(actionGitDiff, &QAction::triggered, this, &MainWindow::gitShowDiff);
    gitMenu->addAction(actionGitDiff);

    gitMenu->addSeparator();

    actionGitCommit = new QAction(tr("提交更改..."), this);
    actionGitCommit->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C));
    connect(actionGitCommit, &QAction::triggered, this, &MainWindow::gitCommit);
    gitMenu->addAction(actionGitCommit);

    QAction *actionGitPush = new QAction(tr("推送到远程..."), this);
    connect(actionGitPush, &QAction::triggered, this, &MainWindow::gitPush);
    gitMenu->addAction(actionGitPush);

    QAction *actionGitPull = new QAction(tr("拉取更新..."), this);
    connect(actionGitPull, &QAction::triggered, this, &MainWindow::gitPull);
    gitMenu->addAction(actionGitPull);

    gitMenu->addSeparator();

    actionGitEnableLfs = new QAction(tr("启用 Git LFS（当前仓库）"), this);
    connect(actionGitEnableLfs, &QAction::triggered, this, &MainWindow::gitEnableLfs);
    gitMenu->addAction(actionGitEnableLfs);

    actionGitManageLfs = new QAction(tr("管理 LFS 跟踪规则..."), this);
    connect(actionGitManageLfs, &QAction::triggered, this, &MainWindow::gitManageLfsTracking);
    gitMenu->addAction(actionGitManageLfs);

    actionGitQuickTrackLargeFiles = new QAction(tr("跟踪常见大文件类型"), this);
    connect(actionGitQuickTrackLargeFiles, &QAction::triggered,
            this, &MainWindow::gitQuickTrackLargeFiles);
    gitMenu->addAction(actionGitQuickTrackLargeFiles);

    // 创建同步菜单
    syncMenu = new QMenu(tr("云同步(&S)"), this);
    menuBar()->insertMenu(ui->menuHelp->menuAction(), syncMenu);

    actionSyncNow = new QAction(tr("立即同步"), this);
    actionSyncNow->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));
    connect(actionSyncNow, &QAction::triggered, this, &MainWindow::syncNow);
    syncMenu->addAction(actionSyncNow);

    syncMenu->addSeparator();

    actionSyncSettings = new QAction(tr("同步设置..."), this);
    connect(actionSyncSettings, &QAction::triggered, this, &MainWindow::syncShowSettings);
    syncMenu->addAction(actionSyncSettings);

    // 尝试打开当前文件所在目录的 Git 仓库
    if (!fileName.isEmpty()) {
        QString dirPath = QFileInfo(fileName).path();
        if (gitManager->isRepository(dirPath)) {
            gitManager->openRepository(dirPath);
        }
    }

    onGitStatusChanged();
}

// 函数说明：实现 MainWindow::libraryOpenManager 的核心逻辑，供当前模块调用。
void MainWindow::libraryOpenManager()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("笔记库管理"));
    dialog.setMinimumSize(800, 600);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    // 工具栏
    QHBoxLayout *toolbarLayout = new QHBoxLayout();

    QPushButton *openLibraryBtn = new QPushButton(tr("打开笔记库..."), &dialog);
    connect(openLibraryBtn, &QPushButton::clicked, [this]() {
        QString path = QFileDialog::getExistingDirectory(this, tr("选择笔记库目录"));
        if (!path.isEmpty()) {
            notesLibrary->openLibrary(path);
        }
    });
    toolbarLayout->addWidget(openLibraryBtn);

    QLineEdit *searchEdit = new QLineEdit(&dialog);
    searchEdit->setPlaceholderText(tr("搜索笔记..."));
    toolbarLayout->addWidget(searchEdit);

    layout->addLayout(toolbarLayout);

    // 笔记列表
    QTableWidget *noteTable = new QTableWidget(&dialog);
    noteTable->setColumnCount(4);
    noteTable->setHorizontalHeaderLabels({tr("标题"), tr("标签"), tr("修改时间"), tr("字数")});
    noteTable->horizontalHeader()->setStretchLastSection(true);
    noteTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    noteTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // 加载笔记
    if (notesLibrary->isOpen()) {
        QVector<NotesLibrary::NoteInfo> notes = notesLibrary->getAllNotes();
        noteTable->setRowCount(notes.size());

        for (int i = 0; i < notes.size(); ++i) {
            const NotesLibrary::NoteInfo &note = notes[i];
            noteTable->setItem(i, 0, new QTableWidgetItem(note.title));
            noteTable->setItem(i, 1, new QTableWidgetItem(note.tags.join(", ")));
            noteTable->setItem(i, 2, new QTableWidgetItem(note.modifiedTime.toString("yyyy-MM-dd HH:mm")));
            noteTable->setItem(i, 3, new QTableWidgetItem(QString::number(note.wordCount)));
        }
    }

    layout->addWidget(noteTable);

    // 搜索功能
    connect(searchEdit, &QLineEdit::textChanged, [this, noteTable](const QString &text) {
        if (text.isEmpty()) {
            for (int i = 0; i < noteTable->rowCount(); ++i) {
                noteTable->setRowHidden(i, false);
            }
            return;
        }

        QVector<NotesLibrary::NoteInfo> results = notesLibrary->quickSearch(text);
        QSet<QString> matchedTitles;
        for (const NotesLibrary::NoteInfo &note : results) {
            matchedTitles.insert(note.title);
        }

        for (int i = 0; i < noteTable->rowCount(); ++i) {
            QString title = noteTable->item(i, 0)->text();
            noteTable->setRowHidden(i, !matchedTitles.contains(title));
        }
    });

    // 双击打开笔记
    connect(noteTable, &QTableWidget::cellDoubleClicked, [this, &dialog](int row, int) {
        if (notesLibrary->isOpen()) {
            QVector<NotesLibrary::NoteInfo> notes = notesLibrary->getAllNotes();
            if (row < notes.size()) {
                load(notes[row].filePath);
                dialog.accept();
            }
        }
    });

    // 按钮
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    dialog.exec();
}

// 函数说明：实现 MainWindow::libraryQuickSearch 的核心逻辑，供当前模块调用。
void MainWindow::libraryQuickSearch()
{
    bool ok;
    QString query = QInputDialog::getText(this, tr("快速搜索"),
        tr("输入搜索关键词:"), QLineEdit::Normal, QString(), &ok);

    if (!ok || query.isEmpty()) {
        return;
    }

    if (!notesLibrary->isOpen()) {
        // 尝试打开当前目录作为笔记库
        QString dirPath = fileName.isEmpty() ?
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) :
            QFileInfo(fileName).path();
        notesLibrary->openLibrary(dirPath);
    }

    QVector<NotesLibrary::SearchResult> results = notesLibrary->search(query);

    if (results.isEmpty()) {
        QMessageBox::information(this, tr("搜索结果"),
            tr("没有找到匹配的笔记。"));
        return;
    }

    // 显示搜索结果
    QDialog dialog(this);
    dialog.setWindowTitle(tr("搜索结果"));
    dialog.setMinimumSize(750, 550);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QLabel *label = new QLabel(tr("找到 %1 个结果:").arg(results.size()), &dialog);
    layout->addWidget(label);

    QListWidget *listWidget = new QListWidget(&dialog);
    for (const NotesLibrary::SearchResult &result : results) {
        QListWidgetItem *item = new QListWidgetItem(result.note.title);
        item->setData(Qt::UserRole, result.note.filePath);
        item->setToolTip(result.matchedText);
        listWidget->addItem(item);
    }
    layout->addWidget(listWidget);

    connect(listWidget, &QListWidget::itemDoubleClicked, [this, &dialog](QListWidgetItem *item) {
        QString filePath = item->data(Qt::UserRole).toString();
        load(filePath);
        dialog.accept();
    });

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    dialog.exec();
}

// 函数说明：实现 MainWindow::libraryShowTags 的核心逻辑，供当前模块调用。
void MainWindow::libraryShowTags()
{
    if (!notesLibrary->isOpen()) {
        QMessageBox::information(this, tr("标签管理"),
            tr("请先打开笔记库。"));
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(tr("标签管理"));
    dialog.setMinimumSize(650, 500);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QListWidget *tagList = new QListWidget(&dialog);
    QVector<NotesLibrary::TagInfo> tags = notesLibrary->getAllTags();

    for (const NotesLibrary::TagInfo &tag : tags) {
        QListWidgetItem *item = new QListWidgetItem(
            QString("%1 (%2)").arg(tag.name).arg(tag.noteCount));
        if (!tag.color.isEmpty()) {
            item->setBackground(QColor(tag.color));
        }
        tagList->addItem(item);
    }
    layout->addWidget(tagList);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    dialog.exec();
}

// 函数说明：实现 MainWindow::libraryShowFolders 的核心逻辑，供当前模块调用。
void MainWindow::libraryShowFolders()
{
    if (!notesLibrary->isOpen()) {
        QMessageBox::information(this, tr("文件夹视图"),
            tr("请先打开笔记库。"));
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(tr("文件夹视图"));
    dialog.setMinimumSize(650, 500);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QTreeWidget *folderTree = new QTreeWidget(&dialog);
    folderTree->setHeaderLabel(tr("文件夹"));

    QVector<NotesLibrary::FolderInfo> folders = notesLibrary->getFolders();
    QMap<QString, QTreeWidgetItem*> folderItems;

    for (const NotesLibrary::FolderInfo &folder : folders) {
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, QString("%1 (%2)").arg(folder.name).arg(folder.noteCount));
        item->setData(0, Qt::UserRole, folder.path);

        if (folder.path.isEmpty()) {
            folderTree->addTopLevelItem(item);
        } else {
            QString parentPath = QFileInfo(folder.path).path();
            if (folderItems.contains(parentPath)) {
                folderItems[parentPath]->addChild(item);
            } else {
                folderTree->addTopLevelItem(item);
            }
        }
        folderItems[folder.path] = item;
    }

    folderTree->expandAll();
    layout->addWidget(folderTree);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    dialog.exec();
}

// 函数说明：实现 MainWindow::ensureGitRepositoryAvailable 的核心逻辑，供当前模块调用。
bool MainWindow::ensureGitRepositoryAvailable(const QString &operationTitle)
{
    if (!gitManager) {
        return false;
    }

    if (!gitManager->repositoryPath().isEmpty()) {
        return true;
    }

    QString candidatePath;
    if (!fileName.isEmpty()) {
        candidatePath = QFileInfo(fileName).path();
    } else {
        candidatePath = QDir::currentPath();
    }

    if (gitManager->openRepository(candidatePath)) {
        return true;
    }

    QMessageBox::information(this, operationTitle,
        tr("未找到可用的 Git 仓库。\n%1").arg(gitManager->lastError()));
    return false;
}

// 函数说明：处理 Git 相关操作，维护当前文档目录的版本状态。
void MainWindow::gitShowHistory()
{
    if (fileName.isEmpty()) {
        QMessageBox::information(this, tr("版本历史"),
            tr("请先打开文件。"));
        return;
    }

    if (!ensureGitRepositoryAvailable(tr("版本历史"))) {
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(tr("版本历史"));
    dialog.setMinimumSize(800, 500);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QTableWidget *historyTable = new QTableWidget(&dialog);
    historyTable->setColumnCount(4);
    historyTable->setHorizontalHeaderLabels({tr("提交"), tr("作者"), tr("日期"), tr("信息")});
    historyTable->horizontalHeader()->setStretchLastSection(true);
    historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    QString relativePath = QDir(gitManager->rootPath()).relativeFilePath(fileName);
    QVector<GitManager::CommitInfo> history = gitManager->getHistory(50, relativePath);

    historyTable->setRowCount(history.size());
    for (int i = 0; i < history.size(); ++i) {
        const GitManager::CommitInfo &commit = history[i];
        historyTable->setItem(i, 0, new QTableWidgetItem(commit.shortHash));
        historyTable->setItem(i, 1, new QTableWidgetItem(commit.author));
        historyTable->setItem(i, 2, new QTableWidgetItem(commit.date.toString("yyyy-MM-dd HH:mm")));
        historyTable->setItem(i, 3, new QTableWidgetItem(commit.subject));
    }

    layout->addWidget(historyTable);

    // 查看选中版本
    QPushButton *viewBtn = new QPushButton(tr("查看此版本"), &dialog);
    connect(viewBtn, &QPushButton::clicked, [this, historyTable, &history, relativePath]() {
        int row = historyTable->currentRow();
        if (row >= 0 && row < history.size()) {
            QString content = gitManager->getFileContent(relativePath, history[row].hash);
            if (!content.isEmpty()) {
                QDialog contentDialog(this);
                contentDialog.setWindowTitle(tr("版本 %1").arg(history[row].shortHash));
                contentDialog.setMinimumSize(600, 400);

                QVBoxLayout *l = new QVBoxLayout(&contentDialog);
                QPlainTextEdit *editor = new QPlainTextEdit(&contentDialog);
                editor->setPlainText(content);
                editor->setReadOnly(true);
                l->addWidget(editor);

                contentDialog.exec();
            }
        }
    });
    layout->addWidget(viewBtn);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    dialog.exec();
}

// 函数说明：处理 Git 相关操作，维护当前文档目录的版本状态。
void MainWindow::gitShowDiff()
{
    if (fileName.isEmpty()) {
        QMessageBox::information(this, tr("查看变更"),
            tr("请先保存文件。"));
        return;
    }

    if (!ensureGitRepositoryAvailable(tr("查看变更"))) {
        return;
    }
    QString relativePath = QDir(gitManager->rootPath()).relativeFilePath(fileName);
    QString diff = gitManager->getDiff(relativePath);

    if (diff.isEmpty()) {
        QMessageBox::information(this, tr("查看变更"),
            tr("没有未提交的变更。"));
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(tr("变更内容"));
    dialog.setMinimumSize(700, 500);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QPlainTextEdit *diffView = new QPlainTextEdit(&dialog);
    diffView->setPlainText(diff);
    diffView->setReadOnly(true);
    diffView->setFont(QFont("Consolas", 10));
    layout->addWidget(diffView);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    dialog.exec();
}

// 函数说明：处理 Git 相关操作，维护当前文档目录的版本状态。
void MainWindow::gitCommit()
{
    if (fileName.isEmpty()) {
        QMessageBox::information(this, tr("提交更改"),
            tr("请先保存文件。"));
        return;
    }

    if (!ensureGitRepositoryAvailable(tr("提交更改"))) {
        return;
    }
    // 检查是否有变更
    QMap<QString, GitManager::FileStatus> status = gitManager->getStatus();
    if (status.isEmpty()) {
        QMessageBox::information(this, tr("提交更改"),
            tr("没有需要提交的变更。"));
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(tr("提交更改"));
    dialog.setMinimumWidth(500);

        QVBoxLayout *layout = new QVBoxLayout(&dialog);

        // 变更列表
        QGroupBox *changesGroup = new QGroupBox(tr("变更文件"), &dialog);
        QVBoxLayout *changesLayout = new QVBoxLayout(changesGroup);

        QListWidget *changesList = new QListWidget(changesGroup);
        for (auto it = status.begin(); it != status.end(); ++it) {
            QString statusText;
            switch (it.value()) {
                case GitManager::FileStatus::Modified: statusText = "[M]"; break;
                case GitManager::FileStatus::Untracked: statusText = "[?]"; break;
                case GitManager::FileStatus::Staged: statusText = "[A]"; break;
                case GitManager::FileStatus::Deleted: statusText = "[D]"; break;
                default: statusText = "[?]"; break;
            }
            QListWidgetItem *item = new QListWidgetItem(statusText + " " + it.key());
            item->setCheckState(Qt::Checked);
            item->setData(Qt::UserRole, it.key());
            changesList->addItem(item);
        }
        changesLayout->addWidget(changesList);
        layout->addWidget(changesGroup);

        // 提交信息
        QLabel *msgLabel = new QLabel(tr("提交信息:"), &dialog);
        layout->addWidget(msgLabel);

        QTextEdit *msgEdit = new QTextEdit(&dialog);
        msgEdit->setMaximumHeight(100);
        msgEdit->setPlaceholderText(tr("输入提交信息..."));
        layout->addWidget(msgEdit);

        // 按钮
        QDialogButtonBox *buttonBox = new QDialogButtonBox(&dialog);
        QPushButton *commitBtn = buttonBox->addButton(tr("提交"), QDialogButtonBox::AcceptRole);
        buttonBox->addButton(QDialogButtonBox::Cancel);

        connect(buttonBox, &QDialogButtonBox::accepted, [this, changesList, msgEdit, &dialog]() {
            QString message = msgEdit->toPlainText().trimmed();
            if (message.isEmpty()) {
                QMessageBox::warning(this, tr("提交更改"),
                    tr("请输入提交信息。"));
                return;
            }

            // 暂存选中的文件
            QStringList filesToStage;
            for (int i = 0; i < changesList->count(); ++i) {
                QListWidgetItem *item = changesList->item(i);
                if (item->checkState() == Qt::Checked) {
                    filesToStage.append(item->data(Qt::UserRole).toString());
                }
            }

            if (filesToStage.isEmpty()) {
                QMessageBox::warning(this, tr("提交更改"),
                    tr("请选择要提交的文件。"));
                return;
            }

            gitManager->stageFiles(filesToStage);

            if (gitManager->commit(message)) {
                statusBar()->showMessage(tr("提交成功"), 3000);
                dialog.accept();
            } else {
                QMessageBox::warning(this, tr("提交失败"),
                    gitManager->lastError());
            }
        });
        connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        layout->addWidget(buttonBox);

    dialog.exec();
}

// 函数说明：处理 Git 相关操作，维护当前文档目录的版本状态。
void MainWindow::gitPush()
{
    if (!ensureGitRepositoryAvailable(tr("推送到远程"))) {
        return;
    }

    statusBar()->showMessage(tr("正在推送..."));

    if (gitManager->push()) {
        statusBar()->showMessage(tr("推送成功"), 3000);
    } else {
        QMessageBox::warning(this, tr("推送失败"),
            gitManager->lastError());
    }
}

// 函数说明：处理 Git 相关操作，维护当前文档目录的版本状态。
void MainWindow::gitPull()
{
    if (!ensureGitRepositoryAvailable(tr("拉取更新"))) {
        return;
    }

    statusBar()->showMessage(tr("正在拉取..."));

    if (gitManager->pull()) {
        statusBar()->showMessage(tr("拉取成功"), 3000);
    } else {
        QMessageBox::warning(this, tr("拉取失败"),
            gitManager->lastError());
    }
}

// 函数说明：处理 Git 相关操作，维护当前文档目录的版本状态。
void MainWindow::gitEnableLfs()
{
    if (!ensureGitRepositoryAvailable(tr("启用 Git LFS"))) {
        return;
    }

    if (!GitManager::isGitLfsInstalled()) {
        QMessageBox::warning(this, tr("启用 Git LFS"),
            tr("当前环境未安装 Git LFS。\n请先安装 git-lfs 后重试。"));
        return;
    }

    if (gitManager->isLfsEnabled()) {
        QMessageBox::information(this, tr("启用 Git LFS"),
            tr("当前仓库已启用 Git LFS。"));
        onGitStatusChanged();
        return;
    }

    if (gitManager->installLfs(true)) {
        statusBar()->showMessage(tr("Git LFS 已在当前仓库启用"), 4000);
        onGitStatusChanged();
    } else {
        QMessageBox::warning(this, tr("启用 Git LFS 失败"),
            gitManager->lastError());
    }
}

// 函数说明：处理 Git 相关操作，维护当前文档目录的版本状态。
void MainWindow::gitManageLfsTracking()
{
    if (!ensureGitRepositoryAvailable(tr("管理 LFS 跟踪规则"))) {
        return;
    }

    if (!GitManager::isGitLfsInstalled()) {
        QMessageBox::warning(this, tr("管理 LFS 跟踪规则"),
            tr("当前环境未安装 Git LFS。\n请先安装 git-lfs 后重试。"));
        return;
    }

    if (!gitManager->isLfsEnabled()) {
        const QMessageBox::StandardButton reply = QMessageBox::question(
            this, tr("管理 LFS 跟踪规则"),
            tr("当前仓库尚未启用 Git LFS，是否先启用？"),
            QMessageBox::Yes | QMessageBox::No);
        if (reply != QMessageBox::Yes) {
            return;
        }
        if (!gitManager->installLfs(true)) {
            QMessageBox::warning(this, tr("管理 LFS 跟踪规则"),
                tr("启用 Git LFS 失败: %1").arg(gitManager->lastError()));
            return;
        }
    }

    QDialog dialog(this);
    dialog.setWindowTitle(tr("管理 LFS 跟踪规则"));
    dialog.setMinimumSize(700, 500);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QLabel *hintLabel = new QLabel(
        tr("建议使用 glob 模式，例如：`*.zip`、`assets/**`。"), &dialog);
    hintLabel->setWordWrap(true);
    layout->addWidget(hintLabel);

    QListWidget *patternList = new QListWidget(&dialog);
    layout->addWidget(patternList, 1);

    auto refreshPatterns = [this, patternList]() {
        patternList->clear();
        const QStringList patterns = gitManager->getLfsTrackedPatterns();
        for (const QString &pattern : patterns) {
            patternList->addItem(pattern);
        }
    };
    refreshPatterns();

    QHBoxLayout *addLayout = new QHBoxLayout();
    QLineEdit *patternEdit = new QLineEdit(&dialog);
    patternEdit->setPlaceholderText(tr("输入要跟踪的模式，例如 *.psd"));
    QPushButton *addBtn = new QPushButton(tr("添加规则"), &dialog);
    addLayout->addWidget(patternEdit, 1);
    addLayout->addWidget(addBtn);
    layout->addLayout(addLayout);

    QHBoxLayout *opLayout = new QHBoxLayout();
    QPushButton *removeBtn = new QPushButton(tr("移除选中"), &dialog);
    QPushButton *refreshBtn = new QPushButton(tr("刷新"), &dialog);
    opLayout->addWidget(removeBtn);
    opLayout->addWidget(refreshBtn);
    opLayout->addStretch(1);
    layout->addLayout(opLayout);

    connect(addBtn, &QPushButton::clicked, [this, patternEdit, refreshPatterns]() {
        const QString pattern = patternEdit->text().trimmed();
        if (pattern.isEmpty()) {
            QMessageBox::warning(this, tr("添加规则"),
                tr("请输入有效的 LFS 跟踪规则。"));
            return;
        }

        if (gitManager->trackWithLfs(pattern)) {
            patternEdit->clear();
            refreshPatterns();
            statusBar()->showMessage(tr("已添加 LFS 跟踪规则: %1").arg(pattern), 3000);
        } else {
            QMessageBox::warning(this, tr("添加规则失败"),
                gitManager->lastError());
        }
    });

    connect(removeBtn, &QPushButton::clicked, [this, patternList, refreshPatterns]() {
        QListWidgetItem *item = patternList->currentItem();
        if (!item) {
            QMessageBox::information(this, tr("移除规则"),
                tr("请先选择要移除的规则。"));
            return;
        }

        const QString pattern = item->text().trimmed();
        if (pattern.isEmpty()) {
            return;
        }

        if (gitManager->untrackFromLfs(pattern)) {
            refreshPatterns();
            statusBar()->showMessage(tr("已移除 LFS 跟踪规则: %1").arg(pattern), 3000);
        } else {
            QMessageBox::warning(this, tr("移除规则失败"),
                gitManager->lastError());
        }
    });

    connect(refreshBtn, &QPushButton::clicked, &dialog, refreshPatterns);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    dialog.exec();
    onGitStatusChanged();
}

// 函数说明：处理 Git 相关操作，维护当前文档目录的版本状态。
void MainWindow::gitQuickTrackLargeFiles()
{
    if (!ensureGitRepositoryAvailable(tr("跟踪常见大文件类型"))) {
        return;
    }

    if (!GitManager::isGitLfsInstalled()) {
        QMessageBox::warning(this, tr("跟踪常见大文件类型"),
            tr("当前环境未安装 Git LFS。\n请先安装 git-lfs 后重试。"));
        return;
    }

    if (!gitManager->isLfsEnabled() && !gitManager->installLfs(true)) {
        QMessageBox::warning(this, tr("跟踪常见大文件类型"),
            tr("启用 Git LFS 失败: %1").arg(gitManager->lastError()));
        return;
    }

    const QStringList presets = {
        QStringLiteral("*.zip"),
        QStringLiteral("*.7z"),
        QStringLiteral("*.tar"),
        QStringLiteral("*.gz"),
        QStringLiteral("*.rar"),
        QStringLiteral("*.mp4"),
        QStringLiteral("*.mov"),
        QStringLiteral("*.mkv"),
        QStringLiteral("*.psd"),
        QStringLiteral("*.ai"),
        QStringLiteral("*.iso")
    };

    const QStringList existing = gitManager->getLfsTrackedPatterns();
    QStringList added;
    QStringList failed;

    for (const QString &pattern : presets) {
        if (existing.contains(pattern)) {
            continue;
        }
        if (gitManager->trackWithLfs(pattern)) {
            added.append(pattern);
        } else {
            failed.append(pattern);
        }
    }

    if (added.isEmpty() && failed.isEmpty()) {
        QMessageBox::information(this, tr("跟踪常见大文件类型"),
            tr("预设规则已全部存在，无需变更。"));
        return;
    }

    QString summary = tr("已添加 %1 条 LFS 规则。").arg(added.size());
    if (!failed.isEmpty()) {
        summary += tr("\n失败规则: %1").arg(failed.join(QStringLiteral(", ")));
    }
    QMessageBox::information(this, tr("跟踪常见大文件类型"), summary);
    onGitStatusChanged();
}

// 函数说明：处理云同步操作，把本地文档状态同步到配置的远端。
void MainWindow::syncNow()
{
    if (cloudSync->getAllServices().isEmpty()) {
        QMessageBox::information(this, tr("云同步"),
            tr("请先配置同步服务。"));
        syncShowSettings();
        return;
    }

    // 同步前先保存当前文件
    if (!fileName.isEmpty() && ui->plainTextEdit->document()->isModified()) {
        fileSave();
    }

    // 如果当前文件不在任何同步服务的本地路径中，自动复制过去
    if (!fileName.isEmpty()) {
        QString currentFilePath = QFileInfo(fileName).absoluteFilePath();
        QString currentFileName = QFileInfo(fileName).fileName();

        const QVector<CloudSync::ServiceConfig> services = cloudSync->getAllServices();
        for (const CloudSync::ServiceConfig &config : services) {
            QString localPath = QDir(config.localPath).absolutePath();
            // 如果当前文件不在本地同步路径下，复制一份过去
            if (!currentFilePath.startsWith(localPath + "/") && currentFilePath != localPath) {
                QString targetPath = localPath + "/" + currentFileName;
                // 先删除目标（QFile::copy 不能覆盖）
                if (QFile::exists(targetPath)) {
                    QFile::remove(targetPath);
                }
                QFile::copy(currentFilePath, targetPath);
            }
        }
    }

    // 禁用同步按钮防止重复点击
    if (actionSyncNow) {
        actionSyncNow->setEnabled(false);
    }
    m_manualSync = true;  // 标记为手动同步，完成后弹窗
    statusBar()->showMessage(tr("正在同步..."));

    // 在事件循环空闲时执行同步，确保 UI 先更新
    QTimer::singleShot(0, this, [this]() {
        cloudSync->startSync();

        // 同步完成后重新启用按钮
        if (actionSyncNow) {
            actionSyncNow->setEnabled(true);
        }
    });
}

// 函数说明：处理云同步操作，把本地文档状态同步到配置的远端。
void MainWindow::syncShowSettings()
{
    auto persistCloudSyncConfig = [this]() {
        if (!saveCloudSyncConfig(cloudSync)) {
            const QString detail = cloudSync->lastError().isEmpty()
                                       ? tr("无法写入配置文件: %1").arg(cloudSyncConfigPath())
                                       : cloudSync->lastError();
            QMessageBox::warning(this, tr("云同步"),
                                 tr("保存同步配置失败。\n%1").arg(detail));
        }
    };

    auto showServiceEditor = [this](const QString &title,
                                    const CloudSync::ServiceConfig &initialConfig,
                                    const QStringList &reservedNames,
                                    CloudSync::ServiceConfig *outConfig) -> bool {
        if (!outConfig) {
            return false;
        }

        QDialog editDialog(this);
        editDialog.setWindowTitle(title);
        editDialog.setMinimumWidth(420);

        QFormLayout *form = new QFormLayout(&editDialog);

        QLineEdit *nameEdit = new QLineEdit(&editDialog);
        form->addRow(tr("名称:"), nameEdit);

        QComboBox *typeCombo = new QComboBox(&editDialog);
        typeCombo->addItem(tr("WebDAV (坚果云/NextCloud)"), static_cast<int>(CloudSync::ServiceType::WebDAV));
        typeCombo->addItem(tr("本地文件夹"), static_cast<int>(CloudSync::ServiceType::LocalFolder));
        form->addRow(tr("类型:"), typeCombo);

        QLineEdit *serverEdit = new QLineEdit(&editDialog);
        serverEdit->setPlaceholderText(tr("https://dav.jianguoyun.com/dav/"));
        form->addRow(tr("服务器:"), serverEdit);

        QLineEdit *userEdit = new QLineEdit(&editDialog);
        form->addRow(tr("用户名:"), userEdit);

        QLineEdit *passEdit = new QLineEdit(&editDialog);
        passEdit->setEchoMode(QLineEdit::Password);
        form->addRow(tr("密码:"), passEdit);

        QCheckBox *verifySslCheck = new QCheckBox(tr("验证 SSL 证书"), &editDialog);
        verifySslCheck->setChecked(true);
        form->addRow(QString(), verifySslCheck);

        QLineEdit *fingerprintEdit = new QLineEdit(&editDialog);
        fingerprintEdit->setPlaceholderText(tr("可选：证书 SHA256 指纹（如 AA:BB:...）"));
        form->addRow(tr("证书指纹:"), fingerprintEdit);

        QLineEdit *remoteEdit = new QLineEdit(&editDialog);
        form->addRow(tr("远程路径:"), remoteEdit);

        QLineEdit *localEdit = new QLineEdit(&editDialog);
        form->addRow(tr("本地路径:"), localEdit);

        QCheckBox *resumeTransferCheck = new QCheckBox(tr("启用断点续传"), &editDialog);
        resumeTransferCheck->setChecked(true);
        form->addRow(QString(), resumeTransferCheck);

        connect(typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), &editDialog,
                [=](int) {
                    const bool isWebDav = static_cast<CloudSync::ServiceType>(
                                              typeCombo->currentData().toInt()) ==
                                          CloudSync::ServiceType::WebDAV;
                    serverEdit->setEnabled(isWebDav);
                    userEdit->setEnabled(isWebDav);
                    passEdit->setEnabled(isWebDav);
                    verifySslCheck->setEnabled(isWebDav);
                    fingerprintEdit->setEnabled(isWebDav);
                    resumeTransferCheck->setEnabled(isWebDav);
                });

        CloudSync::ServiceConfig config = initialConfig;
        if (config.name.isEmpty()) {
            config.type = CloudSync::ServiceType::WebDAV;
            config.remotePath = QStringLiteral("/notes");
            config.localPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) +
                               QStringLiteral("/Notes");
            config.verifySslCertificates = true;
            config.resumeTransfer = true;
        }

        nameEdit->setText(config.name);
        typeCombo->setCurrentIndex(qMax(0, typeCombo->findData(static_cast<int>(config.type))));
        serverEdit->setText(config.serverUrl);
        userEdit->setText(config.username);
        passEdit->setText(config.password);
        verifySslCheck->setChecked(config.verifySslCertificates);
        fingerprintEdit->setText(config.pinnedCertificateSha256);
        remoteEdit->setText(config.remotePath);
        localEdit->setText(config.localPath);
        resumeTransferCheck->setChecked(config.resumeTransfer);
        typeCombo->setCurrentIndex(typeCombo->currentIndex());

        QDialogButtonBox *buttons = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &editDialog);
        connect(buttons, &QDialogButtonBox::accepted, [&]() {
            const QString serviceName = nameEdit->text().trimmed();
            if (serviceName.isEmpty()) {
                QMessageBox::warning(&editDialog, tr("输入错误"),
                                     tr("服务名称不能为空。"));
                return;
            }
            if (reservedNames.contains(serviceName)) {
                QMessageBox::warning(&editDialog, tr("输入错误"),
                                     tr("服务名称 '%1' 已存在。").arg(serviceName));
                return;
            }

            outConfig->name = serviceName;
            outConfig->type = static_cast<CloudSync::ServiceType>(typeCombo->currentData().toInt());
            outConfig->serverUrl = serverEdit->text().trimmed();
            outConfig->username = userEdit->text().trimmed();
            outConfig->password = passEdit->text();
            outConfig->remotePath = remoteEdit->text().trimmed();
            outConfig->localPath = localEdit->text().trimmed();
            outConfig->verifySslCertificates = verifySslCheck->isChecked();
            outConfig->pinnedCertificateSha256 = fingerprintEdit->text().trimmed();
            outConfig->resumeTransfer = resumeTransferCheck->isChecked();

            editDialog.accept();
        });
        connect(buttons, &QDialogButtonBox::rejected, &editDialog, &QDialog::reject);
        form->addWidget(buttons);

        return editDialog.exec() == QDialog::Accepted;
    };

    QDialog dialog(this);
    dialog.setWindowTitle(tr("同步设置"));
    dialog.setMinimumWidth(500);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    // 服务列表
    QGroupBox *servicesGroup = new QGroupBox(tr("同步服务"), &dialog);
    QVBoxLayout *servicesLayout = new QVBoxLayout(servicesGroup);

    QListWidget *servicesList = new QListWidget(servicesGroup);
    QVector<CloudSync::ServiceConfig> services = cloudSync->getAllServices();
    for (const CloudSync::ServiceConfig &config : services) {
        servicesList->addItem(config.name);
    }
    servicesLayout->addWidget(servicesList);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *addBtn = new QPushButton(tr("添加..."), servicesGroup);
    QPushButton *editBtn = new QPushButton(tr("编辑..."), servicesGroup);
    QPushButton *removeBtn = new QPushButton(tr("删除"), servicesGroup);
    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(editBtn);
    btnLayout->addWidget(removeBtn);
    servicesLayout->addLayout(btnLayout);

    layout->addWidget(servicesGroup);

    // 添加服务
    connect(addBtn, &QPushButton::clicked, [this, servicesList, showServiceEditor, persistCloudSyncConfig]() {
        CloudSync::ServiceConfig config;
        QStringList reservedNames;
        const QVector<CloudSync::ServiceConfig> existingServices = cloudSync->getAllServices();
        for (const CloudSync::ServiceConfig &existing : existingServices) {
            reservedNames.append(existing.name);
        }

        if (!showServiceEditor(tr("添加同步服务"), config, reservedNames, &config)) {
            return;
        }

        if (!cloudSync->addService(config)) {
            QMessageBox::warning(this, tr("添加失败"), cloudSync->lastError());
            return;
        }

        servicesList->addItem(config.name);
        persistCloudSyncConfig();
    });

    // 编辑服务
    connect(editBtn, &QPushButton::clicked, [this, servicesList, showServiceEditor, persistCloudSyncConfig]() {
        QListWidgetItem *item = servicesList->currentItem();
        if (!item) {
            QMessageBox::information(this, tr("编辑同步服务"),
                                     tr("请先选择要编辑的服务。"));
            return;
        }

        const QString oldName = item->text();
        const CloudSync::ServiceConfig oldConfig = cloudSync->getService(oldName);
        if (oldConfig.name.isEmpty()) {
            QMessageBox::warning(this, tr("编辑失败"),
                                 tr("无法读取当前服务配置。"));
            return;
        }

        CloudSync::ServiceConfig editedConfig = oldConfig;
        QStringList reservedNames;
        const QVector<CloudSync::ServiceConfig> existingServices = cloudSync->getAllServices();
        for (const CloudSync::ServiceConfig &existing : existingServices) {
            if (existing.name != oldName) {
                reservedNames.append(existing.name);
            }
        }

        if (!showServiceEditor(tr("编辑同步服务"), oldConfig, reservedNames, &editedConfig)) {
            return;
        }

        bool success = false;
        if (editedConfig.name == oldName) {
            success = cloudSync->updateService(editedConfig);
        } else {
            if (!cloudSync->removeService(oldName)) {
                QMessageBox::warning(this, tr("编辑失败"), cloudSync->lastError());
                return;
            }

            success = cloudSync->addService(editedConfig);
            if (!success) {
                const QString addError = cloudSync->lastError();
                if (!cloudSync->addService(oldConfig)) {
                    qWarning() << "Failed to restore old cloud sync service after rename failure:" << oldName;
                }
                QMessageBox::warning(this, tr("编辑失败"), addError);
                return;
            }
        }

        if (!success) {
            QMessageBox::warning(this, tr("编辑失败"), cloudSync->lastError());
            return;
        }

        item->setText(editedConfig.name);
        servicesList->setCurrentItem(item);
        persistCloudSyncConfig();
    });

    // 删除服务
    connect(removeBtn, &QPushButton::clicked, [this, servicesList, persistCloudSyncConfig]() {
        QListWidgetItem *item = servicesList->currentItem();
        if (item) {
            if (cloudSync->removeService(item->text())) {
                delete item;
                persistCloudSyncConfig();
            }
        }
    });

    // 按钮
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    dialog.exec();
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onSyncProgress(const QString &fileName, int percent, const QString &message)
{
    Q_UNUSED(fileName);
    statusBar()->showMessage(QString("%1 (%2%)").arg(message).arg(percent));
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onSyncCompleted()
{
    statusBar()->showMessage(tr("同步完成"), 3000);
    if (m_manualSync) {
        m_manualSync = false;
        QMessageBox::information(this, tr("云同步"), tr("同步完成。"));
    }
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onGitStatusChanged()
{
    const bool repoOpen = gitManager && !gitManager->repositoryPath().isEmpty();
    const bool lfsInstalled = GitManager::isGitLfsInstalled();

    if (actionGitHistory) actionGitHistory->setEnabled(repoOpen);
    if (actionGitDiff) actionGitDiff->setEnabled(repoOpen);
    if (actionGitCommit) actionGitCommit->setEnabled(repoOpen);
    if (actionGitEnableLfs) actionGitEnableLfs->setEnabled(repoOpen);
    if (actionGitManageLfs) actionGitManageLfs->setEnabled(repoOpen && lfsInstalled);
    if (actionGitQuickTrackLargeFiles) actionGitQuickTrackLargeFiles->setEnabled(repoOpen && lfsInstalled);

    if (!actionGitEnableLfs) {
        return;
    }

    QString lfsActionText = tr("启用 Git LFS（当前仓库）");
    if (repoOpen && lfsInstalled && gitManager->isLfsEnabled()) {
        lfsActionText = tr("Git LFS 已启用（当前仓库）");
    }
    actionGitEnableLfs->setText(lfsActionText);

    if (repoOpen) {
        const QString branch = gitManager->currentBranch().trimmed();
        if (!branch.isEmpty()) {
            const QString tooltip = tr("当前分支: %1").arg(branch);
            if (actionGitHistory) actionGitHistory->setToolTip(tooltip);
            if (actionGitDiff) actionGitDiff->setToolTip(tooltip);
            if (actionGitCommit) actionGitCommit->setToolTip(tooltip);
        }
    }
}

// ==================== 焦点模式功能 ====================

void MainWindow::setupFocusMode()
{
    focusMode = new FocusMode(ui->plainTextEdit, this);

    // 创建焦点模式菜单
    focusMenu = new QMenu(tr("焦点模式(&F)"), this);
    ui->menuView->addSeparator();
    ui->menuView->addMenu(focusMenu);

    // 启用/禁用焦点模式
    actionFocusMode = new QAction(tr("启用焦点模式"), this);
    actionFocusMode->setCheckable(true);
    actionFocusMode->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F));
    connect(actionFocusMode, &QAction::toggled, this, &MainWindow::viewToggleFocusMode);
    focusMenu->addAction(actionFocusMode);

    focusMenu->addSeparator();

    // 焦点范围子菜单
    QMenu *scopeMenu = new QMenu(tr("焦点范围"), focusMenu);
    QActionGroup *scopeGroup = new QActionGroup(this);
    scopeGroup->setExclusive(true);

    QAction *actionLine = new QAction(tr("当前行"), scopeMenu);
    actionLine->setCheckable(true);
    actionLine->setChecked(true);
    actionLine->setData(static_cast<int>(FocusMode::FocusScope::Line));
    scopeGroup->addAction(actionLine);
    scopeMenu->addAction(actionLine);

    QAction *actionSentence = new QAction(tr("当前句子"), scopeMenu);
    actionSentence->setCheckable(true);
    actionSentence->setData(static_cast<int>(FocusMode::FocusScope::Sentence));
    scopeGroup->addAction(actionSentence);
    scopeMenu->addAction(actionSentence);

    QAction *actionParagraph = new QAction(tr("当前段落"), scopeMenu);
    actionParagraph->setCheckable(true);
    actionParagraph->setData(static_cast<int>(FocusMode::FocusScope::Paragraph));
    scopeGroup->addAction(actionParagraph);
    scopeMenu->addAction(actionParagraph);

    QAction *actionSection = new QAction(tr("当前章节"), scopeMenu);
    actionSection->setCheckable(true);
    actionSection->setData(static_cast<int>(FocusMode::FocusScope::Section));
    scopeGroup->addAction(actionSection);
    scopeMenu->addAction(actionSection);

    connect(scopeGroup, &QActionGroup::triggered, [this](QAction *action) {
        FocusMode::FocusScope scope = static_cast<FocusMode::FocusScope>(action->data().toInt());
        focusMode->setFocusScope(scope);
    });

    focusMenu->addMenu(scopeMenu);

    // 滚动模式子菜单
    QMenu *scrollMenu = new QMenu(tr("滚动模式"), focusMenu);
    QActionGroup *scrollGroup = new QActionGroup(this);
    scrollGroup->setExclusive(true);

    QAction *actionNormal = new QAction(tr("正常"), scrollMenu);
    actionNormal->setCheckable(true);
    actionNormal->setChecked(true);
    actionNormal->setData(static_cast<int>(FocusMode::ScrollMode::Normal));
    scrollGroup->addAction(actionNormal);
    scrollMenu->addAction(actionNormal);

    QAction *actionTypewriter = new QAction(tr("打字机模式"), scrollMenu);
    actionTypewriter->setCheckable(true);
    actionTypewriter->setData(static_cast<int>(FocusMode::ScrollMode::Typewriter));
    scrollGroup->addAction(actionTypewriter);
    scrollMenu->addAction(actionTypewriter);

    connect(scrollGroup, &QActionGroup::triggered, [this](QAction *action) {
        FocusMode::ScrollMode mode = static_cast<FocusMode::ScrollMode>(action->data().toInt());
        focusMode->setScrollMode(mode);
    });

    focusMenu->addMenu(scrollMenu);

    focusMenu->addSeparator();

    // 主题子菜单
    QMenu *themeMenu = new QMenu(tr("焦点模式主题"), focusMenu);

    QAction *actionLightTheme = new QAction(tr("亮色主题"), themeMenu);
    connect(actionLightTheme, &QAction::triggered, [this]() {
        focusMode->applyLightTheme();
    });
    themeMenu->addAction(actionLightTheme);

    QAction *actionDarkTheme = new QAction(tr("暗色主题"), themeMenu);
    connect(actionDarkTheme, &QAction::triggered, [this]() {
        focusMode->applyDarkTheme();
    });
    themeMenu->addAction(actionDarkTheme);

    QAction *actionSepiaTheme = new QAction(tr("复古主题"), themeMenu);
    connect(actionSepiaTheme, &QAction::triggered, [this]() {
        focusMode->applySepiaTheme();
    });
    themeMenu->addAction(actionSepiaTheme);

    focusMenu->addMenu(themeMenu);
}

// 函数说明：处理视图切换逻辑，同步窗口布局和可见状态。
void MainWindow::viewToggleFocusMode(bool checked)
{
    if (checked) {
        focusMode->enable();
    } else {
        focusMode->disable();
    }
}

// 函数说明：实现 MainWindow::focusSetScope 的核心逻辑，供当前模块调用。
void MainWindow::focusSetScope()
{
    // 通过菜单设置
}

// 函数说明：实现 MainWindow::focusSetScrollMode 的核心逻辑，供当前模块调用。
void MainWindow::focusSetScrollMode()
{
    // 通过菜单设置
}

// 函数说明：实现 MainWindow::focusApplyTheme 的核心逻辑，供当前模块调用。
void MainWindow::focusApplyTheme()
{
    // 通过菜单设置
}

// ==================== 模板系统功能 ====================

void MainWindow::setupTemplateSystem()
{
    templateManager = new TemplateManager(this);

    // 创建模板菜单
    templateMenu = new QMenu(tr("模板(&T)"), this);
    menuBar()->insertMenu(ui->menuHelp->menuAction(), templateMenu);

    // 从模板新建
    actionNewFromTemplate = new QAction(tr("从模板新建..."), this);
    actionNewFromTemplate->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N));
    connect(actionNewFromTemplate, &QAction::triggered, this, &MainWindow::fileNewFromTemplate);
    templateMenu->addAction(actionNewFromTemplate);

    templateMenu->addSeparator();

    // 快速模板子菜单
    QMenu *quickTemplatesMenu = new QMenu(tr("快速模板"), templateMenu);

    // 添加所有内置模板作为快速选项
    QVector<TemplateManager::Template> templates = templateManager->getAllTemplates();
    for (const TemplateManager::Template &tmpl : templates) {
        QAction *action = new QAction(tmpl.name, quickTemplatesMenu);
        action->setData(tmpl.id);
        connect(action, &QAction::triggered, [this, tmpl]() {
            templateApply(tmpl.id);
        });
        quickTemplatesMenu->addAction(action);
    }

    templateMenu->addMenu(quickTemplatesMenu);

    templateMenu->addSeparator();

    // 模板管理
    actionManageTemplates = new QAction(tr("管理模板..."), this);
    connect(actionManageTemplates, &QAction::triggered, this, &MainWindow::templateManage);
    templateMenu->addAction(actionManageTemplates);
}

// 函数说明：处理主窗口的文件菜单动作，衔接文档读写和界面状态。
void MainWindow::fileNewFromTemplate()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("从模板新建"));
    dialog.setMinimumSize(600, 500);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    // 分类过滤
    QHBoxLayout *filterLayout = new QHBoxLayout();
    QLabel *categoryLabel = new QLabel(tr("分类:"), &dialog);
    QComboBox *categoryCombo = new QComboBox(&dialog);
    categoryCombo->addItem(tr("全部"), -1);
    categoryCombo->addItem(tr("日常"), static_cast<int>(TemplateManager::Category::Daily));
    categoryCombo->addItem(tr("工作"), static_cast<int>(TemplateManager::Category::Work));
    categoryCombo->addItem(tr("技术"), static_cast<int>(TemplateManager::Category::Technical));
    categoryCombo->addItem(tr("学术"), static_cast<int>(TemplateManager::Category::Academic));
    categoryCombo->addItem(tr("创意"), static_cast<int>(TemplateManager::Category::Creative));
    categoryCombo->addItem(tr("自定义"), static_cast<int>(TemplateManager::Category::Custom));

    filterLayout->addWidget(categoryLabel);
    filterLayout->addWidget(categoryCombo);
    filterLayout->addStretch();
    layout->addLayout(filterLayout);

    // 模板列表
    QListWidget *templateList = new QListWidget(&dialog);
    templateList->setIconSize(QSize(32, 32));

    // 加载模板
    auto loadTemplates = [this, templateList](int category) {
        templateList->clear();
        QVector<TemplateManager::Template> templates;
        if (category == -1) {
            templates = templateManager->getAllTemplates();
        } else {
            templates = templateManager->getTemplatesByCategory(
                static_cast<TemplateManager::Category>(category));
        }

        for (const TemplateManager::Template &tmpl : templates) {
            QListWidgetItem *item = new QListWidgetItem(templateList);
            item->setText(tmpl.name);
            item->setToolTip(tmpl.description);
            item->setData(Qt::UserRole, tmpl.id);
        }
    };

    loadTemplates(-1);

    connect(categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [loadTemplates, categoryCombo](int) {
        loadTemplates(categoryCombo->currentData().toInt());
    });

    layout->addWidget(templateList);

    // 预览区
    QGroupBox *previewGroup = new QGroupBox(tr("预览"), &dialog);
    QVBoxLayout *previewLayout = new QVBoxLayout(previewGroup);
    QPlainTextEdit *previewEdit = new QPlainTextEdit(previewGroup);
    previewEdit->setReadOnly(true);
    previewEdit->setMaximumHeight(150);
    previewLayout->addWidget(previewEdit);
    layout->addWidget(previewGroup);

    // 模板选择预览
    connect(templateList, &QListWidget::currentItemChanged,
            [this, previewEdit](QListWidgetItem *current, QListWidgetItem *) {
        if (current) {
            QString templateId = current->data(Qt::UserRole).toString();
            TemplateManager::Template tmpl = templateManager->getTemplate(templateId);
            previewEdit->setPlainText(tmpl.content);
        }
    });

    // 变量输入
    QGroupBox *varsGroup = new QGroupBox(tr("变量"), &dialog);
    QFormLayout *varsLayout = new QFormLayout(varsGroup);

    QLineEdit *titleEdit = new QLineEdit(varsGroup);
    titleEdit->setPlaceholderText(tr("文档标题"));
    varsLayout->addRow(tr("标题:"), titleEdit);

    QLineEdit *authorEdit = new QLineEdit(varsGroup);
    authorEdit->setText(qgetenv("USER"));
    varsLayout->addRow(tr("作者:"), authorEdit);

    layout->addWidget(varsGroup);

    // 按钮
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("创建"));

    connect(buttonBox, &QDialogButtonBox::accepted, [&]() {
        QListWidgetItem *item = templateList->currentItem();
        if (!item) {
            QMessageBox::warning(this, tr("提示"), tr("请选择一个模板"));
            return;
        }

        QString templateId = item->data(Qt::UserRole).toString();

        QMap<QString, QString> variables;
        variables["title"] = titleEdit->text();
        variables["author"] = authorEdit->text();

        QString content = templateManager->applyTemplate(templateId, variables);

        // 创建新文档
        if (maybeSave()) {
            ui->plainTextEdit->clear();
            ui->plainTextEdit->setPlainText(content);
            ui->plainTextEdit->document()->setModified(true);
            setFileName(QString());
            dialog.accept();
        }
    });
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    dialog.exec();
}

// 函数说明：实现 MainWindow::templateManage 的核心逻辑，供当前模块调用。
void MainWindow::templateManage()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("模板管理"));
    dialog.setMinimumSize(700, 500);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    // 工具栏
    QHBoxLayout *toolbarLayout = new QHBoxLayout();

    QPushButton *addBtn = new QPushButton(tr("添加模板"), &dialog);
    QPushButton *editBtn = new QPushButton(tr("编辑"), &dialog);
    QPushButton *deleteBtn = new QPushButton(tr("删除"), &dialog);
    QPushButton *importBtn = new QPushButton(tr("导入..."), &dialog);
    QPushButton *exportBtn = new QPushButton(tr("导出..."), &dialog);

    toolbarLayout->addWidget(addBtn);
    toolbarLayout->addWidget(editBtn);
    toolbarLayout->addWidget(deleteBtn);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(importBtn);
    toolbarLayout->addWidget(exportBtn);
    layout->addLayout(toolbarLayout);

    // 模板列表
    QTableWidget *templateTable = new QTableWidget(&dialog);
    templateTable->setColumnCount(4);
    templateTable->setHorizontalHeaderLabels({tr("名称"), tr("分类"), tr("描述"), tr("标签")});
    templateTable->horizontalHeader()->setStretchLastSection(true);
    templateTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    templateTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // 加载模板
    auto loadTemplates = [this, templateTable]() {
        templateTable->setRowCount(0);
        QVector<TemplateManager::Template> templates = templateManager->getAllTemplates();
        templateTable->setRowCount(templates.size());

        QStringList categoryNames = {tr("日常"), tr("工作"), tr("技术"), tr("学术"), tr("创意"), tr("自定义")};

        for (int i = 0; i < templates.size(); ++i) {
            const TemplateManager::Template &tmpl = templates[i];
            templateTable->setItem(i, 0, new QTableWidgetItem(tmpl.name));
            int catIdx = static_cast<int>(tmpl.category);
            QString catName = (catIdx >= 0 && catIdx < categoryNames.size()) ? categoryNames[catIdx] : QString();
            templateTable->setItem(i, 1, new QTableWidgetItem(catName));
            templateTable->setItem(i, 2, new QTableWidgetItem(tmpl.description));
            templateTable->setItem(i, 3, new QTableWidgetItem(tmpl.tags.join(", ")));
        }
    };

    loadTemplates();
    layout->addWidget(templateTable);

    // 添加模板
    connect(addBtn, &QPushButton::clicked, [this, loadTemplates]() {
        QDialog addDialog(this);
        addDialog.setWindowTitle(tr("添加模板"));
        addDialog.setMinimumWidth(500);

        QVBoxLayout *addLayout = new QVBoxLayout(&addDialog);
        QFormLayout *form = new QFormLayout();

        QLineEdit *nameEdit = new QLineEdit(&addDialog);
        form->addRow(tr("名称:"), nameEdit);

        QComboBox *categoryCombo = new QComboBox(&addDialog);
        categoryCombo->addItem(tr("日常"), static_cast<int>(TemplateManager::Category::Daily));
        categoryCombo->addItem(tr("工作"), static_cast<int>(TemplateManager::Category::Work));
        categoryCombo->addItem(tr("技术"), static_cast<int>(TemplateManager::Category::Technical));
        categoryCombo->addItem(tr("学术"), static_cast<int>(TemplateManager::Category::Academic));
        categoryCombo->addItem(tr("创意"), static_cast<int>(TemplateManager::Category::Creative));
        categoryCombo->addItem(tr("自定义"), static_cast<int>(TemplateManager::Category::Custom));
        form->addRow(tr("分类:"), categoryCombo);

        QLineEdit *descEdit = new QLineEdit(&addDialog);
        form->addRow(tr("描述:"), descEdit);

        QLineEdit *tagsEdit = new QLineEdit(&addDialog);
        tagsEdit->setPlaceholderText(tr("用逗号分隔"));
        form->addRow(tr("标签:"), tagsEdit);

        addLayout->addLayout(form);

        QLabel *contentLabel = new QLabel(tr("内容:"), &addDialog);
        addLayout->addWidget(contentLabel);

        QPlainTextEdit *contentEdit = new QPlainTextEdit(&addDialog);
        contentEdit->setPlaceholderText(tr("支持变量: {{title}}, {{date}}, {{author}} 等"));
        addLayout->addWidget(contentEdit);

        QDialogButtonBox *buttons = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &addDialog);
        connect(buttons, &QDialogButtonBox::accepted, [&]() {
            TemplateManager::Template tmpl;
            tmpl.name = nameEdit->text();
            tmpl.category = static_cast<TemplateManager::Category>(categoryCombo->currentData().toInt());
            tmpl.description = descEdit->text();
            tmpl.content = contentEdit->toPlainText();
            tmpl.tags = tagsEdit->text().split(",", Qt::SkipEmptyParts);
            for (QString &tag : tmpl.tags) {
                tag = tag.trimmed();
            }

            if (templateManager->addTemplate(tmpl)) {
                loadTemplates();
                addDialog.accept();
            } else {
                QMessageBox::warning(this, tr("错误"), templateManager->lastError());
            }
        });
        connect(buttons, &QDialogButtonBox::rejected, &addDialog, &QDialog::reject);
        addLayout->addWidget(buttons);

        addDialog.exec();
    });

    // 删除模板
    connect(deleteBtn, &QPushButton::clicked, [this, templateTable, loadTemplates]() {
        int row = templateTable->currentRow();
        if (row >= 0) {
            QVector<TemplateManager::Template> templates = templateManager->getAllTemplates();
            if (row < templates.size()) {
                if (QMessageBox::question(this, tr("确认删除"),
                        tr("确定要删除模板 \"%1\" 吗？").arg(templates[row].name)) == QMessageBox::Yes) {
                    templateManager->removeTemplate(templates[row].id);
                    loadTemplates();
                }
            }
        }
    });

    // 导入导出
    connect(importBtn, &QPushButton::clicked, [this, loadTemplates]() {
        QString filePath = QFileDialog::getOpenFileName(this, tr("导入模板"),
            QString(), tr("JSON 文件 (*.json)"));
        if (!filePath.isEmpty()) {
            if (templateManager->importTemplates(filePath)) {
                loadTemplates();
                QMessageBox::information(this, tr("导入成功"), tr("模板导入成功"));
            } else {
                QMessageBox::warning(this, tr("导入失败"), templateManager->lastError());
            }
        }
    });

    connect(exportBtn, &QPushButton::clicked, [this]() {
        QString filePath = QFileDialog::getSaveFileName(this, tr("导出模板"),
            QString(), tr("JSON 文件 (*.json)"));
        if (!filePath.isEmpty()) {
            if (templateManager->exportTemplates(filePath)) {
                QMessageBox::information(this, tr("导出成功"), tr("模板导出成功"));
            } else {
                QMessageBox::warning(this, tr("导出失败"), templateManager->lastError());
            }
        }
    });

    // 按钮
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    dialog.exec();
}

// 函数说明：实现 MainWindow::templateApply 的核心逻辑，供当前模块调用。
void MainWindow::templateApply(const QString &templateId)
{
    QMap<QString, QString> variables;
    QString content = templateManager->applyTemplate(templateId, variables);

    if (!content.isEmpty() && maybeSave()) {
        ui->plainTextEdit->clear();
        ui->plainTextEdit->setPlainText(content);
        ui->plainTextEdit->document()->setModified(true);
        setFileName(QString());
    }
}

// ==================== 多标签编辑功能 ====================

void MainWindow::setupMultiTabEditor()
{
    m_currentTabIndex = 0;
    m_switchingTab = false;

    // 创建 QTabBar 并插入到编辑器上方
    m_tabBar = new QTabBar(this);
    m_tabBar->setTabsClosable(true);
    m_tabBar->setMovable(true);
    m_tabBar->setExpanding(false);
    m_tabBar->setDocumentMode(true);

    // 插入到 verticalLayout_2 的最上方（编辑器上方）
    ui->verticalLayout_2->insertWidget(0, m_tabBar);

    // 创建第一个标签页
    TabData firstTab;
    firstTab.content = ui->plainTextEdit->toPlainText();
    firstTab.filePath = fileName;
    m_tabs.append(firstTab);
    QString tabTitle = fileName.isEmpty() ? tr("untitled.md") : QFileInfo(fileName).fileName();
    m_tabBar->addTab(tabTitle);

    // 连接信号
    connect(m_tabBar, &QTabBar::currentChanged, this, &MainWindow::onTabChanged);
    connect(m_tabBar, &QTabBar::tabCloseRequested, this, &MainWindow::onTabCloseRequested);

    // 创建标签页菜单
    tabMenu = new QMenu(tr("标签页(&B)"), this);
    menuBar()->insertMenu(ui->menuHelp->menuAction(), tabMenu);

    // 新建标签页
    actionTabNew = new QAction(tr("新建标签页"), this);
    actionTabNew->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_T));
    connect(actionTabNew, &QAction::triggered, this, &MainWindow::tabNew);
    tabMenu->addAction(actionTabNew);

    // 关闭当前标签页
    actionTabClose = new QAction(tr("关闭标签页"), this);
    actionTabClose->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_W));
    connect(actionTabClose, &QAction::triggered, this, &MainWindow::tabClose);
    tabMenu->addAction(actionTabClose);

    // 关闭所有标签页
    actionTabCloseAll = new QAction(tr("关闭所有标签页"), this);
    connect(actionTabCloseAll, &QAction::triggered, this, &MainWindow::tabCloseAll);
    tabMenu->addAction(actionTabCloseAll);

    tabMenu->addSeparator();

    // 下一个标签页
    actionTabNext = new QAction(tr("下一个标签页"), this);
    actionTabNext->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Tab));
    connect(actionTabNext, &QAction::triggered, this, &MainWindow::tabNext);
    tabMenu->addAction(actionTabNext);

    // 上一个标签页
    actionTabPrevious = new QAction(tr("上一个标签页"), this);
    actionTabPrevious->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Tab));
    connect(actionTabPrevious, &QAction::triggered, this, &MainWindow::tabPrevious);
    tabMenu->addAction(actionTabPrevious);

    tabMenu->addSeparator();

    // 恢复关闭的标签页
    actionTabRestoreClosed = new QAction(tr("恢复关闭的标签页"), this);
    actionTabRestoreClosed->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T));
    connect(actionTabRestoreClosed, &QAction::triggered, this, &MainWindow::tabRestoreClosed);
    actionTabRestoreClosed->setEnabled(false);
    tabMenu->addAction(actionTabRestoreClosed);

    // Alt+1~9 快捷键切换标签
    for (int i = 1; i <= 9; ++i) {
        QAction *shortcut = new QAction(this);
        shortcut->setShortcut(QKeySequence(Qt::ALT | static_cast<Qt::Key>(Qt::Key_1 + i - 1)));
        connect(shortcut, &QAction::triggered, this, [this, i]() {
            if (i <= m_tabBar->count()) {
                m_tabBar->setCurrentIndex(i - 1);
            }
        });
        addAction(shortcut);
    }
}

// 函数说明：保存 MainWindow 当前状态，保证用户修改可以持久化。
void MainWindow::saveCurrentTabState()
{
    if (m_currentTabIndex < 0 || m_currentTabIndex >= m_tabs.size())
        return;

    TabData &tab = m_tabs[m_currentTabIndex];
    tab.content = ui->plainTextEdit->toPlainText();
    tab.filePath = fileName;
    tab.cursorPosition = ui->plainTextEdit->textCursor().position();
    tab.scrollPosition = ui->plainTextEdit->verticalScrollBar()->value();
    tab.isModified = ui->plainTextEdit->document()->isModified();
    tab.isEncryptedMode = isEncryptedMode;
}

// 函数说明：实现 MainWindow::restoreTabState 的核心逻辑，供当前模块调用。
void MainWindow::restoreTabState(int index)
{
    if (index < 0 || index >= m_tabs.size())
        return;

    m_switchingTab = true;

    const TabData &tab = m_tabs[index];

    // 阻塞编辑器信号以避免触发 plainTextChanged
    ui->plainTextEdit->blockSignals(true);

    ui->plainTextEdit->resetHighlighting();
    ui->plainTextEdit->setPlainText(tab.content);

    // 恢复光标位置
    QTextCursor cursor = ui->plainTextEdit->textCursor();
    cursor.setPosition(qMin(tab.cursorPosition, ui->plainTextEdit->document()->characterCount() - 1));
    ui->plainTextEdit->setTextCursor(cursor);

    // 恢复滚动位置
    ui->plainTextEdit->verticalScrollBar()->setValue(tab.scrollPosition);

    // 恢复修改状态
    ui->plainTextEdit->document()->setModified(tab.isModified);

    ui->plainTextEdit->blockSignals(false);

    // 更新文件名和窗口标题
    fileName = tab.filePath;
    isEncryptedMode = tab.isEncryptedMode;
    setWindowModified(tab.isModified);

    QString shownName = fileName;
    if (shownName.isEmpty()) {
        shownName = tr("untitled.md");
    }
    setWindowFilePath(shownName);

    m_switchingTab = false;

    // 在 m_switchingTab 恢复为 false 之后再触发预览更新
    ui->htmlSourceTextEdit->clear();

    // 注意：generator 用 null QString 作为线程终止信号，
    // 绝不能传入 null，必须确保传入非 null 的空字符串
    if (tab.content.isNull()) {
        ui->webView->setHtml(QString());
        generator->markdownTextChanged(QStringLiteral(""));
    } else {
        generator->markdownTextChanged(tab.content);
    }
}

// 函数说明：刷新 MainWindow 的内部状态，并同步到相关界面。
void MainWindow::updateTabTitle(int index)
{
    if (index < 0 || index >= m_tabs.size())
        return;

    const TabData &tab = m_tabs[index];
    QString title;
    if (tab.filePath.isEmpty()) {
        title = tr("untitled.md");
    } else {
        title = QFileInfo(tab.filePath).fileName();
    }
    if (tab.isModified) {
        title = "* " + title;
    }
    m_tabBar->setTabText(index, title);
}

// 函数说明：实现 MainWindow::tabNew 的核心逻辑，供当前模块调用。
void MainWindow::tabNew()
{
    // 保存当前标签状态
    saveCurrentTabState();
    updateTabTitle(m_currentTabIndex);

    // 创建新标签数据
    TabData newTab;
    m_tabs.append(newTab);

    // 在 TabBar 上添加新标签
    int newIndex = m_tabBar->addTab(tr("untitled.md"));

    // 切换到新标签（会触发 onTabChanged）
    m_tabBar->setCurrentIndex(newIndex);
}

// 函数说明：实现 MainWindow::tabClose 的核心逻辑，供当前模块调用。
void MainWindow::tabClose()
{
    onTabCloseRequested(m_currentTabIndex);
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onTabCloseRequested(int index)
{
    if (index < 0 || index >= m_tabs.size())
        return;

    // 如果只有一个标签，不关闭，而是清空内容
    if (m_tabs.size() <= 1) {
        if (m_tabs[0].isModified) {
            // 切换到该标签先
            if (m_currentTabIndex != 0) {
                m_tabBar->setCurrentIndex(0);
            }
            if (!maybeSave()) {
                return;
            }
        }
        m_tabs[0] = TabData();
        ui->plainTextEdit->blockSignals(true);
        ui->plainTextEdit->clear();
        ui->plainTextEdit->resetHighlighting();
        ui->plainTextEdit->blockSignals(false);
        ui->webView->setHtml(QString());
        ui->htmlSourceTextEdit->clear();
        fileName.clear();
        isEncryptedMode = false;
        setWindowModified(false);
        setWindowFilePath(tr("untitled.md"));
        m_tabBar->setTabText(0, tr("untitled.md"));
        generator->markdownTextChanged(QString());
        return;
    }

    // 如果要关闭的标签有未保存的修改
    const TabData &tab = m_tabs[index];
    if (tab.isModified || (index == m_currentTabIndex && ui->plainTextEdit->document()->isModified())) {
        // 先切到该标签
        if (m_currentTabIndex != index) {
            m_tabBar->setCurrentIndex(index);
        }
        if (!maybeSave()) {
            return;
        }
        // maybeSave 可能已经保存了，更新状态
        saveCurrentTabState();
    }

    // 保存到关闭历史
    TabData closedTab = m_tabs[index];
    if (index == m_currentTabIndex) {
        // 如果关闭当前标签，获取最新内容
        closedTab.content = ui->plainTextEdit->toPlainText();
        closedTab.cursorPosition = ui->plainTextEdit->textCursor().position();
        closedTab.scrollPosition = ui->plainTextEdit->verticalScrollBar()->value();
    }
    m_closedTabs.append(closedTab);
    if (m_closedTabs.size() > MAX_CLOSED_TABS) {
        m_closedTabs.removeFirst();
    }
    actionTabRestoreClosed->setEnabled(true);

    // 移除标签
    m_tabBar->blockSignals(true);
    m_tabs.removeAt(index);
    m_tabBar->removeTab(index);

    // 调整当前索引
    if (index < m_currentTabIndex) {
        m_currentTabIndex--;
    } else if (index == m_currentTabIndex) {
        // 切换到相邻标签
        if (m_currentTabIndex >= m_tabs.size()) {
            m_currentTabIndex = m_tabs.size() - 1;
        }
        m_tabBar->setCurrentIndex(m_currentTabIndex);
        restoreTabState(m_currentTabIndex);
    }

    m_tabBar->blockSignals(false);
}

// 函数说明：实现 MainWindow::tabCloseAll 的核心逻辑，供当前模块调用。
void MainWindow::tabCloseAll()
{
    // 从后向前关闭所有标签（除最后一个）
    while (m_tabs.size() > 1) {
        int lastIndex = m_tabs.size() - 1;
        // 跳过当前标签
        if (lastIndex == m_currentTabIndex && m_tabs.size() > 1) {
            lastIndex = (m_currentTabIndex == 0) ? 1 : 0;
        }
        int prevSize = m_tabs.size();
        onTabCloseRequested(lastIndex);
        // 如果用户取消了保存提示，退出
        if (m_tabs.size() == prevSize) {
            return;
        }
    }
    // 最后关闭剩余的那个
    onTabCloseRequested(0);
}

// 函数说明：实现 MainWindow::tabNext 的核心逻辑，供当前模块调用。
void MainWindow::tabNext()
{
    if (m_tabBar->count() <= 1) return;
    int next = (m_currentTabIndex + 1) % m_tabBar->count();
    m_tabBar->setCurrentIndex(next);
}

// 函数说明：实现 MainWindow::tabPrevious 的核心逻辑，供当前模块调用。
void MainWindow::tabPrevious()
{
    if (m_tabBar->count() <= 1) return;
    int prev = (m_currentTabIndex - 1 + m_tabBar->count()) % m_tabBar->count();
    m_tabBar->setCurrentIndex(prev);
}

// 函数说明：实现 MainWindow::tabRestoreClosed 的核心逻辑，供当前模块调用。
void MainWindow::tabRestoreClosed()
{
    if (m_closedTabs.isEmpty()) return;

    TabData restored = m_closedTabs.takeLast();
    if (m_closedTabs.isEmpty()) {
        actionTabRestoreClosed->setEnabled(false);
    }

    // 保存当前标签状态
    saveCurrentTabState();
    updateTabTitle(m_currentTabIndex);

    // 添加恢复的标签
    m_tabs.append(restored);
    QString title;
    if (restored.filePath.isEmpty()) {
        title = tr("untitled.md");
    } else {
        title = QFileInfo(restored.filePath).fileName();
    }
    if (restored.isModified) {
        title = "* " + title;
    }
    int newIndex = m_tabBar->addTab(title);
    m_tabBar->setCurrentIndex(newIndex);
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onTabChanged(int index)
{
    if (index < 0 || index >= m_tabs.size())
        return;
    if (index == m_currentTabIndex)
        return;

    // 保存旧标签状态
    saveCurrentTabState();
    updateTabTitle(m_currentTabIndex);

    // 恢复新标签状态
    m_currentTabIndex = index;
    restoreTabState(index);
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onTabContentChanged(int index)
{
    Q_UNUSED(index)
}

// ==================== 协作和分享功能 ====================

void MainWindow::setupCollaborationFeatures()
{
    // 创建管理器
    documentSharing = new DocumentSharing(this);
    commentManager = new CommentManager(ui->plainTextEdit, this);
    revisionTracker = new RevisionTracker(this);
    imageExporter = new ImageExporter(this);

    // 连接信号
    connect(documentSharing, &DocumentSharing::shareCreated,
            this, &MainWindow::onShareCreated);
    connect(commentManager, &CommentManager::commentAdded,
            this, &MainWindow::onCommentAdded);
    connect(imageExporter, &ImageExporter::exportCompleted,
            this, [this](const ImageExporter::ExportResult &result) {
                if (result.success) {
                    onImageExportCompleted(result.filePath);
                }
            });

    // 创建分享菜单
    shareMenu = new QMenu(tr("分享(&H)"), this);
    menuBar()->insertMenu(ui->menuHelp->menuAction(), shareMenu);

    actionShareDocument = new QAction(tr("生成分享链接..."), this);
    actionShareDocument->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));
    connect(actionShareDocument, &QAction::triggered, this, &MainWindow::shareDocument);
    shareMenu->addAction(actionShareDocument);

    actionShareManager = new QAction(tr("分享管理..."), this);
    connect(actionShareManager, &QAction::triggered, this, &MainWindow::shareShowManager);
    shareMenu->addAction(actionShareManager);

    shareMenu->addSeparator();

    // 评论功能
    commentMenu = shareMenu->addMenu(tr("评论批注"));

    actionAddComment = new QAction(tr("添加评论..."), this);
    actionAddComment->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_M));
    connect(actionAddComment, &QAction::triggered, this, &MainWindow::addComment);
    commentMenu->addAction(actionAddComment);

    actionShowComments = new QAction(tr("显示评论面板"), this);
    actionShowComments->setCheckable(true);
    connect(actionShowComments, &QAction::toggled, [this](bool checked) {
        if (checked) {
            commentManager->showHighlights();
        } else {
            commentManager->hideHighlights();
        }
    });
    commentMenu->addAction(actionShowComments);

    actionExportComments = new QAction(tr("导出评论..."), this);
    connect(actionExportComments, &QAction::triggered, this, &MainWindow::exportComments);
    commentMenu->addAction(actionExportComments);

    shareMenu->addSeparator();

    // 版本历史
    revisionMenu = shareMenu->addMenu(tr("修订历史"));

    actionRevisionHistory = new QAction(tr("查看修订历史..."), this);
    actionRevisionHistory->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_H));
    connect(actionRevisionHistory, &QAction::triggered, this, &MainWindow::showRevisionHistory);
    revisionMenu->addAction(actionRevisionHistory);

    actionCreateRevision = new QAction(tr("创建修订点..."), this);
    connect(actionCreateRevision, &QAction::triggered, this, &MainWindow::createRevision);
    revisionMenu->addAction(actionCreateRevision);

    actionCompareRevisions = new QAction(tr("比较修订..."), this);
    connect(actionCompareRevisions, &QAction::triggered, this, &MainWindow::compareRevisions);
    revisionMenu->addAction(actionCompareRevisions);

    actionRestoreRevision = new QAction(tr("恢复修订..."), this);
    connect(actionRestoreRevision, &QAction::triggered, this, &MainWindow::restoreRevision);
    revisionMenu->addAction(actionRestoreRevision);

    shareMenu->addSeparator();

    // 图片导出
    actionExportToPng = new QAction(tr("导出为 PNG..."), this);
    connect(actionExportToPng, &QAction::triggered, this, &MainWindow::exportToPng);
    shareMenu->addAction(actionExportToPng);

    actionExportToSvg = new QAction(tr("导出为 SVG..."), this);
    connect(actionExportToSvg, &QAction::triggered, this, &MainWindow::exportToSvg);
    shareMenu->addAction(actionExportToSvg);

    shareMenu->addSeparator();

    // ==================== 实时协作功能 ====================
    // 创建协作管理器
    collaborationManager = new Collaboration::CollaborationManager(this);
    collaborationManager->setEditor(ui->plainTextEdit);

    // 创建协作面板
    collaborationPanel = new Collaboration::CollaborationPanel(this);
    collaborationPanel->setCollaborationManager(collaborationManager);

    // 创建协作面板停靠窗口
    collaborationDock = new QDockWidget(tr("实时协作"), this);
    collaborationDock->setWidget(collaborationPanel);
    collaborationDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, collaborationDock);
    collaborationDock->hide();

    // 创建远程光标覆盖层
    remoteCursorOverlay = new Collaboration::RemoteCursorOverlay(ui->plainTextEdit, this);

    // 连接协作管理器信号
    connect(collaborationManager, &Collaboration::CollaborationManager::modeChanged,
            this, &MainWindow::onCollabModeChanged);
    connect(collaborationManager, &Collaboration::CollaborationManager::sessionStarted,
            this, &MainWindow::onCollabSessionStarted);
    connect(collaborationManager, &Collaboration::CollaborationManager::sessionEnded,
            this, &MainWindow::onCollabSessionEnded);
    connect(collaborationManager, &Collaboration::CollaborationManager::error,
            this, &MainWindow::onCollabError);

    // 连接远程光标更新
    connect(collaborationManager, &Collaboration::CollaborationManager::cursorMoved,
            this, [this](const QString &userId, int position, const QColor &color) {
                auto users = collaborationManager->users();
                QString userName;
                for (const auto &user : users) {
                    if (user.id == userId) {
                        userName = user.name;
                        break;
                    }
                }
                remoteCursorOverlay->updateCursor(userId, userName, position, color);
            });
    connect(collaborationManager, &Collaboration::CollaborationManager::selectionChanged,
            this, [this](const QString &userId, int start, int end, const QColor &color) {
                Q_UNUSED(color);
                remoteCursorOverlay->updateSelection(userId, start, end);
            });
    connect(collaborationManager, &Collaboration::CollaborationManager::userLeft,
            this, [this](const Collaboration::User &user) {
                remoteCursorOverlay->removeCursor(user.id);
            });

    // 创建实时协作菜单
    collabMenu = shareMenu->addMenu(tr("实时协作"));

    actionCollabStartHosting = new QAction(tr("开始主持协作..."), this);
    actionCollabStartHosting->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_H));
    connect(actionCollabStartHosting, &QAction::triggered, this, &MainWindow::collabStartHosting);
    collabMenu->addAction(actionCollabStartHosting);

    actionCollabJoinSession = new QAction(tr("加入协作会话..."), this);
    actionCollabJoinSession->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_J));
    connect(actionCollabJoinSession, &QAction::triggered, this, &MainWindow::collabJoinSession);
    collabMenu->addAction(actionCollabJoinSession);

    collabMenu->addSeparator();

    actionCollabDisconnect = new QAction(tr("断开协作"), this);
    actionCollabDisconnect->setEnabled(false);
    connect(actionCollabDisconnect, &QAction::triggered, this, &MainWindow::collabDisconnect);
    collabMenu->addAction(actionCollabDisconnect);

    collabMenu->addSeparator();

    actionCollabShowPanel = collaborationDock->toggleViewAction();
    actionCollabShowPanel->setText(tr("显示协作面板"));
    collabMenu->addAction(actionCollabShowPanel);
}

// 函数说明：实现 MainWindow::shareDocument 的核心逻辑，供当前模块调用。
void MainWindow::shareDocument()
{
    if (ui->plainTextEdit->toPlainText().isEmpty()) {
        QMessageBox::warning(this, tr("分享"), tr("文档内容为空"));
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(tr("生成分享链接"));
    dialog.setMinimumWidth(400);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QFormLayout *form = new QFormLayout();

    QLineEdit *titleEdit = new QLineEdit(&dialog);
    titleEdit->setText(fileName.isEmpty() ? tr("未命名文档") : QFileInfo(fileName).baseName());
    form->addRow(tr("标题:"), titleEdit);

    QSpinBox *expireSpin = new QSpinBox(&dialog);
    expireSpin->setRange(5, 1440);
    expireSpin->setValue(60);
    expireSpin->setSuffix(tr(" 分钟"));
    form->addRow(tr("有效期:"), expireSpin);

    QLineEdit *passwordEdit = new QLineEdit(&dialog);
    passwordEdit->setPlaceholderText(tr("留空表示无密码"));
    passwordEdit->setEchoMode(QLineEdit::Password);
    form->addRow(tr("访问密码:"), passwordEdit);

    layout->addLayout(form);

    QLabel *urlLabel = new QLabel(&dialog);
    urlLabel->setWordWrap(true);
    urlLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    urlLabel->hide();
    layout->addWidget(urlLabel);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("生成链接"));

    connect(buttonBox, &QDialogButtonBox::accepted, [&]() {
        QString html = generator->exportHtml(QString(), QString());
        QString markdown = ui->plainTextEdit->toPlainText();

        QString shareId = documentSharing->createShare(
            titleEdit->text(),
            html,
            markdown,
            expireSpin->value(),
            passwordEdit->text()
        );

        if (!shareId.isEmpty()) {
            QString url = documentSharing->getShareUrl(shareId);
            urlLabel->setText(tr("分享链接：\n%1\n\n链接已复制到剪贴板").arg(url));
            urlLabel->show();

            QApplication::clipboard()->setText(url);
            buttonBox->button(QDialogButtonBox::Ok)->setText(tr("完成"));
            buttonBox->button(QDialogButtonBox::Cancel)->hide();
        }
    });
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    dialog.exec();
}

// 函数说明：实现 MainWindow::shareShowManager 的核心逻辑，供当前模块调用。
void MainWindow::shareShowManager()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("分享管理"));
    dialog.setMinimumSize(750, 550);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    // 服务状态
    QLabel *statusLabel = new QLabel(&dialog);
    if (documentSharing->isServerRunning()) {
        statusLabel->setText(tr("服务运行中: %1").arg(documentSharing->serverAddress()));
    } else {
        statusLabel->setText(tr("服务未运行"));
    }
    layout->addWidget(statusLabel);

    // 分享列表
    QTableWidget *table = new QTableWidget(&dialog);
    table->setColumnCount(5);
    table->setHorizontalHeaderLabels({tr("标题"), tr("链接"), tr("访问次数"), tr("过期时间"), tr("操作")});
    table->horizontalHeader()->setStretchLastSection(true);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);

    QList<DocumentSharing::ShareInfo> shares = documentSharing->getAllShares();
    table->setRowCount(shares.size());

    for (int i = 0; i < shares.size(); ++i) {
        const DocumentSharing::ShareInfo &share = shares[i];
        table->setItem(i, 0, new QTableWidgetItem(share.title));
        table->setItem(i, 1, new QTableWidgetItem(documentSharing->getShareUrl(share.id)));
        table->setItem(i, 2, new QTableWidgetItem(QString::number(share.currentViews)));
        table->setItem(i, 3, new QTableWidgetItem(share.expireTime.toString("yyyy-MM-dd HH:mm")));

        QPushButton *deleteBtn = new QPushButton(tr("删除"), table);
        connect(deleteBtn, &QPushButton::clicked, [this, shareId = share.id, table, i]() {
            documentSharing->deleteShare(shareId);
            table->removeRow(i);
        });
        table->setCellWidget(i, 4, deleteBtn);
    }

    layout->addWidget(table);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    dialog.exec();
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onShareCreated(const QString &shareId, const QString &url)
{
    Q_UNUSED(shareId)
    statusBar()->showMessage(tr("分享链接已创建: %1").arg(url), 5000);
}

// 函数说明：向 MainWindow 管理的数据集合中添加一项内容。
void MainWindow::addComment()
{
    QTextCursor cursor = ui->plainTextEdit->textCursor();
    if (!cursor.hasSelection()) {
        QMessageBox::information(this, tr("添加评论"), tr("请先选择要评论的文本"));
        return;
    }

    bool ok;
    QString text = QInputDialog::getMultiLineText(this, tr("添加评论"),
        tr("评论内容:"), QString(), &ok);

    if (ok && !text.isEmpty()) {
        commentManager->addCommentAtSelection(text);
    }
}

// 函数说明：显示 MainWindow 管理的面板、对话框或提示信息。
void MainWindow::showComments()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("评论列表"));
    dialog.setMinimumSize(700, 500);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QListWidget *list = new QListWidget(&dialog);

    QVector<CommentManager::Comment> comments = commentManager->getAllComments();
    for (const CommentManager::Comment &comment : comments) {
        QString statusText = (comment.status == CommentManager::CommentStatus::Resolved) ?
                             tr("[已解决]") : tr("[未解决]");
        QString itemText = QString("%1 %2\n%3")
            .arg(statusText)
            .arg(comment.selectedText.left(30))
            .arg(comment.text);

        QListWidgetItem *item = new QListWidgetItem(itemText, list);
        item->setData(Qt::UserRole, comment.id);
    }

    connect(list, &QListWidget::itemDoubleClicked, [this, &dialog](QListWidgetItem *item) {
        QString commentId = item->data(Qt::UserRole).toString();
        commentManager->goToComment(commentId);
        dialog.accept();
    });

    layout->addWidget(list);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    dialog.exec();
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
void MainWindow::exportComments()
{
    QString filePath = QFileDialog::getSaveFileName(this, tr("导出评论"),
        QString(), tr("HTML 文件 (*.html)"));

    if (filePath.isEmpty()) return;

    QString html = commentManager->exportCommentsToHtml();

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(html.toUtf8());
        file.close();
        statusBar()->showMessage(tr("评论已导出到 %1").arg(filePath), 3000);
    }
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onCommentAdded(const QString &commentId)
{
    Q_UNUSED(commentId)
    statusBar()->showMessage(tr("评论已添加"), 2000);
}

// 函数说明：显示 MainWindow 管理的面板、对话框或提示信息。
void MainWindow::showRevisionHistory()
{
    if (fileName.isEmpty()) {
        QMessageBox::information(this, tr("修订历史"), tr("请先保存文档"));
        return;
    }

    revisionTracker->loadRevisions(fileName);

    QDialog dialog(this);
    dialog.setWindowTitle(tr("修订历史"));
    dialog.setMinimumSize(750, 550);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QTableWidget *table = new QTableWidget(&dialog);
    table->setColumnCount(4);
    table->setHorizontalHeaderLabels({tr("时间"), tr("描述"), tr("字数"), tr("类型")});
    table->horizontalHeader()->setStretchLastSection(true);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);

    QVector<RevisionTracker::Revision> revisions = revisionTracker->getRevisions(fileName);
    table->setRowCount(revisions.size());

    for (int i = 0; i < revisions.size(); ++i) {
        const RevisionTracker::Revision &rev = revisions[revisions.size() - 1 - i];
        table->setItem(i, 0, new QTableWidgetItem(rev.timestamp.toString("yyyy-MM-dd HH:mm:ss")));
        table->setItem(i, 1, new QTableWidgetItem(rev.description.isEmpty() ? tr("(无描述)") : rev.description));
        table->setItem(i, 2, new QTableWidgetItem(QString::number(rev.wordCount)));
        table->setItem(i, 3, new QTableWidgetItem(rev.isAutoSave ? tr("自动") : tr("手动")));
    }

    layout->addWidget(table);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *restoreBtn = new QPushButton(tr("恢复此版本"), &dialog);
    QPushButton *compareBtn = new QPushButton(tr("与当前比较"), &dialog);
    btnLayout->addWidget(restoreBtn);
    btnLayout->addWidget(compareBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    connect(restoreBtn, &QPushButton::clicked, [this, table, &revisions, &dialog]() {
        int row = table->currentRow();
        if (row >= 0 && row < revisions.size()) {
            QString content = revisionTracker->restoreRevision(revisions[revisions.size() - 1 - row].id);
            if (!content.isEmpty()) {
                ui->plainTextEdit->setPlainText(content);
                dialog.accept();
            }
        }
    });

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    dialog.exec();
}

// 函数说明：创建 MainWindow 需要的对象、记录或输出内容。
void MainWindow::createRevision()
{
    if (fileName.isEmpty()) {
        QMessageBox::information(this, tr("创建修订"), tr("请先保存文档"));
        return;
    }

    bool ok;
    QString description = QInputDialog::getText(this, tr("创建修订点"),
        tr("修订描述:"), QLineEdit::Normal, QString(), &ok);

    if (ok) {
        revisionTracker->createRevision(fileName, ui->plainTextEdit->toPlainText(), description, false);
        statusBar()->showMessage(tr("修订点已创建"), 2000);
    }
}

// 函数说明：实现 MainWindow::compareRevisions 的核心逻辑，供当前模块调用。
void MainWindow::compareRevisions()
{
    showRevisionHistory();  // 暂时复用历史对话框
}

// 函数说明：实现 MainWindow::restoreRevision 的核心逻辑，供当前模块调用。
void MainWindow::restoreRevision()
{
    showRevisionHistory();  // 暂时复用历史对话框
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
void MainWindow::exportToImage()
{
    exportToPng();
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
void MainWindow::exportToPng()
{
    QString filePath = QFileDialog::getSaveFileName(this, tr("导出为 PNG"),
        QString(), tr("PNG 图片 (*.png)"));

    if (filePath.isEmpty()) return;

    QString html = generator->exportHtml(QString(), QString());
    imageExporter->exportHtmlToPng(html, filePath);

    statusBar()->showMessage(tr("正在导出..."));
}

// 函数说明：执行导出流程，把当前 Markdown 内容转换为目标格式。
void MainWindow::exportToSvg()
{
    QString filePath = QFileDialog::getSaveFileName(this, tr("导出为 SVG"),
        QString(), tr("SVG 文件 (*.svg)"));

    if (filePath.isEmpty()) return;

    QString html = generator->exportHtml(QString(), QString());
    imageExporter->exportHtmlToSvg(html, filePath);

    statusBar()->showMessage(tr("正在导出..."));
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onImageExportCompleted(const QString &path)
{
    statusBar()->showMessage(tr("已导出到 %1").arg(path), 3000);

    QMessageBox::StandardButton reply = QMessageBox::question(this, tr("导出完成"),
        tr("图片已导出到:\n%1\n\n是否打开文件位置？").arg(path),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).path()));
    }
}

// ============================================================================
// 实时协作功能实现
// ============================================================================

void MainWindow::collabStartHosting()
{
    // 显示协作面板
    collaborationDock->show();
}

// 函数说明：实现 MainWindow::collabJoinSession 的核心逻辑，供当前模块调用。
void MainWindow::collabJoinSession()
{
    // 显示协作面板
    collaborationDock->show();
}

// 函数说明：实现 MainWindow::collabDisconnect 的核心逻辑，供当前模块调用。
void MainWindow::collabDisconnect()
{
    if (!collaborationManager) return;

    if (collaborationManager->isHost()) {
        int result = QMessageBox::question(this, tr("停止协作"),
            tr("确定要停止主持协作吗？所有参与者将被断开连接。"),
            QMessageBox::Yes | QMessageBox::No);

        if (result == QMessageBox::Yes) {
            collaborationManager->stopHosting();
        }
    } else {
        collaborationManager->leaveSession();
    }
}

// 函数说明：实现 MainWindow::collabShowPanel 的核心逻辑，供当前模块调用。
void MainWindow::collabShowPanel(bool checked)
{
    collaborationDock->setVisible(checked);
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onCollabModeChanged(Collaboration::CollaborationManager::Mode mode)
{
    bool isActive = (mode != Collaboration::CollaborationManager::Mode::Offline);

    actionCollabStartHosting->setEnabled(!isActive);
    actionCollabJoinSession->setEnabled(!isActive);
    actionCollabDisconnect->setEnabled(isActive);

    if (isActive) {
        statusBar()->showMessage(tr("实时协作已启用"), 3000);
    }
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onCollabSessionStarted(const QString &sessionId, const QString &url)
{
    Q_UNUSED(url);

    collaborationDock->show();

    if (collaborationManager->isHost()) {
        statusBar()->showMessage(tr("协作会话已创建: %1").arg(sessionId), 5000);

        // 复制链接到剪贴板
        QString shareUrl = collaborationManager->sessionUrl();
        if (!shareUrl.isEmpty()) {
            QApplication::clipboard()->setText(shareUrl);
            QMessageBox::information(this, tr("协作已开始"),
                tr("协作会话 ID: %1\n\n会话链接已复制到剪贴板，可以分享给其他人加入协作。").arg(sessionId));
        }
    } else {
        statusBar()->showMessage(tr("已加入协作会话: %1").arg(sessionId), 3000);
    }
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onCollabSessionEnded()
{
    remoteCursorOverlay->clearAllCursors();
    statusBar()->showMessage(tr("协作会话已结束"), 3000);
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onCollabError(const QString &error)
{
    QMessageBox::warning(this, tr("协作错误"), error);
}

// ============================================================================
// 写作辅助功能实现
// ============================================================================

void MainWindow::setupWritingAssistance()
{
    // 创建写作辅助组件
    wordCountPanel = new WordCountPanel(this);
    outlineNavigator = new OutlineNavigator(this);
    bookmarkManager = new BookmarkManager(this);
    enhancedSpellChecker = new EnhancedSpellChecker(this);
    writingGoal = new WritingGoal(this);

    // 连接到编辑器
    wordCountPanel->setEditor(ui->plainTextEdit);
    outlineNavigator->setEditor(ui->plainTextEdit);
    bookmarkManager->setEditor(ui->plainTextEdit);
    enhancedSpellChecker->setEditor(ui->plainTextEdit);
    writingGoal->setEditor(ui->plainTextEdit);

    // 创建字数统计面板 Dock
    wordCountDock = new QDockWidget(tr("字数统计"), this);
    wordCountDock->setObjectName("wordCountDock");
    wordCountDock->setWidget(wordCountPanel);
    wordCountDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, wordCountDock);
    wordCountDock->hide();

    // 创建大纲导航 Dock
    outlineDock = new QDockWidget(tr("文档大纲"), this);
    outlineDock->setObjectName("outlineDock");
    outlineDock->setWidget(outlineNavigator);
    outlineDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, outlineDock);
    outlineDock->hide();

    // 创建写作目标 Dock
    writingGoalDock = new QDockWidget(tr("写作目标"), this);
    writingGoalDock->setObjectName("writingGoalDock");
    writingGoalDock->setWidget(writingGoal);
    writingGoalDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, writingGoalDock);
    writingGoalDock->hide();

    // 创建写作菜单
    writingMenu = new QMenu(tr("写作辅助(&W)"), this);
    menuBar()->insertMenu(ui->menuExtras->menuAction(), writingMenu);

    // 字数统计
    actionToggleWordCount = new QAction(tr("显示字数统计"), this);
    actionToggleWordCount->setCheckable(true);
    actionToggleWordCount->setShortcut(QKeySequence("Ctrl+Shift+W"));
    connect(actionToggleWordCount, &QAction::toggled, this, &MainWindow::viewToggleWordCount);
    writingMenu->addAction(actionToggleWordCount);

    // 大纲导航
    actionToggleOutline = new QAction(tr("显示文档大纲"), this);
    actionToggleOutline->setCheckable(true);
    actionToggleOutline->setShortcut(QKeySequence("Ctrl+Shift+O"));
    connect(actionToggleOutline, &QAction::toggled, this, &MainWindow::viewToggleOutline);
    writingMenu->addAction(actionToggleOutline);

    writingMenu->addSeparator();

    // 书签子菜单
    bookmarkMenu = new QMenu(tr("书签"), this);
    writingMenu->addMenu(bookmarkMenu);

    actionBookmarkAdd = new QAction(tr("添加书签"), this);
    actionBookmarkAdd->setShortcut(QKeySequence("Ctrl+B"));
    connect(actionBookmarkAdd, &QAction::triggered, this, &MainWindow::bookmarkAdd);
    bookmarkMenu->addAction(actionBookmarkAdd);

    actionBookmarkRemove = new QAction(tr("删除书签"), this);
    connect(actionBookmarkRemove, &QAction::triggered, this, &MainWindow::bookmarkRemove);
    bookmarkMenu->addAction(actionBookmarkRemove);

    bookmarkMenu->addSeparator();

    actionBookmarkNext = new QAction(tr("下一个书签"), this);
    actionBookmarkNext->setShortcut(QKeySequence("F2"));
    connect(actionBookmarkNext, &QAction::triggered, this, &MainWindow::bookmarkNext);
    bookmarkMenu->addAction(actionBookmarkNext);

    actionBookmarkPrevious = new QAction(tr("上一个书签"), this);
    actionBookmarkPrevious->setShortcut(QKeySequence("Shift+F2"));
    connect(actionBookmarkPrevious, &QAction::triggered, this, &MainWindow::bookmarkPrevious);
    bookmarkMenu->addAction(actionBookmarkPrevious);

    bookmarkMenu->addSeparator();

    actionBookmarkList = new QAction(tr("书签列表..."), this);
    connect(actionBookmarkList, &QAction::triggered, this, &MainWindow::bookmarkShowList);
    bookmarkMenu->addAction(actionBookmarkList);

    writingMenu->addSeparator();

    // 拼写检查
    actionSpellCheck = new QAction(tr("检查拼写"), this);
    actionSpellCheck->setShortcut(QKeySequence("F7"));
    connect(actionSpellCheck, &QAction::triggered, this, &MainWindow::spellCheckDocument);
    writingMenu->addAction(actionSpellCheck);

    actionAddToDict = new QAction(tr("添加到词典"), this);
    connect(actionAddToDict, &QAction::triggered, this, &MainWindow::spellCheckAddToDict);
    writingMenu->addAction(actionAddToDict);

    actionManageDict = new QAction(tr("管理词典..."), this);
    connect(actionManageDict, &QAction::triggered, this, &MainWindow::spellCheckManageDict);
    writingMenu->addAction(actionManageDict);

    writingMenu->addSeparator();

    // 写作目标
    actionSetDailyGoal = new QAction(tr("设置每日目标..."), this);
    connect(actionSetDailyGoal, &QAction::triggered, this, &MainWindow::writingSetDailyGoal);
    writingMenu->addAction(actionSetDailyGoal);

    actionSetSessionGoal = new QAction(tr("设置会话目标..."), this);
    connect(actionSetSessionGoal, &QAction::triggered, this, &MainWindow::writingSetSessionGoal);
    writingMenu->addAction(actionSetSessionGoal);

    actionWritingStats = new QAction(tr("写作统计..."), this);
    connect(actionWritingStats, &QAction::triggered, this, &MainWindow::writingShowStats);
    writingMenu->addAction(actionWritingStats);

    // 连接信号
    connect(bookmarkManager, &BookmarkManager::bookmarkNavigated,
            this, &MainWindow::onBookmarkNavigated);

    connect(writingGoal, &WritingGoal::goalCompleted,
            this, [this](WritingGoal::GoalType type) {
                Q_UNUSED(type)
                onWritingGoalCompleted();
            });

    connect(outlineNavigator, &OutlineNavigator::itemClicked,
            this, [this](int lineNumber, int position) {
                Q_UNUSED(position)
                statusBar()->showMessage(tr("跳转到第 %1 行").arg(lineNumber), 2000);
            });
}

// 函数说明：处理视图切换逻辑，同步窗口布局和可见状态。
void MainWindow::viewToggleWordCount(bool checked)
{
    if (wordCountDock) {
        wordCountDock->setVisible(checked);
    }
}

// 函数说明：处理视图切换逻辑，同步窗口布局和可见状态。
void MainWindow::viewToggleOutline(bool checked)
{
    if (outlineDock) {
        outlineDock->setVisible(checked);
    }
}

// 函数说明：实现 MainWindow::bookmarkAdd 的核心逻辑，供当前模块调用。
void MainWindow::bookmarkAdd()
{
    if (!bookmarkManager) return;

    bool ok;
    QString name = QInputDialog::getText(this, tr("添加书签"),
        tr("书签名称:"), QLineEdit::Normal, QString(), &ok);

    if (ok) {
        QString id = bookmarkManager->addBookmarkAtCursor(name);
        if (!id.isEmpty()) {
            statusBar()->showMessage(tr("书签已添加"), 2000);
        } else {
            statusBar()->showMessage(tr("此行已有书签"), 2000);
        }
    }
}

// 函数说明：实现 MainWindow::bookmarkRemove 的核心逻辑，供当前模块调用。
void MainWindow::bookmarkRemove()
{
    if (!bookmarkManager) return;

    QTextCursor cursor = ui->plainTextEdit->textCursor();
    int lineNumber = cursor.blockNumber() + 1;

    if (bookmarkManager->removeBookmarkAtLine(lineNumber)) {
        statusBar()->showMessage(tr("书签已删除"), 2000);
    } else {
        statusBar()->showMessage(tr("此行没有书签"), 2000);
    }
}

// 函数说明：实现 MainWindow::bookmarkNext 的核心逻辑，供当前模块调用。
void MainWindow::bookmarkNext()
{
    if (bookmarkManager) {
        bookmarkManager->gotoNextBookmark();
    }
}

// 函数说明：实现 MainWindow::bookmarkPrevious 的核心逻辑，供当前模块调用。
void MainWindow::bookmarkPrevious()
{
    if (bookmarkManager) {
        bookmarkManager->gotoPreviousBookmark();
    }
}

// 函数说明：实现 MainWindow::bookmarkShowList 的核心逻辑，供当前模块调用。
void MainWindow::bookmarkShowList()
{
    if (!bookmarkManager) return;

    QVector<BookmarkManager::Bookmark> bookmarks = bookmarkManager->getAllBookmarks();

    if (bookmarks.isEmpty()) {
        QMessageBox::information(this, tr("书签列表"), tr("没有书签"));
        return;
    }

    // 创建书签列表对话框
    QDialog dialog(this);
    dialog.setWindowTitle(tr("书签列表"));
    dialog.resize(750, 550);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QListWidget *listWidget = new QListWidget(&dialog);
    for (const auto &bookmark : bookmarks) {
        QString text = QString("%1 (第 %2 行): %3")
            .arg(bookmark.name)
            .arg(bookmark.lineNumber)
            .arg(bookmark.linePreview);
        QListWidgetItem *item = new QListWidgetItem(text);
        item->setData(Qt::UserRole, bookmark.id);
        listWidget->addItem(item);
    }
    layout->addWidget(listWidget);

    QDialogButtonBox *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    connect(listWidget, &QListWidget::itemDoubleClicked,
            [this, &dialog](QListWidgetItem *item) {
                QString id = item->data(Qt::UserRole).toString();
                bookmarkManager->gotoBookmark(id);
                dialog.accept();
            });

    dialog.exec();
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onBookmarkNavigated(const QString &id)
{
    BookmarkManager::Bookmark bookmark = bookmarkManager->getBookmark(id);
    statusBar()->showMessage(tr("跳转到书签: %1").arg(bookmark.name), 2000);
}

// 函数说明：实现 MainWindow::spellCheckDocument 的核心逻辑，供当前模块调用。
void MainWindow::spellCheckDocument()
{
    if (!enhancedSpellChecker) return;

    enhancedSpellChecker->checkDocument();

    int errorCount = enhancedSpellChecker->errorCount();
    if (errorCount == 0) {
        QMessageBox::information(this, tr("拼写检查"),
            tr("没有发现拼写错误"));
        return;
    }

    // 获取错误列表并显示详细对话框
    QString text = ui->plainTextEdit->toPlainText();
    auto errors = enhancedSpellChecker->checkText(text);

    QDialog dialog(this);
    dialog.setWindowTitle(tr("拼写检查 - 发现 %1 个错误").arg(errors.size()));
    dialog.resize(750, 550);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    // 错误列表
    QTreeWidget *tree = new QTreeWidget(&dialog);
    tree->setHeaderLabels({tr("错误单词"), tr("位置(行)"), tr("建议修改")});
    tree->setRootIsDecorated(false);
    tree->setAlternatingRowColors(true);
    tree->header()->setStretchLastSection(true);

    for (const auto &error : errors) {
        if (error.isIgnored) continue;

        // 计算行号
        int line = text.left(error.position).count('\n') + 1;

        QTreeWidgetItem *item = new QTreeWidgetItem(tree);
        item->setText(0, error.word);
        item->setText(1, QString::number(line));
        item->setText(2, error.suggestions.isEmpty() ?
            tr("(无建议)") : error.suggestions.mid(0, 5).join(", "));
        item->setData(0, Qt::UserRole, error.position);
        item->setData(0, Qt::UserRole + 1, error.length);
    }

    tree->resizeColumnToContents(0);
    tree->resizeColumnToContents(1);
    layout->addWidget(tree);

    // 点击条目跳转到对应位置
    connect(tree, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem *item, int) {
        int pos = item->data(0, Qt::UserRole).toInt();
        int len = item->data(0, Qt::UserRole + 1).toInt();
        QTextCursor cursor = ui->plainTextEdit->textCursor();
        cursor.setPosition(pos);
        cursor.setPosition(pos + len, QTextCursor::KeepAnchor);
        ui->plainTextEdit->setTextCursor(cursor);
        ui->plainTextEdit->ensureCursorVisible();
    });

    // 按钮
    QHBoxLayout *btnLayout = new QHBoxLayout;
    QPushButton *closeBtn = new QPushButton(tr("关闭"), &dialog);
    connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);
    layout->addLayout(btnLayout);

    dialog.exec();
}

// 函数说明：实现 MainWindow::spellCheckAddToDict 的核心逻辑，供当前模块调用。
void MainWindow::spellCheckAddToDict()
{
    if (!enhancedSpellChecker) return;

    QTextCursor cursor = ui->plainTextEdit->textCursor();
    QString word = cursor.selectedText();

    if (word.isEmpty()) {
        // 获取光标处的单词
        cursor.select(QTextCursor::WordUnderCursor);
        word = cursor.selectedText();
    }

    if (!word.isEmpty()) {
        enhancedSpellChecker->addToUserDictionary(word);
        statusBar()->showMessage(tr("已添加到词典: %1").arg(word), 2000);
    }
}

// 函数说明：实现 MainWindow::spellCheckManageDict 的核心逻辑，供当前模块调用。
void MainWindow::spellCheckManageDict()
{
    if (!enhancedSpellChecker) return;

    QDialog dialog(this);
    dialog.setWindowTitle(tr("管理词典"));
    dialog.resize(650, 550);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QLabel *label = new QLabel(tr("用户词典 (每行一个单词):"), &dialog);
    layout->addWidget(label);

    QTextEdit *textEdit = new QTextEdit(&dialog);
    QStringList words = enhancedSpellChecker->userDictionary();
    textEdit->setPlainText(words.join("\n"));
    layout->addWidget(textEdit);

    QDialogButtonBox *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() == QDialog::Accepted) {
        // 更新词典
        QStringList newWords = textEdit->toPlainText().split("\n", Qt::SkipEmptyParts);

        // 先清空现有词典，然后添加新词
        for (const QString &word : words) {
            enhancedSpellChecker->removeFromUserDictionary(word);
        }
        for (const QString &word : newWords) {
            enhancedSpellChecker->addToUserDictionary(word.trimmed());
        }

        statusBar()->showMessage(tr("词典已更新"), 2000);
    }
}

// 函数说明：实现 MainWindow::writingSetDailyGoal 的核心逻辑，供当前模块调用。
void MainWindow::writingSetDailyGoal()
{
    if (!writingGoal) return;

    WritingGoal::Goal currentGoal = writingGoal->getDailyGoal();
    int currentTarget = currentGoal.targetWords > 0 ? currentGoal.targetWords : 500;

    bool ok;
    int target = QInputDialog::getInt(this, tr("设置每日目标"),
        tr("每日字数目标:"), currentTarget, 100, 100000, 100, &ok);

    if (ok) {
        writingGoal->setDailyGoal(target);
        writingGoalDock->show();
        actionToggleWordCount->setChecked(true);
        statusBar()->showMessage(tr("每日目标已设置: %1 字").arg(target), 2000);
    }
}

// 函数说明：实现 MainWindow::writingSetSessionGoal 的核心逻辑，供当前模块调用。
void MainWindow::writingSetSessionGoal()
{
    if (!writingGoal) return;

    WritingGoal::Goal currentGoal = writingGoal->getSessionGoal();
    int currentTarget = currentGoal.targetWords > 0 ? currentGoal.targetWords : 500;

    bool ok;
    int target = QInputDialog::getInt(this, tr("设置会话目标"),
        tr("本次会话字数目标:"), currentTarget, 50, 50000, 50, &ok);

    if (ok) {
        writingGoal->setSessionGoal(target);
        writingGoalDock->show();
        statusBar()->showMessage(tr("会话目标已设置: %1 字").arg(target), 2000);
    }
}

// 函数说明：实现 MainWindow::writingShowStats 的核心逻辑，供当前模块调用。
void MainWindow::writingShowStats()
{
    if (!writingGoal) return;

    WritingGoal::WritingStats stats = writingGoal->getStats();

    QString message = tr(
        "写作统计\n\n"
        "今日字数: %1\n"
        "本周字数: %2\n"
        "本月字数: %3\n\n"
        "连续写作: %4 天\n"
        "最长连续: %5 天\n"
        "日均字数: %6\n\n"
        "总写作时间: %7 分钟"
    ).arg(stats.totalWordsToday)
     .arg(stats.totalWordsThisWeek)
     .arg(stats.totalWordsThisMonth)
     .arg(stats.consecutiveDays)
     .arg(stats.bestStreak)
     .arg(stats.averageWordsPerDay)
     .arg(stats.totalSessionMinutes);

    QMessageBox::information(this, tr("写作统计"), message);
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onWritingGoalCompleted()
{
    // 目标完成时的额外处理（如果需要）
    statusBar()->showMessage(tr("🎉 写作目标已完成！"), 5000);
}

// ============================================================================
// 预览和渲染功能实现
// ============================================================================

void MainWindow::setupPreviewFeatures()
{
    // 创建预览主题管理器
    previewThemeManager = new PreviewThemeManager(this);

    // 创建演示模式
    presentationMode = new PresentationMode();

    // 创建打印预览对话框
    printPreviewDialog = new PrintPreviewDialog(this);

    // 创建 TOC 悬浮窗
    tocFloatingWindow = new TocFloatingWindow(this);
    tocFloatingWindow->setEditor(ui->plainTextEdit);
    tocFloatingWindow->setWebView(ui->webView);
    tocFloatingWindow->setParentWindow(this);

    // 创建预览菜单
    previewMenu = new QMenu(tr("预览(&P)"), this);
    menuBar()->insertMenu(ui->menuExtras->menuAction(), previewMenu);

    // 预览主题子菜单
    previewThemeMenu = new QMenu(tr("预览主题"), this);
    previewMenu->addMenu(previewThemeMenu);

    // 添加内置主题
    QStringList themeIds = previewThemeManager->themeIds();
    for (const QString &id : themeIds) {
        PreviewThemeManager::ThemeStyle theme = previewThemeManager->getTheme(id);
        QAction *action = new QAction(theme.name, this);
        action->setData(id);
        action->setCheckable(true);
        connect(action, &QAction::triggered, this, [this, id]() {
            previewThemeManager->setCurrentTheme(id);
        });
        previewThemeMenu->addAction(action);
    }

    previewThemeMenu->addSeparator();

    actionPreviewEditTheme = new QAction(tr("编辑主题..."), this);
    connect(actionPreviewEditTheme, &QAction::triggered, this, &MainWindow::previewEditTheme);
    previewThemeMenu->addAction(actionPreviewEditTheme);

    actionPreviewImportTheme = new QAction(tr("导入主题..."), this);
    connect(actionPreviewImportTheme, &QAction::triggered, this, &MainWindow::previewImportTheme);
    previewThemeMenu->addAction(actionPreviewImportTheme);

    actionPreviewExportTheme = new QAction(tr("导出主题..."), this);
    connect(actionPreviewExportTheme, &QAction::triggered, this, &MainWindow::previewExportTheme);
    previewThemeMenu->addAction(actionPreviewExportTheme);

    previewMenu->addSeparator();

    // 演示模式
    actionStartPresentation = new QAction(tr("开始演示"), this);
    actionStartPresentation->setShortcut(QKeySequence("F5"));
    connect(actionStartPresentation, &QAction::triggered, this, &MainWindow::startPresentation);
    previewMenu->addAction(actionStartPresentation);

    previewMenu->addSeparator();

    // 打印预览
    actionPrintPreview = new QAction(tr("打印预览..."), this);
    actionPrintPreview->setShortcut(QKeySequence("Ctrl+Shift+P"));
    connect(actionPrintPreview, &QAction::triggered, this, &MainWindow::showPrintPreview);
    previewMenu->addAction(actionPrintPreview);

    previewMenu->addSeparator();

    // TOC 悬浮窗
    actionToggleTocFloat = new QAction(tr("显示悬浮目录"), this);
    actionToggleTocFloat->setCheckable(true);
    actionToggleTocFloat->setShortcut(QKeySequence("Ctrl+Shift+T"));
    connect(actionToggleTocFloat, &QAction::triggered, this, &MainWindow::toggleTocFloating);
    previewMenu->addAction(actionToggleTocFloat);

    // 连接信号
    connect(previewThemeManager, &PreviewThemeManager::themeChanged,
            this, &MainWindow::onPreviewThemeChanged);

    connect(presentationMode, &PresentationMode::presentationEnded,
            this, &MainWindow::onPresentationEnded);

    connect(tocFloatingWindow, &TocFloatingWindow::itemClicked,
            this, &MainWindow::onTocItemClicked);

    connect(tocFloatingWindow, &TocFloatingWindow::visibilityChanged,
            actionToggleTocFloat, &QAction::setChecked);

    // 当文档内容变化时更新 TOC
    connect(ui->plainTextEdit, &QPlainTextEdit::textChanged,
            this, [this]() {
                if (tocFloatingWindow->isVisible()) {
                    tocFloatingWindow->updateFromMarkdown(ui->plainTextEdit->toPlainText());
                }
            });
}

// 函数说明：更新预览相关状态，保持 Markdown、HTML 和目录视图一致。
void MainWindow::previewChangeTheme()
{
    // 主题选择通过菜单完成
}

// 函数说明：更新预览相关状态，保持 Markdown、HTML 和目录视图一致。
void MainWindow::previewEditTheme()
{
    // 简化版：显示当前主题的 CSS
    PreviewThemeManager::ThemeStyle theme = previewThemeManager->currentTheme();

    QDialog dialog(this);
    dialog.setWindowTitle(tr("编辑预览主题"));
    dialog.resize(800, 600);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QLabel *nameLabel = new QLabel(tr("主题名称:"), &dialog);
    layout->addWidget(nameLabel);

    QLineEdit *nameEdit = new QLineEdit(theme.name, &dialog);
    nameEdit->setEnabled(!theme.isBuiltin);
    layout->addWidget(nameEdit);

    QLabel *cssLabel = new QLabel(tr("自定义 CSS:"), &dialog);
    layout->addWidget(cssLabel);

    QTextEdit *cssEdit = new QTextEdit(&dialog);
    cssEdit->setPlainText(theme.customCss);
    cssEdit->setEnabled(!theme.isBuiltin);
    layout->addWidget(cssEdit);

    if (theme.isBuiltin) {
        QLabel *warningLabel = new QLabel(tr("内置主题不可编辑，请创建副本"), &dialog);
        warningLabel->setStyleSheet("color: orange;");
        layout->addWidget(warningLabel);
    }

    QDialogButtonBox *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() == QDialog::Accepted && !theme.isBuiltin) {
        theme.name = nameEdit->text();
        theme.customCss = cssEdit->toPlainText();
        previewThemeManager->updateTheme(theme);
        statusBar()->showMessage(tr("主题已保存"), 2000);
    }
}

// 函数说明：更新预览相关状态，保持 Markdown、HTML 和目录视图一致。
void MainWindow::previewImportTheme()
{
    QString filePath = QFileDialog::getOpenFileName(this,
        tr("导入主题"),
        QString(),
        tr("主题文件 (*.json)"));

    if (!filePath.isEmpty()) {
        if (previewThemeManager->importTheme(filePath)) {
            statusBar()->showMessage(tr("主题已导入"), 2000);
        } else {
            QMessageBox::warning(this, tr("导入失败"),
                tr("无法导入主题文件"));
        }
    }
}

// 函数说明：更新预览相关状态，保持 Markdown、HTML 和目录视图一致。
void MainWindow::previewExportTheme()
{
    QString themeId = previewThemeManager->currentThemeId();
    PreviewThemeManager::ThemeStyle theme = previewThemeManager->getTheme(themeId);

    QString filePath = QFileDialog::getSaveFileName(this,
        tr("导出主题"),
        theme.name + ".json",
        tr("主题文件 (*.json)"));

    if (!filePath.isEmpty()) {
        if (previewThemeManager->exportTheme(themeId, filePath)) {
            statusBar()->showMessage(tr("主题已导出到 %1").arg(filePath), 2000);
        } else {
            QMessageBox::warning(this, tr("导出失败"),
                tr("无法导出主题"));
        }
    }
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onPreviewThemeChanged(const QString &themeId)
{
    Q_UNUSED(themeId)

    // 获取新主题的 CSS 并应用到预览
    currentPreviewCss = previewThemeManager->currentCss();

    // 通过 JS 立即注入 CSS 到当前页面
    applyPreviewCss();

    // 触发预览刷新
    plainTextChanged();

    statusBar()->showMessage(tr("预览主题已切换"), 2000);
}

// 函数说明：启动 MainWindow 的异步任务、会话或后台流程。
void MainWindow::startPresentation()
{
    qDebug() << "[PresentationMode] startPresentation called";
    QString markdown = ui->plainTextEdit->toPlainText();

    if (markdown.trimmed().isEmpty()) {
        QMessageBox::information(this, tr("演示模式"),
            tr("文档内容为空，无法开始演示"));
        return;
    }

    qDebug() << "[PresentationMode] markdown length:" << markdown.length();
    presentationMode->setMarkdownContent(markdown);
    qDebug() << "[PresentationMode] slides count:" << presentationMode->slideCount();
    presentationMode->start();
    qDebug() << "[PresentationMode] started, isRunning:" << presentationMode->isRunning();
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onPresentationEnded()
{
    statusBar()->showMessage(tr("演示已结束"), 2000);
}

// 函数说明：显示 MainWindow 管理的面板、对话框或提示信息。
void MainWindow::showPrintPreview()
{
    QString html = generator->exportHtml(QString(), QString());

    printPreviewDialog->setHtmlContent(html);
    printPreviewDialog->setDocumentTitle(
        fileName.isEmpty() ? tr("未命名文档") : QFileInfo(fileName).baseName());

    printPreviewDialog->exec();
}

// 函数说明：切换 MainWindow 对应功能的启用状态。
void MainWindow::toggleTocFloating()
{
    if (tocFloatingWindow->isVisible()) {
        tocFloatingWindow->hide();
    } else {
        // 更新 TOC 内容
        tocFloatingWindow->updateFromMarkdown(ui->plainTextEdit->toPlainText());
        tocFloatingWindow->show();
    }
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onTocItemClicked(const QString &anchor, int lineNumber)
{
    Q_UNUSED(anchor)
    statusBar()->showMessage(tr("跳转到第 %1 行").arg(lineNumber), 2000);
}

// 函数说明：初始化 MainWindow 的 setupExtensionFeatures 相关界面、动作或服务连接。
void MainWindow::setupExtensionFeatures()
{
    // 创建插件管理器
    pluginManager = new PluginManager(this);
    pluginManager->setMainWindow(this);
    pluginManager->setEditor(ui->plainTextEdit);

    // 创建脚本引擎
    scriptEngine = new ScriptEngine(this);
    scriptEngine->setMainWindow(this);
    scriptEngine->setEditor(ui->plainTextEdit);

    // 创建快捷键管理器
    shortcutManager = new ShortcutManager(this);

    // 注册常用快捷键
    shortcutManager->registerAction("fileNew", ui->actionNew, ShortcutManager::CategoryFile);
    shortcutManager->registerAction("fileOpen", ui->actionOpen, ShortcutManager::CategoryFile);
    shortcutManager->registerAction("fileSave", ui->actionSave, ShortcutManager::CategoryFile);
    shortcutManager->registerAction("fileSaveAs", ui->actionSaveAs, ShortcutManager::CategoryFile);
    shortcutManager->registerAction("editUndo", ui->actionUndo, ShortcutManager::CategoryEdit);
    shortcutManager->registerAction("editRedo", ui->actionRedo, ShortcutManager::CategoryEdit);
    shortcutManager->registerAction("editCut", ui->actionCut, ShortcutManager::CategoryEdit);
    shortcutManager->registerAction("editCopy", ui->actionCopy, ShortcutManager::CategoryEdit);
    shortcutManager->registerAction("editPaste", ui->actionPaste, ShortcutManager::CategoryEdit);
    shortcutManager->registerAction("editFind", ui->actionFindReplace, ShortcutManager::CategoryEdit);
    shortcutManager->registerAction("viewFullScreen", ui->actionFullScreenMode, ShortcutManager::CategoryView);

    // 创建编辑器主题管理器
    editorThemeManager = new EditorThemeManager(this);
    editorThemeManager->setEditor(ui->plainTextEdit);

    // 创建扩展菜单
    extensionMenu = new QMenu(tr("扩展(&X)"), this);
    menuBar()->insertMenu(ui->menuHelp->menuAction(), extensionMenu);

    // 插件子菜单
    pluginMenu = new QMenu(tr("插件"), this);
    extensionMenu->addMenu(pluginMenu);

    actionPluginManager = new QAction(tr("插件管理器..."), this);
    connect(actionPluginManager, &QAction::triggered, this, &MainWindow::extShowPluginManager);
    pluginMenu->addAction(actionPluginManager);

    pluginMenu->addSeparator();

    // 脚本子菜单
    scriptMenu = new QMenu(tr("脚本"), this);
    extensionMenu->addMenu(scriptMenu);

    actionScriptManager = new QAction(tr("脚本管理器..."), this);
    connect(actionScriptManager, &QAction::triggered, this, &MainWindow::extShowScriptManager);
    scriptMenu->addAction(actionScriptManager);

    scriptMenu->addSeparator();

    // 添加内置脚本
    QAction *actionToUpper = new QAction(tr("转换为大写"), this);
    actionToUpper->setShortcut(QKeySequence("Ctrl+Shift+U"));
    connect(actionToUpper, &QAction::triggered, this, [this]() {
        scriptEngine->runScriptById("toUpperCase");
    });
    scriptMenu->addAction(actionToUpper);

    QAction *actionToLower = new QAction(tr("转换为小写"), this);
    actionToLower->setShortcut(QKeySequence("Ctrl+Shift+L"));
    connect(actionToLower, &QAction::triggered, this, [this]() {
        scriptEngine->runScriptById("toLowerCase");
    });
    scriptMenu->addAction(actionToLower);

    QAction *actionSortLines = new QAction(tr("行排序"), this);
    connect(actionSortLines, &QAction::triggered, this, [this]() {
        scriptEngine->runScriptById("sortLines");
    });
    scriptMenu->addAction(actionSortLines);

    QAction *actionRemoveDuplicates = new QAction(tr("去除重复行"), this);
    connect(actionRemoveDuplicates, &QAction::triggered, this, [this]() {
        scriptEngine->runScriptById("removeDuplicateLines");
    });
    scriptMenu->addAction(actionRemoveDuplicates);

    QAction *actionInsertDateTime = new QAction(tr("插入日期时间"), this);
    actionInsertDateTime->setShortcut(QKeySequence("Ctrl+Shift+D"));
    connect(actionInsertDateTime, &QAction::triggered, this, [this]() {
        scriptEngine->runScriptById("insertDateTime");
    });
    scriptMenu->addAction(actionInsertDateTime);

    QAction *actionWordCount = new QAction(tr("字数统计"), this);
    connect(actionWordCount, &QAction::triggered, this, [this]() {
        scriptEngine->runScriptById("wordCount");
    });
    scriptMenu->addAction(actionWordCount);

    extensionMenu->addSeparator();

    // 编辑器主题子菜单
    editorThemeMenu = new QMenu(tr("编辑器主题"), this);
    extensionMenu->addMenu(editorThemeMenu);

    for (const QString &id : editorThemeManager->availableThemes()) {
        EditorTheme theme = editorThemeManager->theme(id);
        QAction *action = new QAction(theme.name, this);
        action->setData(id);
        action->setCheckable(true);
        action->setChecked(id == editorThemeManager->currentThemeId());
        connect(action, &QAction::triggered, this, [this, id]() {
            editorThemeManager->applyTheme(id);
        });
        editorThemeMenu->addAction(action);
    }

    editorThemeMenu->addSeparator();

    actionEditorThemeEditor = new QAction(tr("主题编辑器..."), this);
    connect(actionEditorThemeEditor, &QAction::triggered, this, &MainWindow::extShowEditorThemeEditor);
    editorThemeMenu->addAction(actionEditorThemeEditor);

    extensionMenu->addSeparator();

    // 快捷键设置
    actionShortcutEditor = new QAction(tr("快捷键设置..."), this);
    connect(actionShortcutEditor, &QAction::triggered, this, &MainWindow::extShowShortcutEditor);
    extensionMenu->addAction(actionShortcutEditor);

    // 连接信号
    connect(pluginManager, &PluginManager::pluginLoaded,
            this, &MainWindow::onPluginLoaded);
    connect(pluginManager, &PluginManager::pluginError,
            this, &MainWindow::onPluginError);

    connect(pluginManager, &PluginManager::requestOpenFile,
            this, [this](const QString &path) { load(path); });
    connect(pluginManager, &PluginManager::requestSaveFile,
            this, &MainWindow::fileSave);
    connect(pluginManager, &PluginManager::requestShowStatusMessage,
            this, [this](const QString &msg, int timeout) {
                statusBar()->showMessage(msg, timeout);
            });

    connect(scriptEngine, &ScriptEngine::scriptOutput,
            this, &MainWindow::onScriptOutput);
    connect(scriptEngine, &ScriptEngine::requestOpenFile,
            this, [this](const QString &path) { load(path); });
    connect(scriptEngine, &ScriptEngine::requestSaveFile,
            this, &MainWindow::fileSave);
    connect(scriptEngine, &ScriptEngine::requestShowMessage,
            this, [this](const QString &msg, int timeout) {
                statusBar()->showMessage(msg, timeout);
            });

    connect(editorThemeManager, &EditorThemeManager::themeChanged,
            this, &MainWindow::extApplyEditorTheme);

    // 发现和加载插件
    pluginManager->discoverPlugins();
    pluginManager->loadAllPlugins();

    // 发现脚本
    scriptEngine->discoverScripts();
}

// 函数说明：实现 MainWindow::extShowPluginManager 的核心逻辑，供当前模块调用。
void MainWindow::extShowPluginManager()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("插件管理器"));
    dialog.resize(800, 550);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QTableWidget *table = new QTableWidget(&dialog);
    table->setColumnCount(5);
    table->setHorizontalHeaderLabels({tr("名称"), tr("版本"), tr("作者"), tr("状态"), tr("操作")});
    table->horizontalHeader()->setStretchLastSection(true);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QStringList plugins = pluginManager->availablePlugins();
    table->setRowCount(plugins.size());

    int row = 0;
    for (const QString &id : plugins) {
        PluginInfo info = pluginManager->pluginInfo(id);
        table->setItem(row, 0, new QTableWidgetItem(info.metadata.name));
        table->setItem(row, 1, new QTableWidgetItem(info.metadata.version));
        table->setItem(row, 2, new QTableWidgetItem(info.metadata.author));
        table->setItem(row, 3, new QTableWidgetItem(info.loaded ? tr("已加载") : tr("未加载")));

        QPushButton *toggleBtn = new QPushButton(info.enabled ? tr("禁用") : tr("启用"), &dialog);
        connect(toggleBtn, &QPushButton::clicked, this, [this, id, toggleBtn]() {
            bool enabled = pluginManager->isPluginEnabled(id);
            pluginManager->setPluginEnabled(id, !enabled);
            toggleBtn->setText(!enabled ? tr("禁用") : tr("启用"));
        });
        table->setCellWidget(row, 4, toggleBtn);

        row++;
    }

    layout->addWidget(table);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    QPushButton *refreshBtn = new QPushButton(tr("刷新"), &dialog);
    connect(refreshBtn, &QPushButton::clicked, this, [this]() {
        pluginManager->discoverPlugins();
    });
    btnLayout->addWidget(refreshBtn);

    QPushButton *closeBtn = new QPushButton(tr("关闭"), &dialog);
    connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    btnLayout->addWidget(closeBtn);

    layout->addLayout(btnLayout);

    dialog.exec();
}

// 函数说明：实现 MainWindow::extShowScriptManager 的核心逻辑，供当前模块调用。
void MainWindow::extShowScriptManager()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("脚本管理器"));
    dialog.resize(800, 550);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QListWidget *list = new QListWidget(&dialog);
    QStringList scripts = scriptEngine->availableScripts();

    for (const QString &id : scripts) {
        ScriptInfo info = scriptEngine->scriptInfo(id);
        QListWidgetItem *item = new QListWidgetItem(list);
        item->setText(QString("%1 (%2)").arg(info.name, info.version));
        item->setData(Qt::UserRole, id);
        item->setCheckState(info.enabled ? Qt::Checked : Qt::Unchecked);
    }

    connect(list, &QListWidget::itemChanged, this, [this](QListWidgetItem *item) {
        QString id = item->data(Qt::UserRole).toString();
        scriptEngine->setScriptEnabled(id, item->checkState() == Qt::Checked);
    });

    layout->addWidget(list);

    QHBoxLayout *btnLayout = new QHBoxLayout();

    QPushButton *runBtn = new QPushButton(tr("运行"), &dialog);
    connect(runBtn, &QPushButton::clicked, this, [this, list]() {
        QListWidgetItem *item = list->currentItem();
        if (item) {
            QString id = item->data(Qt::UserRole).toString();
            scriptEngine->runScriptById(id);
        }
    });
    btnLayout->addWidget(runBtn);

    QPushButton *openDirBtn = new QPushButton(tr("打开脚本目录"), &dialog);
    connect(openDirBtn, &QPushButton::clicked, this, [this]() {
        QStringList paths = scriptEngine->scriptPaths();
        if (!paths.isEmpty()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(paths.first()));
        }
    });
    btnLayout->addWidget(openDirBtn);

    btnLayout->addStretch();

    QPushButton *closeBtn = new QPushButton(tr("关闭"), &dialog);
    connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    btnLayout->addWidget(closeBtn);

    layout->addLayout(btnLayout);

    dialog.exec();
}

// 函数说明：实现 MainWindow::extRunScript 的核心逻辑，供当前模块调用。
void MainWindow::extRunScript(const QString &scriptId)
{
    scriptEngine->runScriptById(scriptId);
}

// 函数说明：实现 MainWindow::extShowShortcutEditor 的核心逻辑，供当前模块调用。
void MainWindow::extShowShortcutEditor()
{
    ShortcutEditor editor(shortcutManager, this);
    editor.exec();
}

// 函数说明：实现 MainWindow::extShowEditorThemeEditor 的核心逻辑，供当前模块调用。
void MainWindow::extShowEditorThemeEditor()
{
    EditorThemeEditor editor(editorThemeManager, this);
    editor.exec();
}

// 函数说明：实现 MainWindow::extApplyEditorTheme 的核心逻辑，供当前模块调用。
void MainWindow::extApplyEditorTheme(const QString &themeId)
{
    // 更新菜单中的选中状态
    for (QAction *action : editorThemeMenu->actions()) {
        action->setChecked(action->data().toString() == themeId);
    }

    statusBar()->showMessage(tr("编辑器主题已更改"), 2000);
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onPluginLoaded(const QString &pluginId)
{
    PluginInfo info = pluginManager->pluginInfo(pluginId);
    statusBar()->showMessage(tr("插件已加载: %1").arg(info.metadata.name), 3000);
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onPluginError(const QString &pluginId, const QString &error)
{
    QMessageBox::warning(this, tr("插件错误"),
        tr("插件 %1 发生错误:\n%2").arg(pluginId, error));
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onScriptOutput(const QString &message)
{
    statusBar()->showMessage(message, 3000);
}

// ============================================================================
// AI 智能写作助手功能实现
// ============================================================================

// 可拖拽标题栏：拖出主窗口即可脱离为独立窗口
class AIPanelTitleBar : public QFrame
{
public:
    std::function<void(const QPoint &)> detachCallback;

    explicit AIPanelTitleBar(QWidget *panel, QWidget *mainWin)
        : QFrame(panel), m_panel(panel), m_mainWin(mainWin), m_dragging(false)
    {
        setObjectName("aiPanelTitleBar");
        setFixedHeight(34);
        setCursor(Qt::OpenHandCursor);
        setStyleSheet(
            "AIPanelTitleBar { background-color: #e9ecef; border-bottom: 1px solid #dee2e6; }"
        );

        QHBoxLayout *lay = new QHBoxLayout(this);
        lay->setContentsMargins(8, 2, 4, 2);
        lay->setSpacing(4);

        QLabel *lbl = new QLabel(tr("AI 写作助手"), this);
        lbl->setStyleSheet("font-weight: bold; font-size: 11px; color: #495057;");
        lay->addWidget(lbl);
        lay->addStretch();

        QPushButton *detachBtn = new QPushButton(tr("弹出 ⤢"), this);
        detachBtn->setObjectName("aiDetachBtn");
        detachBtn->setToolTip(tr("弹出为独立窗口"));
        detachBtn->setStyleSheet(
            "QPushButton { padding: 2px 10px; font-size: 12px; border: 1px solid #adb5bd; "
            "border-radius: 3px; background-color: #f8f9fa; color: #495057; }"
            "QPushButton:hover { background-color: #e2e6ea; border-color: #8c949c; }"
        );
        lay->addWidget(detachBtn);
        connect(detachBtn, &QPushButton::clicked, this, [this]() {
            if (detachCallback) {
                QPoint pos = m_panel->mapToGlobal(QPoint(0, 0));
                detachCallback(pos);
            }
        });
    }

protected:
    void mousePressEvent(QMouseEvent *e) override {
        if (e->button() == Qt::LeftButton) {
            m_dragging = true;
            setCursor(Qt::ClosedHandCursor);
        }
        QFrame::mousePressEvent(e);
    }

    void mouseMoveEvent(QMouseEvent *e) override {
        if (m_dragging && m_panel->parentWidget() != nullptr && m_mainWin) {
            QPoint gp = e->globalPosition().toPoint();
            if (!m_mainWin->geometry().contains(gp)) {
                m_dragging = false;
                setCursor(Qt::OpenHandCursor);
                if (detachCallback) {
                    detachCallback(gp);
                }
            }
        }
        QFrame::mouseMoveEvent(e);
    }

    void mouseReleaseEvent(QMouseEvent *e) override {
        m_dragging = false;
        setCursor(Qt::OpenHandCursor);
        QFrame::mouseReleaseEvent(e);
    }

private:
    QWidget *m_panel;
    QWidget *m_mainWin;
    bool m_dragging;
};

// 函数说明：初始化 MainWindow 的 setupAIWritingAssistant 相关界面、动作或服务连接。
void MainWindow::setupAIWritingAssistant()
{
    // 创建 AI 写作助手
    aiWritingAssistant = new AIWritingAssistant(this);
    aiWritingAssistant->setEditor(ui->plainTextEdit);

    // 创建 AI 写作面板
    aiWritingPanel = new AIWritingPanel(this);
    aiWritingPanel->setAssistant(aiWritingAssistant);
    aiWritingPanel->setMinimumHeight(150);
    aiWritingPanel->hide();

    // 在面板顶部插入可拖拽标题栏（拖出主窗口可脱离为独立窗口）
    AIPanelTitleBar *titleBar = new AIPanelTitleBar(aiWritingPanel, this);
    QVBoxLayout *panelLayout = qobject_cast<QVBoxLayout*>(aiWritingPanel->layout());
    if (panelLayout) {
        panelLayout->setContentsMargins(0, 0, 0, 8);
        panelLayout->insertWidget(0, titleBar);
    }

    // 将 AI 面板嵌入右下角：在预览区（stackedWidget）下方
    QSplitter *rightSplitter = new QSplitter(Qt::Vertical);
    rightSplitter->setObjectName("aiRightSplitter");

    int stackedIndex = ui->splitter->indexOf(ui->stackedWidget);
    ui->splitter->replaceWidget(stackedIndex, rightSplitter);

    rightSplitter->addWidget(ui->stackedWidget);
    rightSplitter->addWidget(aiWritingPanel);
    ui->stackedWidget->show();
    rightSplitter->setStretchFactor(0, 1);
    rightSplitter->setStretchFactor(1, 0);

    aiWritingDock = nullptr;

    // ---- 脱离/嵌入的统一逻辑 ----
    // 脱离：将面板从 splitter 中取出，变为独立窗口
    auto detachPanel = [this](const QPoint &globalPos) {
        if (!aiWritingPanel || aiWritingPanel->parentWidget() == nullptr) return;

        QSize sz = aiWritingPanel->size();
        if (sz.width() < 300) sz.setWidth(380);
        if (sz.height() < 200) sz.setHeight(520);

        aiWritingPanel->setParent(nullptr);
        aiWritingPanel->setWindowFlags(Qt::Window);
        aiWritingPanel->setWindowTitle(tr("AI 写作助手"));
        aiWritingPanel->resize(sz);
        aiWritingPanel->move(globalPos.x() - sz.width() / 2, globalPos.y() - 14);
        aiWritingPanel->show();

        // 隐藏嵌入式标题栏（独立窗口自带标题栏）
        QWidget *tb = aiWritingPanel->findChild<QWidget*>("aiPanelTitleBar");
        if (tb) tb->hide();

        // 更新菜单文字
        QAction *act = findChild<QAction*>("actionAIDetach");
        if (act) act->setText(tr("嵌入到主窗口"));

        // 启动原生窗口拖拽，让用户能继续移动窗口
        if (aiWritingPanel->windowHandle()) {
            aiWritingPanel->windowHandle()->startSystemMove();
        }
    };

    // 嵌入：将独立窗口放回 splitter
    auto embedPanel = [this]() {
        if (!aiWritingPanel || aiWritingPanel->parentWidget() != nullptr) return;

        aiWritingPanel->hide();
        aiWritingPanel->setWindowFlags(Qt::Widget);
        QSplitter *sp = findChild<QSplitter*>("aiRightSplitter");
        if (sp) {
            sp->addWidget(aiWritingPanel);
            aiWritingPanel->show();
        }

        // 显示嵌入式标题栏
        QWidget *tb = aiWritingPanel->findChild<QWidget*>("aiPanelTitleBar");
        if (tb) tb->show();

        QAction *act = findChild<QAction*>("actionAIDetach");
        if (act) act->setText(tr("弹出为独立窗口"));
    };

    // 标题栏拖拽脱离回调
    titleBar->detachCallback = detachPanel;

    // 连接信号
    connect(aiWritingPanel, &AIWritingPanel::insertTextRequested,
            this, &MainWindow::onAIInsertText);
    connect(aiWritingPanel, &AIWritingPanel::applyTitleRequested,
            this, &MainWindow::onAIApplyTitle);

    // 创建 AI 菜单
    aiMenu = new QMenu(tr("AI 助手(&A)"), this);
    menuBar()->insertMenu(ui->menuHelp->menuAction(), aiMenu);

    actionAICompletion = new QAction(tr("AI 智能补全"), this);
    actionAICompletion->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_J));
    actionAICompletion->setToolTip(tr("使用 AI 智能续写当前内容"));
    connect(actionAICompletion, &QAction::triggered, this, &MainWindow::aiCompletion);
    aiMenu->addAction(actionAICompletion);

    actionAIGrammarCheck = new QAction(tr("语法纠错"), this);
    actionAIGrammarCheck->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_G));
    actionAIGrammarCheck->setToolTip(tr("AI 驱动的语法和风格检查"));
    connect(actionAIGrammarCheck, &QAction::triggered, this, &MainWindow::aiGrammarCheck);
    aiMenu->addAction(actionAIGrammarCheck);

    actionAISummary = new QAction(tr("生成摘要"), this);
    actionAISummary->setToolTip(tr("为文档自动生成摘要"));
    connect(actionAISummary, &QAction::triggered, this, &MainWindow::aiSummary);
    aiMenu->addAction(actionAISummary);

    actionAITitle = new QAction(tr("生成标题建议"), this);
    actionAITitle->setToolTip(tr("基于内容生成标题建议"));
    connect(actionAITitle, &QAction::triggered, this, &MainWindow::aiTitleSuggestion);
    aiMenu->addAction(actionAITitle);

    aiMenu->addSeparator();

    actionAIShowPanel = new QAction(tr("显示 AI 助手面板"), this);
    actionAIShowPanel->setCheckable(true);
    connect(actionAIShowPanel, &QAction::triggered, this, &MainWindow::aiShowPanel);
    aiMenu->addAction(actionAIShowPanel);

    // 弹出/嵌入菜单项
    QAction *actionAIDetach = new QAction(tr("弹出为独立窗口"), this);
    actionAIDetach->setObjectName("actionAIDetach");
    connect(actionAIDetach, &QAction::triggered, this, [this, detachPanel, embedPanel]() {
        if (!aiWritingPanel) return;
        if (aiWritingPanel->parentWidget() != nullptr) {
            QPoint pos = aiWritingPanel->mapToGlobal(QPoint(0, 0));
            detachPanel(pos);
        } else {
            embedPanel();
        }
    });
    aiMenu->addAction(actionAIDetach);

    aiMenu->addSeparator();

    actionAISettings = new QAction(tr("AI 设置..."), this);
    connect(actionAISettings, &QAction::triggered, this, &MainWindow::aiShowSettings);
    aiMenu->addAction(actionAISettings);
}

// 函数说明：实现 MainWindow::aiCompletion 的核心逻辑，供当前模块调用。
void MainWindow::aiCompletion()
{
    if (!aiWritingPanel || !aiWritingAssistant) {
        QMessageBox::warning(this, tr("AI 助手"),
                             tr("AI 写作助手尚未完成初始化，请重启应用后重试。"));
        return;
    }

    aiWritingPanel->show();
    if (aiWritingPanel->parentWidget() == nullptr) {
        aiWritingPanel->raise();
        aiWritingPanel->activateWindow();
    }
    aiWritingAssistant->requestCompletion(false, true);
}

// 函数说明：实现 MainWindow::aiGrammarCheck 的核心逻辑，供当前模块调用。
void MainWindow::aiGrammarCheck()
{
    if (!aiWritingPanel || !aiWritingAssistant) {
        QMessageBox::warning(this, tr("AI 助手"),
                             tr("AI 写作助手尚未完成初始化，请重启应用后重试。"));
        return;
    }

    aiWritingPanel->show();
    if (aiWritingPanel->parentWidget() == nullptr) {
        aiWritingPanel->raise();
        aiWritingPanel->activateWindow();
    }
    aiWritingAssistant->requestGrammarCheck(false);
}

// 函数说明：实现 MainWindow::aiSummary 的核心逻辑，供当前模块调用。
void MainWindow::aiSummary()
{
    if (!aiWritingPanel || !aiWritingAssistant) {
        QMessageBox::warning(this, tr("AI 助手"),
                             tr("AI 写作助手尚未完成初始化，请重启应用后重试。"));
        return;
    }

    aiWritingPanel->show();
    if (aiWritingPanel->parentWidget() == nullptr) {
        aiWritingPanel->raise();
        aiWritingPanel->activateWindow();
    }
    aiWritingAssistant->requestSummary();
}

// 函数说明：实现 MainWindow::aiTitleSuggestion 的核心逻辑，供当前模块调用。
void MainWindow::aiTitleSuggestion()
{
    if (!aiWritingPanel || !aiWritingAssistant) {
        QMessageBox::warning(this, tr("AI 助手"),
                             tr("AI 写作助手尚未完成初始化，请重启应用后重试。"));
        return;
    }

    aiWritingPanel->show();
    if (aiWritingPanel->parentWidget() == nullptr) {
        aiWritingPanel->raise();
        aiWritingPanel->activateWindow();
    }
    aiWritingAssistant->requestTitleSuggestions();
}

// 函数说明：实现 MainWindow::aiShowPanel 的核心逻辑，供当前模块调用。
void MainWindow::aiShowPanel(bool checked)
{
    if (!aiWritingPanel) return;

    if (checked) {
        // 如果面板已弹出为独立窗口且不可见，重新嵌入
        if (aiWritingPanel->parentWidget() == nullptr && !aiWritingPanel->isVisible()) {
            aiWritingPanel->setWindowFlags(Qt::Widget);
            QSplitter *sp = findChild<QSplitter*>("aiRightSplitter");
            if (sp) {
                sp->addWidget(aiWritingPanel);
            }
            // 显示嵌入式标题栏
            QWidget *tb = aiWritingPanel->findChild<QWidget*>("aiPanelTitleBar");
            if (tb) tb->show();
            // 重置弹出按钮文字
            QAction *detachAction = findChild<QAction*>("actionAIDetach");
            if (detachAction) {
                detachAction->setText(tr("弹出为独立窗口"));
            }
        }
        aiWritingPanel->show();
        if (aiWritingPanel->parentWidget() == nullptr) {
            aiWritingPanel->raise();
            aiWritingPanel->activateWindow();
        }
    } else {
        aiWritingPanel->hide();
    }
}

// 函数说明：实现 MainWindow::aiShowSettings 的核心逻辑，供当前模块调用。
void MainWindow::aiShowSettings()
{
    if (!aiWritingPanel) {
        QMessageBox::warning(this, tr("AI 助手"),
                             tr("AI 写作助手面板未初始化，请重启应用后重试。"));
        return;
    }

    aiWritingPanel->show();
    if (aiWritingPanel->parentWidget() == nullptr) {
        aiWritingPanel->raise();
        aiWritingPanel->activateWindow();
    }
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onAIInsertText(const QString &text)
{
    if (text.isEmpty()) return;

    QTextCursor cursor = ui->plainTextEdit->textCursor();
    cursor.insertText(text);
    ui->plainTextEdit->setTextCursor(cursor);

    statusBar()->showMessage(tr("AI 内容已插入"), 2000);
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onAIApplyTitle(const QString &title)
{
    if (title.isEmpty()) return;
    if (!aiWritingAssistant) {
        QMessageBox::warning(this, tr("AI 助手"),
                             tr("AI 写作助手未初始化，无法应用标题。"));
        return;
    }

    aiWritingAssistant->applySelectedTitle(title);
    statusBar()->showMessage(tr("标题已应用: %1").arg(title), 2000);
}

// ============================================================================
// 高级搜索与索引功能实现
// ============================================================================

void MainWindow::setupAdvancedSearch()
{
    // 创建搜索索引管理器
    searchIndexManager = new SearchIndexManager(this);

    // 创建高级搜索面板
    advancedSearchPanel = new AdvancedSearchPanel(this);
    advancedSearchPanel->setSearchManager(searchIndexManager);

    // 创建停靠窗口
    searchDock = new QDockWidget(tr("高级搜索"), this);
    searchDock->setWidget(advancedSearchPanel);
    searchDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    searchDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    searchDock->setFloating(true);
    searchDock->resize(500, 450);
    searchDock->hide();

    // 连接信号
    connect(advancedSearchPanel, &AdvancedSearchPanel::resultSelected,
            this, &MainWindow::onSearchResultActivated);

    // 创建搜索菜单
    searchMenu = new QMenu(tr("搜索(&S)"), this);
    menuBar()->insertMenu(ui->menuHelp->menuAction(), searchMenu);

    // 搜索（打开/关闭搜索面板）
    actionSearchInFiles = new QAction(tr("搜索..."), this);
    actionSearchInFiles->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F));
    actionSearchInFiles->setToolTip(tr("打开搜索面板，搜索当前打开的文档"));
    connect(actionSearchInFiles, &QAction::triggered, this, &MainWindow::searchInFiles);
    searchMenu->addAction(actionSearchInFiles);

    searchMenu->addSeparator();

    // 重建索引
    actionSearchRebuildIndex = new QAction(tr("重建搜索索引"), this);
    actionSearchRebuildIndex->setToolTip(tr("重新构建全文搜索索引"));
    connect(actionSearchRebuildIndex, &QAction::triggered, this, &MainWindow::searchRebuildIndex);
    searchMenu->addAction(actionSearchRebuildIndex);

    // 搜索历史
    actionSearchHistory = new QAction(tr("搜索历史..."), this);
    actionSearchHistory->setToolTip(tr("查看搜索历史记录"));
    connect(actionSearchHistory, &QAction::triggered, this, &MainWindow::searchShowHistory);
    searchMenu->addAction(actionSearchHistory);

    // 清除搜索历史
    actionSearchClearHistory = new QAction(tr("清除搜索历史"), this);
    connect(actionSearchClearHistory, &QAction::triggered, this, &MainWindow::searchClearHistory);
    searchMenu->addAction(actionSearchClearHistory);

    // 如果当前有打开的文件，添加其所在目录到索引
    if (!fileName.isEmpty()) {
        QFileInfo fileInfo(fileName);
        QString directory = fileInfo.absolutePath();
        searchIndexManager->addSearchPath(directory);
    }
}

// 函数说明：实现 MainWindow::searchInFiles 的核心逻辑，供当前模块调用。
void MainWindow::searchInFiles()
{
    // 将所有标签页的内容加入搜索索引
    for (int i = 0; i < m_tabs.size(); ++i) {
        QString content;
        QString path;
        QString title;

        if (i == m_currentTabIndex) {
            content = ui->plainTextEdit->toPlainText();
            path = fileName.isEmpty() ? QString("untitled-%1").arg(i) : fileName;
        } else {
            content = m_tabs[i].content;
            path = m_tabs[i].filePath.isEmpty() ? QString("untitled-%1").arg(i) : m_tabs[i].filePath;
        }

        title = m_tabBar->tabText(i);
        searchIndexManager->indexContent(path, content, title);
    }

    if (searchDock->isVisible()) {
        searchDock->hide();
    } else {
        searchDock->show();
        advancedSearchPanel->focusSearchInput();
    }
}

// 函数说明：实现 MainWindow::searchRebuildIndex 的核心逻辑，供当前模块调用。
void MainWindow::searchRebuildIndex()
{
    searchIndexManager->rebuildIndex();
    statusBar()->showMessage(tr("正在重建搜索索引..."), 3000);
}

// 函数说明：实现 MainWindow::searchShowHistory 的核心逻辑，供当前模块调用。
void MainWindow::searchShowHistory()
{
    searchDock->show();
    advancedSearchPanel->showHistoryTab();
}

// 函数说明：实现 MainWindow::searchClearHistory 的核心逻辑，供当前模块调用。
void MainWindow::searchClearHistory()
{
    searchIndexManager->clearHistory();
    statusBar()->showMessage(tr("搜索历史已清除"), 2000);
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onSearchResultActivated(const QString &filePath, int lineNumber, int column)
{
    Q_UNUSED(column)

    // 打开文件
    if (load(filePath)) {
        // 跳转到指定行
        QTextCursor cursor = ui->plainTextEdit->textCursor();
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, lineNumber - 1);
        cursor.select(QTextCursor::LineUnderCursor);
        ui->plainTextEdit->setTextCursor(cursor);
        ui->plainTextEdit->centerCursor();
        ui->plainTextEdit->setFocus();

        statusBar()->showMessage(tr("已跳转到第 %1 行").arg(lineNumber), 2000);
    }
}

// ============================================================================
// 时间线/历史记录可视化功能实现
// ============================================================================

void MainWindow::setupTimelineView()
{
    // 创建时间线视图
    timelineView = new TimelineView(this);
    timelineView->setRevisionTracker(revisionTracker);

    // 创建迷你时间线视图（用于状态栏）
    timelineMiniView = new TimelineMiniView(this);
    timelineMiniView->setRevisionTracker(revisionTracker);
    timelineMiniView->hide(); // 未添加到任何 layout，须隐藏防止遮挡其他控件

    // 创建停靠窗口
    timelineDock = new QDockWidget(tr("版本历史时间线"), this);
    timelineDock->setWidget(timelineView);
    timelineDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, timelineDock);
    timelineDock->hide();

    // 连接信号
    connect(timelineView, &TimelineView::revisionRestoreRequested,
            this, &MainWindow::onTimelineRevisionRestore);
    connect(timelineView, &TimelineView::revisionSelected,
            this, &MainWindow::onTimelineRevisionSelected);

    connect(timelineMiniView, &TimelineMiniView::showFullTimeline,
            this, [this]() { timelineDock->show(); });
    connect(timelineMiniView, &TimelineMiniView::revisionClicked,
            this, &MainWindow::onTimelineRevisionSelected);

    // 创建时间线菜单
    timelineMenu = new QMenu(tr("时间线(&T)"), this);
    menuBar()->insertMenu(ui->menuHelp->menuAction(), timelineMenu);

    // 显示时间线面板
    actionTimelineShowPanel = timelineDock->toggleViewAction();
    actionTimelineShowPanel->setText(tr("显示时间线面板"));
    actionTimelineShowPanel->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T));
    timelineMenu->addAction(actionTimelineShowPanel);

    // 刷新时间线
    actionTimelineRefresh = new QAction(tr("刷新时间线"), this);
    actionTimelineRefresh->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_R));
    actionTimelineRefresh->setToolTip(tr("重新加载版本历史"));
    connect(actionTimelineRefresh, &QAction::triggered, this, &MainWindow::timelineRefresh);
    timelineMenu->addAction(actionTimelineRefresh);

    timelineMenu->addSeparator();

    // 对比模式
    actionTimelineCompareMode = new QAction(tr("版本对比模式"), this);
    actionTimelineCompareMode->setCheckable(true);
    actionTimelineCompareMode->setToolTip(tr("启用版本对比模式"));
    connect(actionTimelineCompareMode, &QAction::triggered, this, &MainWindow::timelineCompareMode);
    timelineMenu->addAction(actionTimelineCompareMode);

    // 如果有打开的文件，更新时间线
    if (!fileName.isEmpty()) {
        timelineView->setDocumentPath(fileName);
        timelineMiniView->setDocumentPath(fileName);
    }
}

// 函数说明：实现 MainWindow::timelineShowPanel 的核心逻辑，供当前模块调用。
void MainWindow::timelineShowPanel(bool checked)
{
    timelineDock->setVisible(checked);
}

// 函数说明：实现 MainWindow::timelineRefresh 的核心逻辑，供当前模块调用。
void MainWindow::timelineRefresh()
{
    if (!fileName.isEmpty()) {
        timelineView->setDocumentPath(fileName);
        timelineView->setCurrentContent(ui->plainTextEdit->toPlainText());
        timelineView->refresh();

        timelineMiniView->setDocumentPath(fileName);
        timelineMiniView->refresh();
    }
    statusBar()->showMessage(tr("时间线已刷新"), 2000);
}

// 函数说明：实现 MainWindow::timelineCompareMode 的核心逻辑，供当前模块调用。
void MainWindow::timelineCompareMode()
{
    if (actionTimelineCompareMode->isChecked()) {
        timelineView->enterCompareMode();
    } else {
        timelineView->exitCompareMode();
    }
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onTimelineRevisionRestore(const QString &revisionId)
{
    if (!revisionTracker) return;

    QString content = revisionTracker->restoreRevision(revisionId);
    if (!content.isEmpty()) {
        ui->plainTextEdit->setPlainText(content);
        statusBar()->showMessage(tr("已恢复到修订版本: %1").arg(revisionId.left(8)), 3000);
    }
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onTimelineRevisionSelected(const QString &revisionId)
{
    Q_UNUSED(revisionId)
    // 可以在这里更新状态栏或其他 UI 元素
    statusBar()->showMessage(tr("已选择修订版本: %1").arg(revisionId.left(8)), 2000);
}

// ============================================================================
// 大文件处理优化功能实现
// ============================================================================

void MainWindow::setupLargeFileManager()
{
    // 创建大文件管理器
    largeFileManager = new LargeFileManager(this);

    // 配置大文件管理器
    LargeFileManager::Config config;
    config.largeFileThreshold = 1024 * 1024;      // 1MB 启用大文件模式
    config.veryLargeFileThreshold = 10 * 1024 * 1024;  // 10MB 启用分块加载
    config.chunkSize = 500;                        // 每块 500 行
    config.preloadChunks = 3;                      // 预加载 3 块
    config.highlightDelay = 100;                   // 100ms 高亮延迟
    config.enableVirtualization = true;
    config.maxMemoryUsage = 256;                   // 最大 256MB 内存
    largeFileManager->setConfig(config);

    // 创建虚拟滚动辅助
    virtualScrollHelper = new VirtualScrollHelper(ui->plainTextEdit, this);
    virtualScrollHelper->setLargeFileManager(largeFileManager);

    // 连接大文件管理器信号
    connect(largeFileManager, &LargeFileManager::loadingStarted,
            this, &MainWindow::onLargeFileLoadingStarted);
    connect(largeFileManager, &LargeFileManager::loadingProgress,
            this, &MainWindow::onLargeFileLoadingProgress);
    connect(largeFileManager, &LargeFileManager::loadingFinished,
            this, &MainWindow::onLargeFileLoadingFinished);
    connect(largeFileManager, &LargeFileManager::memoryWarning,
            this, &MainWindow::onLargeFileMemoryWarning);

    // 创建状态标签用于显示大文件加载状态
    largeFileStatusLabel = new QLabel(this);
    largeFileStatusLabel->setVisible(false);
    statusBar()->addPermanentWidget(largeFileStatusLabel);
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onLargeFileLoadingStarted(const QString &filePath)
{
    largeFileStatusLabel->setText(tr("正在加载大文件: %1").arg(QFileInfo(filePath).fileName()));
    largeFileStatusLabel->setVisible(true);

    // 禁用一些操作以提高性能
    ui->plainTextEdit->setUpdatesEnabled(false);

    statusBar()->showMessage(tr("正在加载大文件..."), 0);
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onLargeFileLoadingProgress(int percent, const QString &message)
{
    largeFileStatusLabel->setText(message);
    statusBar()->showMessage(tr("加载进度: %1%").arg(percent), 0);
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onLargeFileLoadingFinished(bool success)
{
    largeFileStatusLabel->setVisible(false);
    ui->plainTextEdit->setUpdatesEnabled(true);

    if (success) {
        // 显示文件大小级别信息
        QString levelStr;
        switch (largeFileManager->fileSizeLevel()) {
        case LargeFileManager::Normal:
            levelStr = tr("普通");
            break;
        case LargeFileManager::Large:
            levelStr = tr("大文件");
            break;
        case LargeFileManager::VeryLarge:
            levelStr = tr("超大文件");
            break;
        }

        qint64 fileSize = largeFileManager->fileSize();
        QString sizeStr;
        if (fileSize >= 1024 * 1024) {
            sizeStr = QString::number(fileSize / (1024.0 * 1024.0), 'f', 2) + " MB";
        } else if (fileSize >= 1024) {
            sizeStr = QString::number(fileSize / 1024.0, 'f', 2) + " KB";
        } else {
            sizeStr = QString::number(fileSize) + " B";
        }

        statusBar()->showMessage(tr("文件加载完成 [%1, %2, %3 行]")
                                 .arg(levelStr)
                                 .arg(sizeStr)
                                 .arg(largeFileManager->totalLines()), 5000);
    } else {
        statusBar()->showMessage(tr("文件加载失败"), 3000);
    }
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onLargeFileMemoryWarning(qint64 currentUsage, qint64 maxUsage)
{
    Q_UNUSED(currentUsage)
    Q_UNUSED(maxUsage)

    QString currentStr = QString::number(currentUsage / (1024.0 * 1024.0), 'f', 1) + " MB";
    QString maxStr = QString::number(maxUsage / (1024.0 * 1024.0), 'f', 1) + " MB";

    QMessageBox::warning(this, tr("内存警告"),
                         tr("当前内存使用量 (%1) 已接近限制 (%2)。\n\n"
                            "建议保存文件并重新打开，或关闭其他应用程序以释放内存。")
                         .arg(currentStr).arg(maxStr));
}

// ============================================================================
// 内存优化功能实现
// ============================================================================

void MainWindow::setupMemoryOptimizer()
{
    // 创建内存优化器
    memoryOptimizer = new MemoryOptimizer(this);

    // 配置内存优化器
    MemoryOptimizer::Config config;
    config.warningThreshold = 500 * 1024 * 1024;    // 500MB 警告
    config.criticalThreshold = 800 * 1024 * 1024;   // 800MB 临界
    config.monitorInterval = 5000;                   // 5秒监控一次
    config.enableAutoOptimize = true;
    memoryOptimizer->setConfig(config);

    // 配置图片缓存管理器
    ImageCacheManager::Config imageConfig;
    imageConfig.maxCachedImages = 50;
    imageConfig.maxCacheMemory = 100 * 1024 * 1024;  // 100MB
    imageConfig.preloadDistance = 500;
    imageConfig.enableLazyLoad = true;
    imageConfig.loadDelay = 100;
    memoryOptimizer->imageCacheManager()->setConfig(imageConfig);

    // 配置文档内存管理器
    DocumentMemoryManager::Config docConfig;
    docConfig.maxLoadedDocuments = 5;
    docConfig.maxMemoryUsage = 200 * 1024 * 1024;   // 200MB
    docConfig.unloadTimeout = 300;                   // 5分钟不活跃后卸载
    docConfig.enableAutoUnload = true;
    memoryOptimizer->documentMemoryManager()->setConfig(docConfig);

    // 连接信号
    connect(memoryOptimizer, &MemoryOptimizer::memoryWarning,
            this, [this](const MemoryOptimizer::MemoryReport &report) {
        onMemoryWarning(report.totalUsage, memoryOptimizer->config().warningThreshold);
    });
    connect(memoryOptimizer, &MemoryOptimizer::memoryCritical,
            this, [this](const MemoryOptimizer::MemoryReport &report) {
        onMemoryCritical(report.totalUsage, memoryOptimizer->config().criticalThreshold);
    });
    connect(memoryOptimizer, &MemoryOptimizer::optimizationCompleted,
            this, &MainWindow::onOptimizationCompleted);

    // 创建状态标签
    memoryStatusLabel = new QLabel(this);
    memoryStatusLabel->setToolTip(tr("内存使用状态"));
    statusBar()->addPermanentWidget(memoryStatusLabel);
    updateMemoryStatusLabel();

    // 添加菜单项（添加到工具菜单）
    if (toolsMenu) {
        toolsMenu->addSeparator();

        actionMemoryStatus = new QAction(tr("内存状态..."), this);
        actionMemoryStatus->setToolTip(tr("显示详细内存使用情况"));
        connect(actionMemoryStatus, &QAction::triggered, this, &MainWindow::showMemoryStatus);
        toolsMenu->addAction(actionMemoryStatus);

        actionMemoryOptimize = new QAction(tr("立即优化内存"), this);
        actionMemoryOptimize->setToolTip(tr("清理缓存并释放内存"));
        connect(actionMemoryOptimize, &QAction::triggered, this, &MainWindow::optimizeMemoryNow);
        toolsMenu->addAction(actionMemoryOptimize);
    }

    // 启动内存监控
    memoryOptimizer->startMonitoring();

    // 定时更新状态标签
    QTimer *updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &MainWindow::updateMemoryStatusLabel);
    updateTimer->start(10000);  // 每10秒更新一次
}

// 函数说明：刷新 MainWindow 的内部状态，并同步到相关界面。
void MainWindow::updateMemoryStatusLabel()
{
    if (!memoryOptimizer || !memoryStatusLabel) return;

    qint64 usage = memoryOptimizer->currentMemoryUsage();
    QString usageStr;

    if (usage >= 1024 * 1024 * 1024) {
        usageStr = QString::number(usage / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
    } else {
        usageStr = QString::number(usage / (1024.0 * 1024.0), 'f', 1) + " MB";
    }

    QString statusText = tr("内存: %1").arg(usageStr);

    if (memoryOptimizer->isMemoryCritical()) {
        memoryStatusLabel->setStyleSheet("QLabel { color: red; font-weight: bold; }");
        statusText += " ⚠";
    } else if (memoryOptimizer->isMemoryWarning()) {
        memoryStatusLabel->setStyleSheet("QLabel { color: orange; }");
        statusText += " !";
    } else {
        memoryStatusLabel->setStyleSheet("");
    }

    memoryStatusLabel->setText(statusText);
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onMemoryWarning(qint64 currentUsage, qint64 maxUsage)
{
    Q_UNUSED(maxUsage)

    QString usageStr = QString::number(currentUsage / (1024.0 * 1024.0), 'f', 1) + " MB";
    statusBar()->showMessage(tr("内存使用较高: %1 - 建议关闭不需要的标签页").arg(usageStr), 5000);

    updateMemoryStatusLabel();
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onMemoryCritical(qint64 currentUsage, qint64 maxUsage)
{
    QString currentStr = QString::number(currentUsage / (1024.0 * 1024.0), 'f', 1) + " MB";
    QString maxStr = QString::number(maxUsage / (1024.0 * 1024.0), 'f', 1) + " MB";

    QMessageBox::warning(this, tr("内存警告"),
                         tr("内存使用量 (%1) 已达到临界值 (%2)！\n\n"
                            "系统将自动优化内存。建议：\n"
                            "• 保存当前工作\n"
                            "• 关闭不需要的标签页\n"
                            "• 重启应用程序")
                         .arg(currentStr).arg(maxStr));

    updateMemoryStatusLabel();
}

// 函数说明：响应 MainWindow 收到的信号或异步回调，并更新界面状态。
void MainWindow::onOptimizationCompleted(qint64 freedMemory)
{
    QString freedStr;
    if (freedMemory >= 1024 * 1024) {
        freedStr = QString::number(freedMemory / (1024.0 * 1024.0), 'f', 1) + " MB";
    } else {
        freedStr = QString::number(freedMemory / 1024.0, 'f', 1) + " KB";
    }

    statusBar()->showMessage(tr("内存优化完成，释放了 %1").arg(freedStr), 3000);
    updateMemoryStatusLabel();
}

// 函数说明：显示 MainWindow 管理的面板、对话框或提示信息。
void MainWindow::showMemoryStatus()
{
    if (!memoryOptimizer) return;

    MemoryOptimizer::MemoryReport report = memoryOptimizer->generateReport();

    auto formatSize = [](qint64 bytes) -> QString {
        if (bytes >= 1024 * 1024 * 1024) {
            return QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
        } else if (bytes >= 1024 * 1024) {
            return QString::number(bytes / (1024.0 * 1024.0), 'f', 2) + " MB";
        } else if (bytes >= 1024) {
            return QString::number(bytes / 1024.0, 'f', 2) + " KB";
        }
        return QString::number(bytes) + " B";
    };

    QString details = tr(
        "内存使用详情\n"
        "================\n\n"
        "总内存使用: %1\n"
        "图片缓存: %2 (%3 张图片)\n"
        "文档内存: %4 (%5 个已加载文档)\n"
        "其他: %6\n\n"
        "缓存命中率: %7%\n"
        "报告时间: %8"
    ).arg(formatSize(report.totalUsage))
     .arg(formatSize(report.imageCacheUsage))
     .arg(report.cachedImages)
     .arg(formatSize(report.documentUsage))
     .arg(report.loadedDocuments)
     .arg(formatSize(report.otherUsage))
     .arg(report.imageCacheHitRate * 100, 0, 'f', 1)
     .arg(report.timestamp.toString("yyyy-MM-dd hh:mm:ss"));

    QMessageBox::information(this, tr("内存状态"), details);
}

// 函数说明：实现 MainWindow::optimizeMemoryNow 的核心逻辑，供当前模块调用。
void MainWindow::optimizeMemoryNow()
{
    if (!memoryOptimizer) return;

    statusBar()->showMessage(tr("正在优化内存..."), 0);
    QApplication::processEvents();

    memoryOptimizer->optimizeNow();
}

// 函数说明：初始化 MainWindow 的 setupDockLayout 相关界面、动作或服务连接。
void MainWindow::setupDockLayout()
{
    // 将所有非核心 dock 设为浮动并隐藏，避免 dock 标签页重叠
    QList<QDockWidget*> floatingDocks;
    if (outlineDock) floatingDocks << outlineDock;
    if (dataVisualizationDock) floatingDocks << dataVisualizationDock;
    if (mindMapDock) floatingDocks << mindMapDock;
    if (collaborationDock) floatingDocks << collaborationDock;
    if (wordCountDock) floatingDocks << wordCountDock;
    if (writingGoalDock) floatingDocks << writingGoalDock;
    if (timelineDock) floatingDocks << timelineDock;

    for (QDockWidget *dock : floatingDocks) {
        dock->setFloating(true);
        dock->hide();
    }

    // UI 中定义的 dock：Markdown 语法参考也设为浮动并隐藏
    ui->dockWidget_2->setFloating(true);
    ui->dockWidget_2->hide();

    // 确保核心 dock 可见并停靠
    ui->fileExplorerDockWidget->setFloating(false);
    ui->fileExplorerDockWidget->show();
    addDockWidget(Qt::LeftDockWidgetArea, ui->fileExplorerDockWidget);

    ui->dockWidget->setFloating(false);
    ui->dockWidget->show();
    addDockWidget(Qt::RightDockWidgetArea, ui->dockWidget);

    // 调整文件浏览器宽度
    resizeDocks({ui->fileExplorerDockWidget}, {200}, Qt::Horizontal);
}

