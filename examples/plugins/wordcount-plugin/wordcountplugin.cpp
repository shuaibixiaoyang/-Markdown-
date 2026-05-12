#include "wordcountplugin.h"
#include <QPlainTextEdit>
#include <QRegularExpression>

WordCountPlugin::WordCountPlugin(QObject *parent)
    : QObject(parent)
{
}

WordCountPlugin::~WordCountPlugin()
{
}

PluginMetadata WordCountPlugin::metadata() const
{
    PluginMetadata meta;
    meta.id = "wordcount";
    meta.name = "实时字数统计";
    meta.version = "1.0.0";
    meta.author = "CuteMarkEd";
    meta.description = "在侧边栏实时显示字数、行数、中文字数和预估阅读时间";
    meta.license = "GPL-2.0";
    meta.apiVersion = 1;
    return meta;
}

bool WordCountPlugin::initialize(PluginContext *context)
{
    m_context = context;
    m_context->log("WordCountPlugin: 初始化中...");

    // 创建面板 UI
    m_panel = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_panel);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);

    QLabel *titleLabel = new QLabel("<b>字数统计</b>");
    titleLabel->setStyleSheet("font-size: 14px; color: #333;");
    layout->addWidget(titleLabel);

    // 创建统计标签
    auto createLabel = [layout]() -> QLabel* {
        QLabel *label = new QLabel();
        label->setStyleSheet("font-size: 12px; padding: 2px 0;");
        layout->addWidget(label);
        return label;
    };

    m_charLabel = createLabel();
    m_wordLabel = createLabel();
    m_chineseLabel = createLabel();
    m_lineLabel = createLabel();
    m_readTimeLabel = createLabel();

    layout->addStretch();

    // 添加到 Dock
    m_dock = m_context->addDockWidget("字数统计", m_panel, Qt::RightDockWidgetArea);

    // 设置定时器，每 500ms 更新一次统计
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &WordCountPlugin::updateStats);
    m_timer->start(500);

    // 立即更新一次
    updateStats();

    m_context->log("WordCountPlugin: 初始化完成");
    return true;
}

void WordCountPlugin::shutdown()
{
    if (m_timer) {
        m_timer->stop();
    }
    if (m_dock && m_context) {
        m_context->removeDockWidget(m_dock);
        m_dock = nullptr;
    }
    m_context = nullptr;
}

void WordCountPlugin::setEnabled(bool enabled)
{
    m_enabled = enabled;
    if (m_dock) {
        m_dock->setVisible(enabled);
    }
    if (m_timer) {
        if (enabled) {
            m_timer->start(500);
        } else {
            m_timer->stop();
        }
    }
}

bool WordCountPlugin::isEnabled() const
{
    return m_enabled;
}

void WordCountPlugin::updateStats()
{
    if (!m_context || !m_enabled) return;

    QString text = m_context->currentDocument();

    // 总字符数
    int charCount = text.length();
    int charNoSpace = text.count(QRegularExpression("\\S"));

    // 中文字数
    int chineseCount = text.count(QRegularExpression("[\\x{4e00}-\\x{9fff}]"));

    // 英文单词数
    int englishWords = 0;
    QRegularExpressionMatchIterator it = QRegularExpression("[a-zA-Z]+").globalMatch(text);
    while (it.hasNext()) {
        it.next();
        englishWords++;
    }

    // 行数
    int lineCount = text.isEmpty() ? 0 : text.count('\n') + 1;

    // 预估阅读时间
    int readMinutes = qMax(1, static_cast<int>(qRound(chineseCount / 300.0 + englishWords / 200.0)));

    // 更新显示
    m_charLabel->setText(QString("字符数：%1（不含空格 %2）").arg(charCount).arg(charNoSpace));
    m_wordLabel->setText(QString("英文单词：%1").arg(englishWords));
    m_chineseLabel->setText(QString("中文字数：%1").arg(chineseCount));
    m_lineLabel->setText(QString("总行数：%1").arg(lineCount));
    m_readTimeLabel->setText(QString("预估阅读：约 %1 分钟").arg(readMinutes));
}
