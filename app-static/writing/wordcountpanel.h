// 文件说明：app-static\writing\wordcountpanel.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef WORDCOUNTPANEL_H
#define WORDCOUNTPANEL_H

#include <QWidget>
#include <QString>
#include <QLabel>
#include <QVBoxLayout>
#include <QTimer>
#include <QPlainTextEdit>

/**
 * @brief 字数统计面板
 *
 * 功能：
 * - 实时统计字数、字符数
 * - 统计段落数、行数
 * - 计算预估阅读时间
 * - 显示选中文本统计
 */
class WordCountPanel : public QWidget
{
    Q_OBJECT

public:
    // 统计结果结构
    struct Statistics {
        int characters;         // 字符数（含空格）
        int charactersNoSpace;  // 字符数（不含空格）
        int words;              // 单词/词语数
        int chineseChars;       // 中文字符数
        int englishWords;       // 英文单词数
        int paragraphs;         // 段落数
        int lines;              // 行数
        int sentences;          // 句子数
        int readingTimeMinutes; // 预估阅读时间（分钟）
        int speakingTimeMinutes;// 预估朗读时间（分钟）

        Statistics()
            : characters(0), charactersNoSpace(0), words(0)
            , chineseChars(0), englishWords(0), paragraphs(0)
            , lines(0), sentences(0), readingTimeMinutes(0)
            , speakingTimeMinutes(0) {}
    };

    explicit WordCountPanel(QWidget *parent = nullptr);
    ~WordCountPanel();

    // 设置编辑器
    void setEditor(QPlainTextEdit *editor);

    // 获取统计结果
    Statistics currentStatistics() const { return m_statistics; }
    Statistics selectionStatistics() const { return m_selectionStats; }

    // 配置
    void setReadingSpeed(int wordsPerMinute);    // 默认 200 词/分钟
    void setSpeakingSpeed(int wordsPerMinute);   // 默认 150 词/分钟
    void setUpdateInterval(int milliseconds);    // 更新间隔

    // 显示控制
    void setShowSelection(bool show);
    void setCompactMode(bool compact);

signals:
    void statisticsUpdated(const Statistics &stats);
    void selectionStatisticsUpdated(const Statistics &stats);

public slots:
    void updateStatistics();
    void updateSelectionStatistics();

private slots:
    void onTextChanged();
    void onSelectionChanged();
    void performUpdate();

private:
    void setupUi();
    Statistics calculateStatistics(const QString &text);
    int countChineseCharacters(const QString &text);
    int countEnglishWords(const QString &text);
    int countSentences(const QString &text);
    int countParagraphs(const QString &text);
    QString formatTime(int minutes);
    void updateDisplay();

    QPlainTextEdit *m_editor;
    QTimer *m_updateTimer;

    // UI 组件
    QVBoxLayout *m_layout;
    QLabel *m_titleLabel;
    QLabel *m_charactersLabel;
    QLabel *m_wordsLabel;
    QLabel *m_chineseLabel;
    QLabel *m_englishLabel;
    QLabel *m_paragraphsLabel;
    QLabel *m_linesLabel;
    QLabel *m_sentencesLabel;
    QLabel *m_readingTimeLabel;
    QLabel *m_speakingTimeLabel;

    // 选中文本统计
    QLabel *m_selectionTitleLabel;
    QLabel *m_selectionCharsLabel;
    QLabel *m_selectionWordsLabel;

    // 统计数据
    Statistics m_statistics;
    Statistics m_selectionStats;

    // 配置
    int m_readingSpeed;     // 阅读速度（词/分钟）
    int m_speakingSpeed;    // 朗读速度（词/分钟）
    int m_updateInterval;   // 更新间隔
    bool m_showSelection;   // 显示选中统计
    bool m_compactMode;     // 紧凑模式
};

#endif // WORDCOUNTPANEL_H

