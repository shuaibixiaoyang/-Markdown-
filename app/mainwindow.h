// 文件说明：app\mainwindow.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef MAINWINDOW_H // 中文注释：开始头文件保护，避免重复包含。
#define MAINWINDOW_H // 中文注释：定义头文件保护宏。

#include <QMainWindow> // 中文注释：引入当前文件需要的依赖头文件。
#include <QMap> // 中文注释：引入当前文件需要的依赖头文件。
#include <QHash> // 中文注释：引入当前文件需要的依赖头文件。
#include <QTabBar> // 中文注释：引入当前文件需要的依赖头文件。
#include <themes/theme.h> // 中文注释：引入当前文件需要的依赖头文件。
#include <publishing/blogpublisher.h> // 中文注释：引入当前文件需要的依赖头文件。
#include <collaboration/collaborationmanager.h> // 中文注释：引入当前文件需要的依赖头文件。

namespace Ui { // 中文注释：声明命名空间范围。
class MainWindow; // 中文注释：声明类类型或前置声明类。
} // 中文注释：结束当前代码块。

class QAction; // 中文注释：声明类类型或前置声明类。
class QActionGroup; // 中文注释：声明类类型或前置声明类。
class QLabel; // 中文注释：声明类类型或前置声明类。
class QDockWidget; // 中文注释：声明类类型或前置声明类。
class ActiveLabel; // 中文注释：声明类类型或前置声明类。
class Dictionary; // 中文注释：声明类类型或前置声明类。
class HtmlPreviewController; // 中文注释：声明类类型或前置声明类。
class HtmlPreviewGenerator; // 中文注释：声明类类型或前置声明类。
class HtmlHighlighter; // 中文注释：声明类类型或前置声明类。
class RecentFilesMenu; // 中文注释：声明类类型或前置声明类。
class Options; // 中文注释：声明类类型或前置声明类。
class SlideLineMapping; // 中文注释：声明类类型或前置声明类。
class SnippetCollection; // 中文注释：声明类类型或前置声明类。
class ThemeCollection; // 中文注释：声明类类型或前置声明类。
class ViewSynchronizer; // 中文注释：声明类类型或前置声明类。
class StatusBarWidget; // 中文注释：声明类类型或前置声明类。
class DataVisualizationPanel; // 中文注释：声明类类型或前置声明类。
class SecurityManager; // 中文注释：声明类类型或前置声明类。
class SecureDocument; // 中文注释：声明类类型或前置声明类。
class EpubExporter; // 中文注释：声明类类型或前置声明类。
class SlideshowPresenter; // 中文注释：声明类类型或前置声明类。
class StaticSiteGenerator; // 中文注释：声明类类型或前置声明类。
class CitationManager; // 中文注释：声明类类型或前置声明类。
class VoiceInput; // 中文注释：声明类类型或前置声明类。
class TranslationManager; // 中文注释：声明类类型或前置声明类。
class WordExporter; // 中文注释：声明类类型或前置声明类。
class LaTeXExporter; // 中文注释：声明类类型或前置声明类。
class MindMapView; // 中文注释：声明类类型或前置声明类。
class NotesLibrary; // 中文注释：声明类类型或前置声明类。
class GitManager; // 中文注释：声明类类型或前置声明类。
class CloudSync; // 中文注释：声明类类型或前置声明类。
class FocusMode; // 中文注释：声明类类型或前置声明类。
class TemplateManager; // 中文注释：声明类类型或前置声明类。
class MultiTabEditor; // 中文注释：声明类类型或前置声明类。
class DocumentSharing; // 中文注释：声明类类型或前置声明类。
class CommentManager; // 中文注释：声明类类型或前置声明类。
class RevisionTracker; // 中文注释：声明类类型或前置声明类。
class ImageExporter; // 中文注释：声明类类型或前置声明类。
class WordCountPanel; // 中文注释：声明类类型或前置声明类。
class OutlineNavigator; // 中文注释：声明类类型或前置声明类。
class BookmarkManager; // 中文注释：声明类类型或前置声明类。
class EnhancedSpellChecker; // 中文注释：声明类类型或前置声明类。
class WritingGoal; // 中文注释：声明类类型或前置声明类。
class PreviewThemeManager; // 中文注释：声明类类型或前置声明类。
class PresentationMode; // 中文注释：声明类类型或前置声明类。
class PrintPreviewDialog; // 中文注释：声明类类型或前置声明类。
class TocFloatingWindow; // 中文注释：声明类类型或前置声明类。
class PluginManager; // 中文注释：声明类类型或前置声明类。
class ScriptEngine; // 中文注释：声明类类型或前置声明类。
class ShortcutManager; // 中文注释：声明类类型或前置声明类。
class EditorThemeManager; // 中文注释：声明类类型或前置声明类。
class AIWritingAssistant; // 中文注释：声明类类型或前置声明类。
class AIWritingPanel; // 中文注释：声明类类型或前置声明类。
class SearchIndexManager; // 中文注释：声明类类型或前置声明类。
class AdvancedSearchPanel; // 中文注释：声明类类型或前置声明类。
class TimelineView; // 中文注释：声明类类型或前置声明类。
class TimelineMiniView; // 中文注释：声明类类型或前置声明类。
class LargeFileManager; // 中文注释：声明类类型或前置声明类。
class VirtualScrollHelper; // 中文注释：声明类类型或前置声明类。
class MemoryOptimizer; // 中文注释：声明类类型或前置声明类。
class ImageCacheManager; // 中文注释：声明类类型或前置声明类。
class DocumentMemoryManager; // 中文注释：声明类类型或前置声明类。

namespace Collaboration { // 中文注释：声明命名空间范围。
class CollaborationManager; // 中文注释：声明类类型或前置声明类。
class CollaborationPanel; // 中文注释：声明类类型或前置声明类。
class RemoteCursorOverlay; // 中文注释：声明类类型或前置声明类。
} // 中文注释：结束当前代码块。


class MainWindow : public QMainWindow // 中文注释：声明类类型或前置声明类。
{ // 中文注释：进入当前代码块。
    Q_OBJECT // 中文注释：启用 Qt 元对象系统能力。

public: // 中文注释：声明类成员的访问权限区域。
    explicit MainWindow(const QString &fileName = QString(), QWidget *parent = nullptr); // 中文注释：声明变量、对象或函数。
    ~MainWindow(); // 中文注释：执行当前语句。

    // 供外部调用（如 macOS Finder 文件打开事件）
    void openFile(const QString &filePath); // 中文注释：声明变量、对象或函数。

protected: // 中文注释：声明类成员的访问权限区域。
    void closeEvent(QCloseEvent *e) override; // 中文注释：声明变量、对象或函数。
    void resizeEvent(QResizeEvent *e) override; // 中文注释：声明变量、对象或函数。

private slots: // 中文注释：声明 Qt 槽函数区域。
    void initializeApp(); // 中文注释：声明变量、对象或函数。
    void openRecentFile(const QString &fileName); // 中文注释：声明变量、对象或函数。
    void languageChanged(const Dictionary &dictionary); // 中文注释：声明变量、对象或函数。

    void fileNew(); // 中文注释：声明变量、对象或函数。
    void fileOpen(); // 中文注释：声明变量、对象或函数。
    bool fileSave(); // 中文注释：声明变量、对象或函数。
    bool fileSaveAs(); // 中文注释：声明变量、对象或函数。
    void fileExportToHtml(); // 中文注释：声明变量、对象或函数。
    void fileExportToPdf(); // 中文注释：声明变量、对象或函数。
    void filePrint(); // 中文注释：声明变量、对象或函数。

    void editUndo(); // 中文注释：声明变量、对象或函数。
    void editRedo(); // 中文注释：声明变量、对象或函数。
    void editCopyHtml(); // 中文注释：声明变量、对象或函数。
    void editGotoLine(); // 中文注释：声明变量、对象或函数。
    void editFindReplace(); // 中文注释：声明变量、对象或函数。
    void editStrong(); // 中文注释：声明变量、对象或函数。
    void editEmphasize(); // 中文注释：声明变量、对象或函数。
    void editStrikethrough(); // 中文注释：声明变量、对象或函数。
    void editInlineCode(); // 中文注释：声明变量、对象或函数。
    void editCenterParagraph(); // 中文注释：声明变量、对象或函数。
    void editHardLinebreak(); // 中文注释：声明变量、对象或函数。
    void editBlockquote(); // 中文注释：声明变量、对象或函数。
    void editIncreaseHeaderLevel(); // 中文注释：声明变量、对象或函数。
    void editDecreaseHeaderLevel(); // 中文注释：声明变量、对象或函数。
    void editInsertTable(); // 中文注释：声明变量、对象或函数。
    void editInsertImage(); // 中文注释：声明变量、对象或函数。

    void viewChangeSplit(); // 中文注释：声明变量、对象或函数。
    void lastUsedTheme(); // 中文注释：声明变量、对象或函数。
    void themeChanged(); // 中文注释：声明变量、对象或函数。
    void editorStyleChanged(); // 中文注释：声明变量、对象或函数。
    void viewFullScreenMode(); // 中文注释：声明变量、对象或函数。
    void viewHorizontalLayout(bool checked); // 中文注释：声明变量、对象或函数。

    void extrasShowSpecialCharacters(bool checked); // 中文注释：声明变量、对象或函数。
    void extrasYamlHeaderSupport(bool checked); // 中文注释：声明变量、对象或函数。
    void extrasWordWrap(bool checked); // 中文注释：声明变量、对象或函数。
    void extensionsAutolink(bool checked); // 中文注释：声明变量、对象或函数。
    void extensionsStrikethrough(bool checked); // 中文注释：声明变量、对象或函数。
    void extensionsAlphabeticLists(bool checked); // 中文注释：声明变量、对象或函数。
    void extensionsDefinitionLists(bool checked); // 中文注释：声明变量、对象或函数。
    void extensionsSmartyPants(bool checked); // 中文注释：声明变量、对象或函数。
    void extensionsFootnotes(bool enabled); // 中文注释：声明变量、对象或函数。
    void extensionsSuperscript(bool enabled); // 中文注释：声明变量、对象或函数。
    void extrasCheckSpelling(bool checked); // 中文注释：声明变量、对象或函数。
    void extrasOptions(); // 中文注释：声明变量、对象或函数。

    void helpMarkdownSyntax(); // 中文注释：声明变量、对象或函数。
    void helpAbout(); // 中文注释：声明变量、对象或函数。

    void setHtmlSource(bool enabled); // 中文注释：声明变量、对象或函数。

    // 数据可视化面板
    void viewToggleDataVisualization(bool checked); // 中文注释：声明变量、对象或函数。
    void onDataVisualizationBlockClicked(int startLine, int endLine); // 中文注释：声明变量、对象或函数。
    void updateDataVisualization(); // 中文注释：声明变量、对象或函数。

    // 安全功能
    void fileEncrypt(); // 中文注释：声明变量、对象或函数。
    void fileDecrypt(); // 中文注释：声明变量、对象或函数。
    void fileOpenEncrypted(); // 中文注释：声明变量、对象或函数。
    void securitySettings(); // 中文注释：声明变量、对象或函数。
    void onDocumentExpired(); // 中文注释：声明变量、对象或函数。
    void onDocumentLocked(); // 中文注释：声明变量、对象或函数。
    void checkBiometricAvailability(); // 中文注释：声明变量、对象或函数。

    // 发布功能
    void publishExportToEpub(); // 中文注释：声明变量、对象或函数。
    void publishToBlog(); // 中文注释：声明变量、对象或函数。
    void publishStartSlideshow(); // 中文注释：声明变量、对象或函数。
    void publishToStaticSite(); // 中文注释：声明变量、对象或函数。
    void publishShowBlogSettings(); // 中文注释：声明变量、对象或函数。
    void publishShowStaticSiteManager(); // 中文注释：声明变量、对象或函数。
    void onBlogPublished(const BlogPublisher::PublishResult &result); // 中文注释：声明变量、对象或函数。
    void onSlideshowEnded(); // 中文注释：声明变量、对象或函数。

    // 工具功能
    void loadDefaultCitationEntries(); // 中文注释：声明变量、对象或函数。
    void toolsShowCitationManager(); // 中文注释：声明变量、对象或函数。
    void toolsInsertCitation(); // 中文注释：声明变量、对象或函数。
    void toolsGenerateBibliography(); // 中文注释：声明变量、对象或函数。
    void toolsStartVoiceInput(); // 中文注释：声明变量、对象或函数。
    void toolsStopVoiceInput(); // 中文注释：声明变量、对象或函数。
    void toolsShowVoiceInputSettings(); // 中文注释：声明变量、对象或函数。
    void toolsTranslateSelection(); // 中文注释：声明变量、对象或函数。
    void toolsTranslateDocument(); // 中文注释：声明变量、对象或函数。
    void toolsShowTranslationSettings(); // 中文注释：声明变量、对象或函数。
    void onVoiceResult(const QString &text); // 中文注释：声明变量、对象或函数。
    void onTranslationCompleted(const QString &result); // 中文注释：声明变量、对象或函数。

    // 导出扩展功能
    void exportToWord(); // 中文注释：声明变量、对象或函数。
    void exportToLaTeX(); // 中文注释：声明变量、对象或函数。
    void exportShowMindMap(); // 中文注释：声明变量、对象或函数。

    // 内容管理功能
    void libraryOpenManager(); // 中文注释：声明变量、对象或函数。
    void libraryQuickSearch(); // 中文注释：声明变量、对象或函数。
    void libraryShowTags(); // 中文注释：声明变量、对象或函数。
    void libraryShowFolders(); // 中文注释：声明变量、对象或函数。
    void gitShowHistory(); // 中文注释：声明变量、对象或函数。
    void gitShowDiff(); // 中文注释：声明变量、对象或函数。
    void gitCommit(); // 中文注释：声明变量、对象或函数。
    void gitPush(); // 中文注释：声明变量、对象或函数。
    void gitPull(); // 中文注释：声明变量、对象或函数。
    void gitEnableLfs(); // 中文注释：声明变量、对象或函数。
    void gitManageLfsTracking(); // 中文注释：声明变量、对象或函数。
    void gitQuickTrackLargeFiles(); // 中文注释：声明变量、对象或函数。
    void syncNow(); // 中文注释：声明变量、对象或函数。
    void syncShowSettings(); // 中文注释：声明变量、对象或函数。
    void onSyncProgress(const QString &fileName, int percent, const QString &message); // 中文注释：声明变量、对象或函数。
    void onSyncCompleted(); // 中文注释：声明变量、对象或函数。
    void onGitStatusChanged(); // 中文注释：声明变量、对象或函数。

    // 焦点模式功能
    void viewToggleFocusMode(bool checked); // 中文注释：声明变量、对象或函数。
    void focusSetScope(); // 中文注释：声明变量、对象或函数。
    void focusSetScrollMode(); // 中文注释：声明变量、对象或函数。
    void focusApplyTheme(); // 中文注释：声明变量、对象或函数。

    // 模板功能
    void fileNewFromTemplate(); // 中文注释：声明变量、对象或函数。
    void templateManage(); // 中文注释：声明变量、对象或函数。
    void templateApply(const QString &templateId); // 中文注释：声明变量、对象或函数。

    // 多标签编辑
    void tabNew(); // 中文注释：声明变量、对象或函数。
    void tabClose(); // 中文注释：声明变量、对象或函数。
    void tabCloseAll(); // 中文注释：声明变量、对象或函数。
    void tabNext(); // 中文注释：声明变量、对象或函数。
    void tabPrevious(); // 中文注释：声明变量、对象或函数。
    void tabRestoreClosed(); // 中文注释：声明变量、对象或函数。
    void onTabChanged(int index); // 中文注释：声明变量、对象或函数。
    void onTabContentChanged(int index); // 中文注释：声明变量、对象或函数。
    void onTabCloseRequested(int index); // 中文注释：声明变量、对象或函数。

    // 协作和分享功能
    void shareDocument(); // 中文注释：声明变量、对象或函数。
    void shareShowManager(); // 中文注释：声明变量、对象或函数。
    void onShareCreated(const QString &shareId, const QString &url); // 中文注释：声明变量、对象或函数。
    void addComment(); // 中文注释：声明变量、对象或函数。
    void showComments(); // 中文注释：声明变量、对象或函数。
    void exportComments(); // 中文注释：声明变量、对象或函数。
    void onCommentAdded(const QString &commentId); // 中文注释：声明变量、对象或函数。
    void showRevisionHistory(); // 中文注释：声明变量、对象或函数。
    void createRevision(); // 中文注释：声明变量、对象或函数。
    void compareRevisions(); // 中文注释：声明变量、对象或函数。
    void restoreRevision(); // 中文注释：声明变量、对象或函数。
    void exportToImage(); // 中文注释：声明变量、对象或函数。
    void exportToPng(); // 中文注释：声明变量、对象或函数。
    void exportToSvg(); // 中文注释：声明变量、对象或函数。
    void onImageExportCompleted(const QString &path); // 中文注释：声明变量、对象或函数。

    // 实时协作功能
    void collabStartHosting(); // 中文注释：声明变量、对象或函数。
    void collabJoinSession(); // 中文注释：声明变量、对象或函数。
    void collabDisconnect(); // 中文注释：声明变量、对象或函数。
    void collabShowPanel(bool checked); // 中文注释：声明变量、对象或函数。
    void onCollabModeChanged(Collaboration::CollaborationManager::Mode mode); // 中文注释：声明变量、对象或函数。
    void onCollabSessionStarted(const QString &sessionId, const QString &url); // 中文注释：声明变量、对象或函数。
    void onCollabSessionEnded(); // 中文注释：声明变量、对象或函数。
    void onCollabError(const QString &error); // 中文注释：声明变量、对象或函数。

    // 写作辅助功能
    void viewToggleWordCount(bool checked); // 中文注释：声明变量、对象或函数。
    void viewToggleOutline(bool checked); // 中文注释：声明变量、对象或函数。
    void bookmarkAdd(); // 中文注释：声明变量、对象或函数。
    void bookmarkRemove(); // 中文注释：声明变量、对象或函数。
    void bookmarkNext(); // 中文注释：声明变量、对象或函数。
    void bookmarkPrevious(); // 中文注释：声明变量、对象或函数。
    void bookmarkShowList(); // 中文注释：声明变量、对象或函数。
    void onBookmarkNavigated(const QString &id); // 中文注释：声明变量、对象或函数。
    void spellCheckDocument(); // 中文注释：声明变量、对象或函数。
    void spellCheckAddToDict(); // 中文注释：声明变量、对象或函数。
    void spellCheckManageDict(); // 中文注释：声明变量、对象或函数。
    void writingSetDailyGoal(); // 中文注释：声明变量、对象或函数。
    void writingSetSessionGoal(); // 中文注释：声明变量、对象或函数。
    void writingShowStats(); // 中文注释：声明变量、对象或函数。
    void onWritingGoalCompleted(); // 中文注释：声明变量、对象或函数。

    // 预览和渲染功能
    void previewChangeTheme(); // 中文注释：声明变量、对象或函数。
    void previewEditTheme(); // 中文注释：声明变量、对象或函数。
    void previewImportTheme(); // 中文注释：声明变量、对象或函数。
    void previewExportTheme(); // 中文注释：声明变量、对象或函数。
    void onPreviewThemeChanged(const QString &themeId); // 中文注释：声明变量、对象或函数。
    void startPresentation(); // 中文注释：声明变量、对象或函数。
    void onPresentationEnded(); // 中文注释：声明变量、对象或函数。
    void showPrintPreview(); // 中文注释：声明变量、对象或函数。
    void toggleTocFloating(); // 中文注释：声明变量、对象或函数。
    void onTocItemClicked(const QString &anchor, int lineNumber); // 中文注释：声明变量、对象或函数。

    // 扩展性功能
    void extShowPluginManager(); // 中文注释：声明变量、对象或函数。
    void extShowScriptManager(); // 中文注释：声明变量、对象或函数。
    void extRunScript(const QString &scriptId); // 中文注释：声明变量、对象或函数。
    void extShowShortcutEditor(); // 中文注释：声明变量、对象或函数。
    void extShowEditorThemeEditor(); // 中文注释：声明变量、对象或函数。
    void extApplyEditorTheme(const QString &themeId); // 中文注释：声明变量、对象或函数。
    void onPluginLoaded(const QString &pluginId); // 中文注释：声明变量、对象或函数。
    void onPluginError(const QString &pluginId, const QString &error); // 中文注释：声明变量、对象或函数。
    void onScriptOutput(const QString &message); // 中文注释：声明变量、对象或函数。

    // AI 智能写作助手
    void aiCompletion(); // 中文注释：声明变量、对象或函数。
    void aiGrammarCheck(); // 中文注释：声明变量、对象或函数。
    void aiSummary(); // 中文注释：声明变量、对象或函数。
    void aiTitleSuggestion(); // 中文注释：声明变量、对象或函数。
    void aiShowPanel(bool checked); // 中文注释：声明变量、对象或函数。
    void aiShowSettings(); // 中文注释：声明变量、对象或函数。
    void onAIInsertText(const QString &text); // 中文注释：声明变量、对象或函数。
    void onAIApplyTitle(const QString &title); // 中文注释：声明变量、对象或函数。
    void setupAIWritingAssistant(); // 中文注释：声明变量、对象或函数。

    // 高级搜索功能
    void searchInFiles(); // 中文注释：声明变量、对象或函数。
    void searchRebuildIndex(); // 中文注释：声明变量、对象或函数。
    void searchShowHistory(); // 中文注释：声明变量、对象或函数。
    void searchClearHistory(); // 中文注释：声明变量、对象或函数。
    void setupAdvancedSearch(); // 中文注释：声明变量、对象或函数。
    void onSearchResultActivated(const QString &filePath, int lineNumber, int column); // 中文注释：声明变量、对象或函数。

    // Dock 布局
    void setupDockLayout(); // 中文注释：声明变量、对象或函数。

    // 时间线/历史记录可视化
    void timelineShowPanel(bool checked); // 中文注释：声明变量、对象或函数。
    void timelineRefresh(); // 中文注释：声明变量、对象或函数。
    void timelineCompareMode(); // 中文注释：声明变量、对象或函数。
    void setupTimelineView(); // 中文注释：声明变量、对象或函数。
    void onTimelineRevisionRestore(const QString &revisionId); // 中文注释：声明变量、对象或函数。
    void onTimelineRevisionSelected(const QString &revisionId); // 中文注释：声明变量、对象或函数。

    // 大文件处理优化
    void setupLargeFileManager(); // 中文注释：声明变量、对象或函数。
    void onLargeFileLoadingStarted(const QString &filePath); // 中文注释：声明变量、对象或函数。
    void onLargeFileLoadingProgress(int percent, const QString &message); // 中文注释：声明变量、对象或函数。
    void onLargeFileLoadingFinished(bool success); // 中文注释：声明变量、对象或函数。
    void onLargeFileMemoryWarning(qint64 currentUsage, qint64 maxUsage); // 中文注释：声明变量、对象或函数。

    // 内存优化
    void setupMemoryOptimizer(); // 中文注释：声明变量、对象或函数。
    void updateMemoryStatusLabel(); // 中文注释：声明变量、对象或函数。
    void onMemoryWarning(qint64 currentUsage, qint64 maxUsage); // 中文注释：声明变量、对象或函数。
    void onMemoryCritical(qint64 currentUsage, qint64 maxUsage); // 中文注释：声明变量、对象或函数。
    void onOptimizationCompleted(qint64 freedMemory); // 中文注释：声明变量、对象或函数。
    void showMemoryStatus(); // 中文注释：声明变量、对象或函数。
    void optimizeMemoryNow(); // 中文注释：声明变量、对象或函数。

    void plainTextChanged(); // 中文注释：声明变量、对象或函数。
    void htmlResultReady(const QString &html); // 中文注释：声明变量、对象或函数。
    void tocResultReady(const QString &toc); // 中文注释：声明变量、对象或函数。

    void previewLinkClicked(const QUrl &url); // 中文注释：声明变量、对象或函数。
    void tocLinkClicked(const QUrl &url); // 中文注释：声明变量、对象或函数。

    void splitterMoved(int pos, int index); // 中文注释：声明变量、对象或函数。

    void addJavaScriptObject(); // 中文注释：声明变量、对象或函数。
    bool load(const QString &fileName); // 中文注释：声明变量、对象或函数。
    void proxyConfigurationChanged(); // 中文注释：声明变量、对象或函数。
    void markdownConverterChanged(); // 中文注释：声明变量、对象或函数。

private: // 中文注释：声明类成员的访问权限区域。
    void setupUi(); // 中文注释：声明变量、对象或函数。
    void setupActions(); // 中文注释：声明变量、对象或函数。
    void setActionsIcons(); // 中文注释：声明变量、对象或函数。
    void setupStatusBar(); // 中文注释：声明变量、对象或函数。
    void setupMarkdownEditor(); // 中文注释：声明变量、对象或函数。
    void setupHtmlPreview(); // 中文注释：声明变量、对象或函数。
    void setupHtmlSourceView(); // 中文注释：声明变量、对象或函数。
    void setupCustomShortcuts(); // 中文注释：声明变量、对象或函数。
    void setCustomShortcut(QMenu *menu); // 中文注释：声明变量、对象或函数。
    void setCustomShortcut(QAction *action); // 中文注释：声明变量、对象或函数。
    void updateExtensionStatus(); // 中文注释：声明变量、对象或函数。
    void syncWebViewToHtmlSource(); // 中文注释：声明变量、对象或函数。
    bool maybeSave(); // 中文注释：声明变量、对象或函数。
    void setFileName(const QString &fileName); // 中文注释：声明变量、对象或函数。
    void updateSplitter(); // 中文注释：声明变量、对象或函数。
    void setupHtmlPreviewThemes(); // 中文注释：声明变量、对象或函数。
    void addSeparatorAfterBuiltInThemes(); // 中文注释：声明变量、对象或函数。
    void loadCustomStyles(); // 中文注释：声明变量、对象或函数。
    void readSettings(); // 中文注释：声明变量、对象或函数。
    void writeSettings(); // 中文注释：声明变量、对象或函数。
    void applyCurrentTheme(); // 中文注释：声明变量、对象或函数。
    void applyPreviewCss(); // 中文注释：声明变量、对象或函数。
    QString stylePath(const QString &styleName); // 中文注释：声明变量、对象或函数。

private: // 中文注释：声明类成员的访问权限区域。
    void setupDataVisualizationPanel(); // 中文注释：声明变量、对象或函数。
    void setupSecurityFeatures(); // 中文注释：声明变量、对象或函数。
    void setupPublishingFeatures(); // 中文注释：声明变量、对象或函数。
    void setupToolsFeatures(); // 中文注释：声明变量、对象或函数。
    void setupExportExtensions(); // 中文注释：声明变量、对象或函数。
    void setupContentManagement(); // 中文注释：声明变量、对象或函数。
    void setupFocusMode(); // 中文注释：声明变量、对象或函数。
    void setupTemplateSystem(); // 中文注释：声明变量、对象或函数。
    void setupMultiTabEditor(); // 中文注释：声明变量、对象或函数。
    void saveCurrentTabState(); // 中文注释：声明变量、对象或函数。
    void restoreTabState(int index); // 中文注释：声明变量、对象或函数。
    void updateTabTitle(int index); // 中文注释：声明变量、对象或函数。
    void setupCollaborationFeatures(); // 中文注释：声明变量、对象或函数。
    void setupWritingAssistance(); // 中文注释：声明变量、对象或函数。
    void setupPreviewFeatures(); // 中文注释：声明变量、对象或函数。
    void setupExtensionFeatures(); // 中文注释：声明变量、对象或函数。
    bool openEncryptedFile(const QString &filePath); // 中文注释：声明变量、对象或函数。
    bool saveAsEncrypted(const QString &password); // 中文注释：声明变量、对象或函数。
    bool ensureGitRepositoryAvailable(const QString &operationTitle); // 中文注释：声明变量、对象或函数。

private: // 中文注释：声明类成员的访问权限区域。
    Ui::MainWindow *ui; // 中文注释：执行当前语句。
    RecentFilesMenu *recentFilesMenu; // 中文注释：执行当前语句。
    Options *options; // 中文注释：执行当前语句。
    QActionGroup *stylesGroup; // 中文注释：声明变量、对象或函数。
    StatusBarWidget* statusBarWidget; // 中文注释：声明变量、对象或函数。
    HtmlPreviewGenerator* generator; // 中文注释：声明变量、对象或函数。
    HtmlHighlighter *htmlHighlighter; // 中文注释：执行当前语句。
    SnippetCollection *snippetCollection; // 中文注释：执行当前语句。
    ViewSynchronizer *viewSynchronizer; // 中文注释：执行当前语句。
    HtmlPreviewController *htmlPreviewController; // 中文注释：执行当前语句。
    ThemeCollection *themeCollection; // 中文注释：执行当前语句。
    Theme currentTheme { "Default", "Default", "Default", "Default" }; // 中文注释：声明变量、对象或函数。
    QString currentPreviewCss;  // 当前预览主题的 CSS 内容 // 中文注释：声明变量、对象或函数。
    QString fileName; // 中文注释：声明变量、对象或函数。
    float splitFactor; // 中文注释：声明变量、对象或函数。
    bool rightViewCollapsed; // 中文注释：声明变量、对象或函数。

    // 数据可视化面板
    QDockWidget *dataVisualizationDock; // 中文注释：声明变量、对象或函数。
    DataVisualizationPanel *dataVisualizationPanel; // 中文注释：执行当前语句。

    // 安全功能
    SecureDocument *secureDocument; // 中文注释：执行当前语句。
    bool isEncryptedMode; // 中文注释：声明变量、对象或函数。
    QAction *actionEncrypt; // 中文注释：声明变量、对象或函数。
    QAction *actionDecrypt; // 中文注释：声明变量、对象或函数。
    QAction *actionSecuritySettings; // 中文注释：声明变量、对象或函数。
    QMenu *securityMenu; // 中文注释：声明变量、对象或函数。

    // 发布功能
    EpubExporter *epubExporter; // 中文注释：执行当前语句。
    BlogPublisher *blogPublisher; // 中文注释：执行当前语句。
    SlideshowPresenter *slideshowPresenter; // 中文注释：执行当前语句。
    StaticSiteGenerator *staticSiteGenerator; // 中文注释：执行当前语句。
    QMenu *publishMenu; // 中文注释：声明变量、对象或函数。
    QAction *actionExportEpub; // 中文注释：声明变量、对象或函数。
    QAction *actionPublishBlog; // 中文注释：声明变量、对象或函数。
    QAction *actionSlideshow; // 中文注释：声明变量、对象或函数。
    QAction *actionStaticSite; // 中文注释：声明变量、对象或函数。

    // 工具功能
    CitationManager *citationManager; // 中文注释：执行当前语句。
    VoiceInput *voiceInput; // 中文注释：执行当前语句。
    TranslationManager *translationManager; // 中文注释：执行当前语句。
    QMenu *toolsMenu; // 中文注释：声明变量、对象或函数。
    QAction *actionCitationManager; // 中文注释：声明变量、对象或函数。
    QAction *actionInsertCitation; // 中文注释：声明变量、对象或函数。
    QAction *actionVoiceInput; // 中文注释：声明变量、对象或函数。
    QAction *actionTranslate; // 中文注释：声明变量、对象或函数。

    // 导出扩展功能
    WordExporter *wordExporter; // 中文注释：执行当前语句。
    LaTeXExporter *latexExporter; // 中文注释：执行当前语句。
    MindMapView *mindMapView; // 中文注释：执行当前语句。
    QDockWidget *mindMapDock; // 中文注释：声明变量、对象或函数。
    QAction *actionExportWord; // 中文注释：声明变量、对象或函数。
    QAction *actionExportLaTeX; // 中文注释：声明变量、对象或函数。
    QAction *actionShowMindMap; // 中文注释：声明变量、对象或函数。

    // 内容管理功能
    NotesLibrary *notesLibrary; // 中文注释：执行当前语句。
    GitManager *gitManager; // 中文注释：执行当前语句。
    CloudSync *cloudSync; // 中文注释：执行当前语句。
    bool m_manualSync = false;  // 区分手动同步和自动同步 // 中文注释：声明变量、对象或函数。
    QDockWidget *libraryDock; // 中文注释：声明变量、对象或函数。
    QMenu *libraryMenu; // 中文注释：声明变量、对象或函数。
    QMenu *gitMenu; // 中文注释：声明变量、对象或函数。
    QMenu *syncMenu; // 中文注释：声明变量、对象或函数。
    QAction *actionLibraryManager; // 中文注释：声明变量、对象或函数。
    QAction *actionQuickSearch; // 中文注释：声明变量、对象或函数。
    QAction *actionGitHistory; // 中文注释：声明变量、对象或函数。
    QAction *actionGitDiff; // 中文注释：声明变量、对象或函数。
    QAction *actionGitCommit; // 中文注释：声明变量、对象或函数。
    QAction *actionGitEnableLfs; // 中文注释：声明变量、对象或函数。
    QAction *actionGitManageLfs; // 中文注释：声明变量、对象或函数。
    QAction *actionGitQuickTrackLargeFiles; // 中文注释：声明变量、对象或函数。
    QAction *actionSyncNow; // 中文注释：声明变量、对象或函数。
    QAction *actionSyncSettings; // 中文注释：声明变量、对象或函数。

    // 焦点模式
    FocusMode *focusMode; // 中文注释：执行当前语句。
    QAction *actionFocusMode; // 中文注释：声明变量、对象或函数。
    QAction *actionFocusScope; // 中文注释：声明变量、对象或函数。
    QAction *actionFocusScrollMode; // 中文注释：声明变量、对象或函数。
    QMenu *focusMenu; // 中文注释：声明变量、对象或函数。

    // 模板系统
    TemplateManager *templateManager; // 中文注释：执行当前语句。
    QMenu *templateMenu; // 中文注释：声明变量、对象或函数。
    QAction *actionNewFromTemplate; // 中文注释：声明变量、对象或函数。
    QAction *actionManageTemplates; // 中文注释：声明变量、对象或函数。

    // 多标签编辑
    struct TabData { // 中文注释：声明结构体类型。
        QString filePath; // 中文注释：声明变量、对象或函数。
        QString content = QStringLiteral("");  // 必须为非 null，generator 用 null 作终止信号 // 中文注释：声明变量、对象或函数。
        int cursorPosition; // 中文注释：声明变量、对象或函数。
        int scrollPosition; // 中文注释：声明变量、对象或函数。
        bool isModified; // 中文注释：声明变量、对象或函数。
        bool isEncryptedMode; // 中文注释：声明变量、对象或函数。
        TabData() : cursorPosition(0), scrollPosition(0), isModified(false), isEncryptedMode(false) {} // 中文注释：保留当前代码结构。
    }; // 中文注释：结束类型或作用域声明。

    QTabBar *m_tabBar = nullptr; // 中文注释：声明变量、对象或函数。
    QList<TabData> m_tabs; // 中文注释：声明变量、对象或函数。
    int m_currentTabIndex = 0; // 中文注释：声明变量、对象或函数。
    QList<TabData> m_closedTabs; // 中文注释：声明变量、对象或函数。
    static const int MAX_CLOSED_TABS = 10; // 中文注释：声明变量、对象或函数。
    bool m_switchingTab = false;  // 防止切换标签时触发内容变化信号 // 中文注释：声明变量、对象或函数。

    MultiTabEditor *multiTabEditor; // 中文注释：执行当前语句。
    QMenu *tabMenu; // 中文注释：声明变量、对象或函数。
    QAction *actionTabNew; // 中文注释：声明变量、对象或函数。
    QAction *actionTabClose; // 中文注释：声明变量、对象或函数。
    QAction *actionTabCloseAll; // 中文注释：声明变量、对象或函数。
    QAction *actionTabNext; // 中文注释：声明变量、对象或函数。
    QAction *actionTabPrevious; // 中文注释：声明变量、对象或函数。
    QAction *actionTabRestoreClosed; // 中文注释：声明变量、对象或函数。

    // 协作和分享功能
    DocumentSharing *documentSharing; // 中文注释：执行当前语句。
    CommentManager *commentManager; // 中文注释：执行当前语句。
    RevisionTracker *revisionTracker; // 中文注释：执行当前语句。
    ImageExporter *imageExporter; // 中文注释：执行当前语句。
    QMenu *shareMenu; // 中文注释：声明变量、对象或函数。
    QMenu *commentMenu; // 中文注释：声明变量、对象或函数。
    QMenu *revisionMenu; // 中文注释：声明变量、对象或函数。
    QAction *actionShareDocument; // 中文注释：声明变量、对象或函数。
    QAction *actionShareManager; // 中文注释：声明变量、对象或函数。
    QAction *actionAddComment; // 中文注释：声明变量、对象或函数。
    QAction *actionShowComments; // 中文注释：声明变量、对象或函数。
    QAction *actionExportComments; // 中文注释：声明变量、对象或函数。
    QAction *actionRevisionHistory; // 中文注释：声明变量、对象或函数。
    QAction *actionCreateRevision; // 中文注释：声明变量、对象或函数。
    QAction *actionCompareRevisions; // 中文注释：声明变量、对象或函数。
    QAction *actionRestoreRevision; // 中文注释：声明变量、对象或函数。
    QAction *actionExportToPng; // 中文注释：声明变量、对象或函数。
    QAction *actionExportToSvg; // 中文注释：声明变量、对象或函数。

    // 实时协作功能
    Collaboration::CollaborationManager *collaborationManager; // 中文注释：执行当前语句。
    Collaboration::CollaborationPanel *collaborationPanel; // 中文注释：执行当前语句。
    Collaboration::RemoteCursorOverlay *remoteCursorOverlay; // 中文注释：执行当前语句。
    QDockWidget *collaborationDock; // 中文注释：声明变量、对象或函数。
    QMenu *collabMenu; // 中文注释：声明变量、对象或函数。
    QAction *actionCollabStartHosting; // 中文注释：声明变量、对象或函数。
    QAction *actionCollabJoinSession; // 中文注释：声明变量、对象或函数。
    QAction *actionCollabDisconnect; // 中文注释：声明变量、对象或函数。
    QAction *actionCollabShowPanel; // 中文注释：声明变量、对象或函数。

    // 写作辅助功能
    WordCountPanel *wordCountPanel; // 中文注释：执行当前语句。
    OutlineNavigator *outlineNavigator; // 中文注释：执行当前语句。
    BookmarkManager *bookmarkManager; // 中文注释：执行当前语句。
    EnhancedSpellChecker *enhancedSpellChecker; // 中文注释：执行当前语句。
    WritingGoal *writingGoal; // 中文注释：执行当前语句。
    QDockWidget *wordCountDock; // 中文注释：声明变量、对象或函数。
    QDockWidget *outlineDock; // 中文注释：声明变量、对象或函数。
    QDockWidget *writingGoalDock; // 中文注释：声明变量、对象或函数。
    QMenu *writingMenu; // 中文注释：声明变量、对象或函数。
    QMenu *bookmarkMenu; // 中文注释：声明变量、对象或函数。
    QAction *actionToggleWordCount; // 中文注释：声明变量、对象或函数。
    QAction *actionToggleOutline; // 中文注释：声明变量、对象或函数。
    QAction *actionBookmarkAdd; // 中文注释：声明变量、对象或函数。
    QAction *actionBookmarkRemove; // 中文注释：声明变量、对象或函数。
    QAction *actionBookmarkNext; // 中文注释：声明变量、对象或函数。
    QAction *actionBookmarkPrevious; // 中文注释：声明变量、对象或函数。
    QAction *actionBookmarkList; // 中文注释：声明变量、对象或函数。
    QAction *actionSpellCheck; // 中文注释：声明变量、对象或函数。
    QAction *actionAddToDict; // 中文注释：声明变量、对象或函数。
    QAction *actionManageDict; // 中文注释：声明变量、对象或函数。
    QAction *actionSetDailyGoal; // 中文注释：声明变量、对象或函数。
    QAction *actionSetSessionGoal; // 中文注释：声明变量、对象或函数。
    QAction *actionWritingStats; // 中文注释：声明变量、对象或函数。

    // 预览和渲染功能
    PreviewThemeManager *previewThemeManager; // 中文注释：执行当前语句。
    PresentationMode *presentationMode; // 中文注释：执行当前语句。
    PrintPreviewDialog *printPreviewDialog; // 中文注释：执行当前语句。
    TocFloatingWindow *tocFloatingWindow; // 中文注释：执行当前语句。
    QMenu *previewMenu; // 中文注释：声明变量、对象或函数。
    QMenu *previewThemeMenu; // 中文注释：声明变量、对象或函数。
    QAction *actionPreviewChangeTheme; // 中文注释：声明变量、对象或函数。
    QAction *actionPreviewEditTheme; // 中文注释：声明变量、对象或函数。
    QAction *actionPreviewImportTheme; // 中文注释：声明变量、对象或函数。
    QAction *actionPreviewExportTheme; // 中文注释：声明变量、对象或函数。
    QAction *actionStartPresentation; // 中文注释：声明变量、对象或函数。
    QAction *actionPrintPreview; // 中文注释：声明变量、对象或函数。
    QAction *actionToggleTocFloat; // 中文注释：声明变量、对象或函数。

    // 扩展性功能
    PluginManager *pluginManager; // 中文注释：执行当前语句。
    ScriptEngine *scriptEngine; // 中文注释：执行当前语句。
    ShortcutManager *shortcutManager; // 中文注释：执行当前语句。
    EditorThemeManager *editorThemeManager; // 中文注释：执行当前语句。
    QMenu *extensionMenu; // 中文注释：声明变量、对象或函数。
    QMenu *pluginMenu; // 中文注释：声明变量、对象或函数。
    QMenu *scriptMenu; // 中文注释：声明变量、对象或函数。
    QMenu *editorThemeMenu; // 中文注释：声明变量、对象或函数。
    QAction *actionPluginManager; // 中文注释：声明变量、对象或函数。
    QAction *actionScriptManager; // 中文注释：声明变量、对象或函数。
    QAction *actionShortcutEditor; // 中文注释：声明变量、对象或函数。
    QAction *actionEditorThemeEditor; // 中文注释：声明变量、对象或函数。

    // AI 智能写作助手
    AIWritingAssistant *aiWritingAssistant; // 中文注释：执行当前语句。
    AIWritingPanel *aiWritingPanel; // 中文注释：执行当前语句。
    QDockWidget *aiWritingDock; // 中文注释：声明变量、对象或函数。
    QMenu *aiMenu; // 中文注释：声明变量、对象或函数。
    QAction *actionAICompletion; // 中文注释：声明变量、对象或函数。
    QAction *actionAIGrammarCheck; // 中文注释：声明变量、对象或函数。
    QAction *actionAISummary; // 中文注释：声明变量、对象或函数。
    QAction *actionAITitle; // 中文注释：声明变量、对象或函数。
    QAction *actionAIShowPanel; // 中文注释：声明变量、对象或函数。
    QAction *actionAISettings; // 中文注释：声明变量、对象或函数。

    // 高级搜索与索引
    SearchIndexManager *searchIndexManager; // 中文注释：执行当前语句。
    AdvancedSearchPanel *advancedSearchPanel; // 中文注释：执行当前语句。
    QDockWidget *searchDock; // 中文注释：声明变量、对象或函数。
    QMenu *searchMenu; // 中文注释：声明变量、对象或函数。
    QAction *actionSearchInFiles; // 中文注释：声明变量、对象或函数。
    QAction *actionSearchRebuildIndex; // 中文注释：声明变量、对象或函数。
    QAction *actionSearchHistory; // 中文注释：声明变量、对象或函数。
    QAction *actionSearchClearHistory; // 中文注释：声明变量、对象或函数。

    // 时间线/历史记录可视化
    TimelineView *timelineView; // 中文注释：执行当前语句。
    TimelineMiniView *timelineMiniView; // 中文注释：执行当前语句。
    QDockWidget *timelineDock; // 中文注释：声明变量、对象或函数。
    QMenu *timelineMenu; // 中文注释：声明变量、对象或函数。
    QAction *actionTimelineShowPanel; // 中文注释：声明变量、对象或函数。
    QAction *actionTimelineRefresh; // 中文注释：声明变量、对象或函数。
    QAction *actionTimelineCompareMode; // 中文注释：声明变量、对象或函数。

    // 大文件处理优化
    LargeFileManager *largeFileManager; // 中文注释：执行当前语句。
    VirtualScrollHelper *virtualScrollHelper; // 中文注释：执行当前语句。
    QLabel *largeFileStatusLabel; // 中文注释：声明变量、对象或函数。

    // 内存优化
    MemoryOptimizer *memoryOptimizer; // 中文注释：执行当前语句。
    QLabel *memoryStatusLabel; // 中文注释：声明变量、对象或函数。
    QAction *actionMemoryOptimize; // 中文注释：声明变量、对象或函数。
    QAction *actionMemoryStatus; // 中文注释：声明变量、对象或函数。
}; // 中文注释：结束类型或作用域声明。

#endif // MAINWINDOW_H // 中文注释：结束条件编译或头文件保护。

