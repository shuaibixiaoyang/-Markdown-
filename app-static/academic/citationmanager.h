// 文件说明：app-static\academic\citationmanager.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef CITATIONMANAGER_H
#define CITATIONMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QVector>
#include <QDateTime>
#include <QVariant>

/**
 * @brief 引用管理器
 *
 * 功能：
 * - 解析和管理 BibTeX 文件
 * - 支持多种引用类型（文章、书籍、会议等）
 * - 生成引用文本（多种格式：APA, MLA, Chicago, IEEE等）
 * - 插入 Markdown 引用
 * - 生成参考文献列表
 */
class CitationManager : public QObject
{
    Q_OBJECT

public:
    // 引用类型
    enum class EntryType {
        Article,        // 期刊文章
        Book,           // 书籍
        Booklet,        // 小册子
        Conference,     // 会议论文 (同 InProceedings)
        InBook,         // 书籍章节
        InCollection,   // 论文集中的文章
        InProceedings,  // 会议论文
        Manual,         // 技术手册
        MastersThesis,  // 硕士论文
        Misc,           // 其他
        PhdThesis,      // 博士论文
        Proceedings,    // 会议论文集
        TechReport,     // 技术报告
        Unpublished,    // 未发表
        Online,         // 在线资源
        Patent,         // 专利
        Unknown
    };
    Q_ENUM(EntryType)

    // 引用样式
    enum class CitationStyle {
        APA,            // APA 第7版
        MLA,            // MLA 第9版
        Chicago,        // Chicago 第17版
        IEEE,           // IEEE
        Harvard,        // Harvard
        Vancouver,      // Vancouver (医学)
        GB7714,         // 中国国标 GB/T 7714-2015
        Custom          // 自定义
    };
    Q_ENUM(CitationStyle)

    // BibTeX 条目
    struct BibEntry {
        QString key;                        // 引用键 (如 smith2020)
        EntryType type;                     // 条目类型
        QString title;                      // 标题
        QStringList authors;                // 作者列表
        QString year;                       // 年份
        QString month;                      // 月份
        QString journal;                    // 期刊名
        QString booktitle;                  // 书名/会议名
        QString publisher;                  // 出版社
        QString address;                    // 出版地
        QString volume;                     // 卷号
        QString number;                     // 期号
        QString pages;                      // 页码
        QString edition;                    // 版本
        QString editor;                     // 编辑
        QString chapter;                    // 章节
        QString series;                     // 系列
        QString school;                     // 学校（论文）
        QString institution;                // 机构
        QString organization;               // 组织
        QString howpublished;               // 发布方式
        QString note;                       // 备注
        QString doi;                        // DOI
        QString url;                        // URL
        QString urldate;                    // 访问日期
        QString isbn;                       // ISBN
        QString issn;                       // ISSN
        QString abstract;                   // 摘要
        QString keywords;                   // 关键词
        QString language;                   // 语言
        QMap<QString, QString> customFields; // 自定义字段

        // 元数据
        QDateTime addedDate;                // 添加日期
        QDateTime modifiedDate;             // 修改日期
        QStringList tags;                   // 标签
        int citationCount;                  // 被引用次数
        QString filePath;                   // 关联的 PDF 文件

        BibEntry() : type(EntryType::Unknown), citationCount(0) {}
    };

    // 引用库
    struct Library {
        QString name;
        QString filePath;
        QVector<BibEntry> entries;
        QDateTime lastModified;
    };

    explicit CitationManager(QObject *parent = nullptr);
    ~CitationManager();

    // 库管理
    bool loadLibrary(const QString &filePath);
    bool saveLibrary(const QString &filePath = QString());
    bool createLibrary(const QString &filePath, const QString &name);
    void closeLibrary();
    bool isLibraryLoaded() const { return !m_library.filePath.isEmpty(); }
    Library currentLibrary() const { return m_library; }

    // 条目管理
    bool addEntry(const BibEntry &entry);
    bool updateEntry(const QString &key, const BibEntry &entry);
    bool removeEntry(const QString &key);
    BibEntry findEntry(const QString &key) const;
    QVector<BibEntry> allEntries() const { return m_library.entries; }
    QVector<BibEntry> searchEntries(const QString &query) const;
    QVector<BibEntry> filterByType(EntryType type) const;
    QVector<BibEntry> filterByTag(const QString &tag) const;
    QVector<BibEntry> filterByYear(const QString &year) const;
    QVector<BibEntry> filterByAuthor(const QString &author) const;

    // BibTeX 解析和生成
    static QVector<BibEntry> parseBibTeX(const QString &content);
    static QString generateBibTeX(const QVector<BibEntry> &entries);
    static QString generateBibTeX(const BibEntry &entry);

    // 引用格式化
    void setCitationStyle(CitationStyle style);
    CitationStyle citationStyle() const { return m_citationStyle; }

    QString formatCitation(const QString &key) const;
    QString formatCitation(const BibEntry &entry) const;
    QString formatInTextCitation(const QString &key) const;
    QString formatInTextCitation(const QStringList &keys) const;
    QString formatBibliography(const QStringList &keys) const;
    QString formatBibliography() const;  // 所有已引用的条目

    // Markdown 集成
    QString insertCitation(const QString &key);  // 返回 Markdown 引用 [@key]
    QString insertCitations(const QStringList &keys);  // [@key1; @key2]
    QString generateMarkdownBibliography(const QStringList &keys);

    // 从在线数据库导入
    void importFromDOI(const QString &doi);
    void importFromISBN(const QString &isbn);
    void importFromArXiv(const QString &arxivId);
    void importFromPubMed(const QString &pmid);

    // 导出
    bool exportToRIS(const QString &filePath, const QVector<BibEntry> &entries);
    bool exportToEndNote(const QString &filePath, const QVector<BibEntry> &entries);
    bool exportToCSV(const QString &filePath, const QVector<BibEntry> &entries);

    // 错误处理
    QString lastError() const { return m_lastError; }

signals:
    void libraryLoaded(const QString &filePath);
    void librarySaved();
    void libraryModified();
    void entryAdded(const QString &key);
    void entryUpdated(const QString &key);
    void entryRemoved(const QString &key);
    void importCompleted(const BibEntry &entry);
    void importFailed(const QString &error);
    void errorOccurred(const QString &error);

private:
    // BibTeX 解析辅助
    static EntryType parseEntryType(const QString &typeStr);
    static QString entryTypeToString(EntryType type);
    static QStringList parseAuthors(const QString &authorsStr);
    static QString formatAuthorsAPA(const QStringList &authors);
    static QString formatAuthorsMLA(const QStringList &authors);
    static QString formatAuthorsChicago(const QStringList &authors);
    static QString formatAuthorsIEEE(const QStringList &authors);
    static QString formatAuthorsGB7714(const QStringList &authors);

    // 格式化辅助
    QString formatAPA(const BibEntry &entry) const;
    QString formatMLA(const BibEntry &entry) const;
    QString formatChicago(const BibEntry &entry) const;
    QString formatIEEE(const BibEntry &entry) const;
    QString formatHarvard(const BibEntry &entry) const;
    QString formatVancouver(const BibEntry &entry) const;
    QString formatGB7714(const BibEntry &entry) const;

    // 生成唯一键
    QString generateUniqueKey(const BibEntry &entry) const;

    Library m_library;
    CitationStyle m_citationStyle;
    QStringList m_citedKeys;  // 已引用的键列表
    QString m_lastError;
};

#endif // CITATIONMANAGER_H

