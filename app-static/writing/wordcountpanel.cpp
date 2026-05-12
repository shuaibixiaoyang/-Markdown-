// 文件说明：app-static\writing\wordcountpanel.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "wordcountpanel.h"

#include <QRegularExpression>
#include <QScrollBar>
#include <QFrame>

// 函数说明：构造 WordCountPanel 对象，初始化本模块需要的状态、界面和资源。
WordCountPanel::WordCountPanel(QWidget *parent)
    : QWidget(parent)
    , m_editor(nullptr)
    , m_updateTimer(new QTimer(this))
    , m_readingSpeed(200)      // 200 词/分钟
    , m_speakingSpeed(150)     // 150 词/分钟
    , m_updateInterval(500)    // 500ms 更新间隔
    , m_showSelection(true)
    , m_compactMode(false)
{
    setupUi();

    m_updateTimer->setSingleShot(true);
    connect(m_updateTimer, &QTimer::timeout,
            this, &WordCountPanel::performUpdate);
}

// 函数说明：销毁 WordCountPanel 对象，释放本模块持有的资源。
WordCountPanel::~WordCountPanel()
{
}

// 函数说明：初始化 WordCountPanel 的 setupUi 相关界面、动作或服务连接。
void WordCountPanel::setupUi()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(10, 10, 10, 10);
    m_layout->setSpacing(5);

    // 标题
    m_titleLabel = new QLabel(tr("📊 文档统计"), this);
    m_titleLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    m_layout->addWidget(m_titleLabel);

    // 分隔线
    QFrame *line1 = new QFrame(this);
    line1->setFrameShape(QFrame::HLine);
    line1->setFrameShadow(QFrame::Sunken);
    m_layout->addWidget(line1);

    // 字符统计
    m_charactersLabel = new QLabel(tr("字符数: 0"), this);
    m_layout->addWidget(m_charactersLabel);

    // 词语统计
    m_wordsLabel = new QLabel(tr("词语数: 0"), this);
    m_layout->addWidget(m_wordsLabel);

    // 中文字符
    m_chineseLabel = new QLabel(tr("中文字符: 0"), this);
    m_layout->addWidget(m_chineseLabel);

    // 英文单词
    m_englishLabel = new QLabel(tr("英文单词: 0"), this);
    m_layout->addWidget(m_englishLabel);

    // 段落数
    m_paragraphsLabel = new QLabel(tr("段落数: 0"), this);
    m_layout->addWidget(m_paragraphsLabel);

    // 行数
    m_linesLabel = new QLabel(tr("行数: 0"), this);
    m_layout->addWidget(m_linesLabel);

    // 句子数
    m_sentencesLabel = new QLabel(tr("句子数: 0"), this);
    m_layout->addWidget(m_sentencesLabel);

    // 分隔线
    QFrame *line2 = new QFrame(this);
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);
    m_layout->addWidget(line2);

    // 阅读时间
    m_readingTimeLabel = new QLabel(tr("阅读时间: 0 分钟"), this);
    m_layout->addWidget(m_readingTimeLabel);

    // 朗读时间
    m_speakingTimeLabel = new QLabel(tr("朗读时间: 0 分钟"), this);
    m_layout->addWidget(m_speakingTimeLabel);

    // 选中文本统计
    QFrame *line3 = new QFrame(this);
    line3->setFrameShape(QFrame::HLine);
    line3->setFrameShadow(QFrame::Sunken);
    m_layout->addWidget(line3);

    m_selectionTitleLabel = new QLabel(tr("✏️ 选中文本"), this);
    m_selectionTitleLabel->setStyleSheet("font-weight: bold;");
    m_layout->addWidget(m_selectionTitleLabel);

    m_selectionCharsLabel = new QLabel(tr("字符: 0"), this);
    m_layout->addWidget(m_selectionCharsLabel);

    m_selectionWordsLabel = new QLabel(tr("词语: 0"), this);
    m_layout->addWidget(m_selectionWordsLabel);

    // 弹性空间
    m_layout->addStretch();

    setMinimumWidth(150);
}

// 函数说明：设置 WordCountPanel 的运行参数，并触发必要的界面或数据刷新。
void WordCountPanel::setEditor(QPlainTextEdit *editor)
{
    if (m_editor) {
        disconnect(m_editor, nullptr, this, nullptr);
    }

    m_editor = editor;

    if (m_editor) {
        connect(m_editor, &QPlainTextEdit::textChanged,
                this, &WordCountPanel::onTextChanged);
        connect(m_editor, &QPlainTextEdit::selectionChanged,
                this, &WordCountPanel::onSelectionChanged);

        // 立即更新
        updateStatistics();
    }
}

// 函数说明：设置 WordCountPanel 的运行参数，并触发必要的界面或数据刷新。
void WordCountPanel::setReadingSpeed(int wordsPerMinute)
{
    m_readingSpeed = wordsPerMinute;
    updateDisplay();
}

// 函数说明：设置 WordCountPanel 的运行参数，并触发必要的界面或数据刷新。
void WordCountPanel::setSpeakingSpeed(int wordsPerMinute)
{
    m_speakingSpeed = wordsPerMinute;
    updateDisplay();
}

// 函数说明：初始化 WordCountPanel 的 setUpdateInterval 相关界面、动作或服务连接。
void WordCountPanel::setUpdateInterval(int milliseconds)
{
    m_updateInterval = milliseconds;
}

// 函数说明：设置 WordCountPanel 的运行参数，并触发必要的界面或数据刷新。
void WordCountPanel::setShowSelection(bool show)
{
    m_showSelection = show;
    m_selectionTitleLabel->setVisible(show);
    m_selectionCharsLabel->setVisible(show);
    m_selectionWordsLabel->setVisible(show);
}

// 函数说明：设置 WordCountPanel 的运行参数，并触发必要的界面或数据刷新。
void WordCountPanel::setCompactMode(bool compact)
{
    m_compactMode = compact;

    // 在紧凑模式下隐藏部分标签
    m_chineseLabel->setVisible(!compact);
    m_englishLabel->setVisible(!compact);
    m_sentencesLabel->setVisible(!compact);
    m_speakingTimeLabel->setVisible(!compact);
}

// 函数说明：响应 WordCountPanel 收到的信号或异步回调，并更新界面状态。
void WordCountPanel::onTextChanged()
{
    // 使用延迟更新避免频繁计算
    m_updateTimer->start(m_updateInterval);
}

// 函数说明：响应 WordCountPanel 收到的信号或异步回调，并更新界面状态。
void WordCountPanel::onSelectionChanged()
{
    if (m_showSelection) {
        updateSelectionStatistics();
    }
}

// 函数说明：实现 WordCountPanel::performUpdate 的核心逻辑，供当前模块调用。
void WordCountPanel::performUpdate()
{
    updateStatistics();
}

// 函数说明：刷新 WordCountPanel 的内部状态，并同步到相关界面。
void WordCountPanel::updateStatistics()
{
    if (!m_editor) return;

    QString text = m_editor->toPlainText();
    m_statistics = calculateStatistics(text);

    updateDisplay();
    emit statisticsUpdated(m_statistics);
}

// 函数说明：刷新 WordCountPanel 的内部状态，并同步到相关界面。
void WordCountPanel::updateSelectionStatistics()
{
    if (!m_editor) return;

    QTextCursor cursor = m_editor->textCursor();
    if (!cursor.hasSelection()) {
        m_selectionCharsLabel->setText(tr("字符: 0"));
        m_selectionWordsLabel->setText(tr("词语: 0"));
        return;
    }

    QString selectedText = cursor.selectedText();
    // QTextCursor 使用 Unicode 段落分隔符，替换为换行
    selectedText.replace(QChar::ParagraphSeparator, '\n');

    m_selectionStats = calculateStatistics(selectedText);

    m_selectionCharsLabel->setText(tr("字符: %1").arg(m_selectionStats.characters));
    m_selectionWordsLabel->setText(tr("词语: %1").arg(m_selectionStats.words));

    emit selectionStatisticsUpdated(m_selectionStats);
}

// 函数说明：实现 WordCountPanel::calculateStatistics 的核心逻辑，供当前模块调用。
WordCountPanel::Statistics WordCountPanel::calculateStatistics(const QString &text)
{
    Statistics stats;

    if (text.isEmpty()) {
        return stats;
    }

    // 字符数（含空格）
    stats.characters = text.length();

    // 字符数（不含空格）
    QString noSpace = text;
    noSpace.remove(QRegularExpression("\\s"));
    stats.charactersNoSpace = noSpace.length();

    // 中文字符数
    stats.chineseChars = countChineseCharacters(text);

    // 英文单词数
    stats.englishWords = countEnglishWords(text);

    // 总词语数 = 中文字符 + 英文单词
    stats.words = stats.chineseChars + stats.englishWords;

    // 段落数
    stats.paragraphs = countParagraphs(text);

    // 行数
    stats.lines = text.count('\n') + 1;

    // 句子数
    stats.sentences = countSentences(text);

    // 阅读时间（中文按 300 字/分钟，英文按设定速度）
    int chineseReadingTime = stats.chineseChars / 300;
    int englishReadingTime = stats.englishWords / m_readingSpeed;
    stats.readingTimeMinutes = chineseReadingTime + englishReadingTime;
    if (stats.readingTimeMinutes < 1 && stats.words > 0) {
        stats.readingTimeMinutes = 1;
    }

    // 朗读时间
    int chineseSpeakingTime = stats.chineseChars / 200;
    int englishSpeakingTime = stats.englishWords / m_speakingSpeed;
    stats.speakingTimeMinutes = chineseSpeakingTime + englishSpeakingTime;
    if (stats.speakingTimeMinutes < 1 && stats.words > 0) {
        stats.speakingTimeMinutes = 1;
    }

    return stats;
}

// 函数说明：实现 WordCountPanel::countChineseCharacters 的核心逻辑，供当前模块调用。
int WordCountPanel::countChineseCharacters(const QString &text)
{
    int count = 0;
    // 匹配中文字符（基本汉字 + 扩展）
    QRegularExpression chineseRegex("[\\x{4e00}-\\x{9fff}\\x{3400}-\\x{4dbf}\\x{f900}-\\x{faff}]");
    QRegularExpressionMatchIterator it = chineseRegex.globalMatch(text);
    while (it.hasNext()) {
        it.next();
        count++;
    }
    return count;
}

// 函数说明：实现 WordCountPanel::countEnglishWords 的核心逻辑，供当前模块调用。
int WordCountPanel::countEnglishWords(const QString &text)
{
    // 匹配英文单词
    QRegularExpression wordRegex("\\b[a-zA-Z]+\\b");
    QRegularExpressionMatchIterator it = wordRegex.globalMatch(text);
    int count = 0;
    while (it.hasNext()) {
        it.next();
        count++;
    }
    return count;
}

// 函数说明：实现 WordCountPanel::countSentences 的核心逻辑，供当前模块调用。
int WordCountPanel::countSentences(const QString &text)
{
    // 匹配中英文句子结束符
    QRegularExpression sentenceRegex("[.!?。！？]");
    return text.count(sentenceRegex);
}

// 函数说明：实现 WordCountPanel::countParagraphs 的核心逻辑，供当前模块调用。
int WordCountPanel::countParagraphs(const QString &text)
{
    // 段落以空行分隔
    QStringList paragraphs = text.split(QRegularExpression("\n\\s*\n"), Qt::SkipEmptyParts);
    int count = paragraphs.count();

    // 如果没有空行分隔，但有内容，算作一个段落
    if (count == 0 && !text.trimmed().isEmpty()) {
        count = 1;
    }

    return count;
}

// 函数说明：实现 WordCountPanel::formatTime 的核心逻辑，供当前模块调用。
QString WordCountPanel::formatTime(int minutes)
{
    if (minutes < 60) {
        return tr("%1 分钟").arg(minutes);
    } else {
        int hours = minutes / 60;
        int mins = minutes % 60;
        return tr("%1 小时 %2 分钟").arg(hours).arg(mins);
    }
}

// 函数说明：刷新 WordCountPanel 的内部状态，并同步到相关界面。
void WordCountPanel::updateDisplay()
{
    m_charactersLabel->setText(tr("字符数: %1 (不含空格: %2)")
        .arg(m_statistics.characters)
        .arg(m_statistics.charactersNoSpace));

    m_wordsLabel->setText(tr("词语数: %1").arg(m_statistics.words));
    m_chineseLabel->setText(tr("中文字符: %1").arg(m_statistics.chineseChars));
    m_englishLabel->setText(tr("英文单词: %1").arg(m_statistics.englishWords));
    m_paragraphsLabel->setText(tr("段落数: %1").arg(m_statistics.paragraphs));
    m_linesLabel->setText(tr("行数: %1").arg(m_statistics.lines));
    m_sentencesLabel->setText(tr("句子数: %1").arg(m_statistics.sentences));
    m_readingTimeLabel->setText(tr("阅读时间: %1").arg(formatTime(m_statistics.readingTimeMinutes)));
    m_speakingTimeLabel->setText(tr("朗读时间: %1").arg(formatTime(m_statistics.speakingTimeMinutes)));
}

