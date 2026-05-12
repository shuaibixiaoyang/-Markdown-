// 文件说明：app-static\writing\writinggoal.cpp
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#include "writinggoal.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QDir>
#include <QMessageBox>
#include <QApplication>
#include <QFrame>

// 函数说明：构造 WritingGoal 对象，初始化本模块需要的状态、界面和资源。
WritingGoal::WritingGoal(QWidget *parent)
    : QWidget(parent)
    , m_editor(nullptr)
    , m_notifyOnComplete(true)
    , m_showInStatusBar(true)
    , m_autoResetDaily(true)
    , m_updateTimer(new QTimer(this))
    , m_dailyResetTimer(new QTimer(this))
    , m_sessionStartWords(0)
{
    setupUi();
    setupTimers();
    loadGoals();
    loadStats();
}

// 函数说明：销毁 WritingGoal 对象，释放本模块持有的资源。
WritingGoal::~WritingGoal()
{
    saveGoals();
    saveStats();
}

// 函数说明：初始化 WritingGoal 的 setupUi 相关界面、动作或服务连接。
void WritingGoal::setupUi()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(10, 10, 10, 10);
    m_layout->setSpacing(8);

    // 标题
    m_titleLabel = new QLabel(tr("🎯 写作目标"), this);
    m_titleLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    m_layout->addWidget(m_titleLabel);

    // 分隔线
    QFrame *line1 = new QFrame(this);
    line1->setFrameShape(QFrame::HLine);
    line1->setFrameShadow(QFrame::Sunken);
    m_layout->addWidget(line1);

    // 每日目标
    QLabel *dailyTitle = new QLabel(tr("📅 每日目标"), this);
    m_layout->addWidget(dailyTitle);

    m_dailyProgress = new QProgressBar(this);
    m_dailyProgress->setRange(0, 100);
    m_dailyProgress->setValue(0);
    m_dailyProgress->setTextVisible(true);
    m_dailyProgress->setFormat("%p%");
    m_layout->addWidget(m_dailyProgress);

    m_dailyLabel = new QLabel(tr("0 / 0 字"), this);
    m_dailyLabel->setStyleSheet("color: gray; font-size: 11px;");
    m_layout->addWidget(m_dailyLabel);

    // 本次会话目标
    QLabel *sessionTitle = new QLabel(tr("⏱️ 本次会话"), this);
    m_layout->addWidget(sessionTitle);

    m_sessionProgress = new QProgressBar(this);
    m_sessionProgress->setRange(0, 100);
    m_sessionProgress->setValue(0);
    m_sessionProgress->setTextVisible(true);
    m_sessionProgress->setFormat("%p%");
    m_layout->addWidget(m_sessionProgress);

    m_sessionLabel = new QLabel(tr("0 / 0 字"), this);
    m_sessionLabel->setStyleSheet("color: gray; font-size: 11px;");
    m_layout->addWidget(m_sessionLabel);

    // 分隔线
    QFrame *line2 = new QFrame(this);
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);
    m_layout->addWidget(line2);

    // 连续写作
    m_streakLabel = new QLabel(tr("🔥 连续写作: 0 天"), this);
    m_streakLabel->setStyleSheet("font-weight: bold;");
    m_layout->addWidget(m_streakLabel);

    // 统计
    m_statsLabel = new QLabel(this);
    m_statsLabel->setWordWrap(true);
    m_statsLabel->setStyleSheet("color: gray; font-size: 11px;");
    m_layout->addWidget(m_statsLabel);

    // 弹性空间
    m_layout->addStretch();

    setMinimumWidth(180);
}

// 函数说明：初始化 WritingGoal 的 setupTimers 相关界面、动作或服务连接。
void WritingGoal::setupTimers()
{
    // 更新定时器
    m_updateTimer->setInterval(1000);  // 每秒更新
    connect(m_updateTimer, &QTimer::timeout,
            this, &WritingGoal::updateProgress);
    m_updateTimer->start();

    // 每日重置定时器
    m_dailyResetTimer->setInterval(60000);  // 每分钟检查
    connect(m_dailyResetTimer, &QTimer::timeout,
            this, &WritingGoal::checkDailyReset);
    m_dailyResetTimer->start();

    m_lastResetDate = QDate::currentDate();
}

// 函数说明：设置 WritingGoal 的运行参数，并触发必要的界面或数据刷新。
void WritingGoal::setEditor(QPlainTextEdit *editor)
{
    if (m_editor) {
        disconnect(m_editor, nullptr, this, nullptr);
    }

    m_editor = editor;

    if (m_editor) {
        connect(m_editor, &QPlainTextEdit::textChanged,
                this, &WritingGoal::onTextChanged);

        // 记录会话开始字数
        m_sessionStartWords = countWords(m_editor->toPlainText());
        m_sessionStartTime = QDateTime::currentDateTime();

        updateProgress();
    }
}

// 函数说明：设置 WritingGoal 的运行参数，并触发必要的界面或数据刷新。
void WritingGoal::setDailyGoal(int words)
{
    Goal *goal = findGoal(GoalType::Daily);
    if (goal) {
        goal->targetWords = words;
    } else {
        Goal newGoal;
        newGoal.id = "daily";
        newGoal.name = tr("每日目标");
        newGoal.type = GoalType::Daily;
        newGoal.targetWords = words;
        newGoal.startTime = QDateTime::currentDateTime();
        newGoal.startTime.setTime(QTime(0, 0, 0));
        newGoal.endTime = newGoal.startTime.addDays(1);
        m_goals.append(newGoal);
    }

    saveGoals();
    updateDisplay();
}

// 函数说明：设置 WritingGoal 的运行参数，并触发必要的界面或数据刷新。
void WritingGoal::setWeeklyGoal(int words)
{
    Goal *goal = findGoal(GoalType::Weekly);
    if (goal) {
        goal->targetWords = words;
    } else {
        Goal newGoal;
        newGoal.id = "weekly";
        newGoal.name = tr("每周目标");
        newGoal.type = GoalType::Weekly;
        newGoal.targetWords = words;
        m_goals.append(newGoal);
    }

    saveGoals();
    updateDisplay();
}

// 函数说明：设置 WritingGoal 的运行参数，并触发必要的界面或数据刷新。
void WritingGoal::setSessionGoal(int words)
{
    Goal *goal = findGoal(GoalType::Session);
    if (goal) {
        goal->targetWords = words;
        goal->startWords = m_sessionStartWords;
    } else {
        Goal newGoal;
        newGoal.id = "session";
        newGoal.name = tr("本次会话");
        newGoal.type = GoalType::Session;
        newGoal.targetWords = words;
        newGoal.startTime = m_sessionStartTime;
        newGoal.startWords = m_sessionStartWords;
        m_goals.append(newGoal);
    }

    updateDisplay();
}

// 函数说明：设置 WritingGoal 的运行参数，并触发必要的界面或数据刷新。
void WritingGoal::setDocumentGoal(int words)
{
    Goal *goal = findGoal(GoalType::Document);
    if (goal) {
        goal->targetWords = words;
    } else {
        Goal newGoal;
        newGoal.id = "document";
        newGoal.name = tr("文档目标");
        newGoal.type = GoalType::Document;
        newGoal.targetWords = words;
        newGoal.startWords = 0;
        m_goals.append(newGoal);
    }

    saveGoals();
    updateDisplay();
}

// 函数说明：设置 WritingGoal 的运行参数，并触发必要的界面或数据刷新。
void WritingGoal::setCustomGoal(const QString &name, int words, const QDateTime &endTime)
{
    Goal newGoal;
    newGoal.id = QString("custom_%1").arg(QDateTime::currentMSecsSinceEpoch());
    newGoal.name = name;
    newGoal.type = GoalType::Custom;
    newGoal.targetWords = words;
    newGoal.startTime = QDateTime::currentDateTime();
    newGoal.endTime = endTime;
    m_goals.append(newGoal);

    saveGoals();
    updateDisplay();
}

// 函数说明：从 WritingGoal 管理的数据集合中移除指定内容。
void WritingGoal::removeGoal(GoalType type)
{
    for (int i = m_goals.size() - 1; i >= 0; --i) {
        if (m_goals[i].type == type) {
            m_goals.removeAt(i);
        }
    }
    saveGoals();
    updateDisplay();
}

// 函数说明：清空 WritingGoal 保存的临时状态或缓存数据。
void WritingGoal::clearAllGoals()
{
    m_goals.clear();
    saveGoals();
    updateDisplay();
}

// 函数说明：读取 WritingGoal 当前保存的状态或计算结果。
WritingGoal::Goal WritingGoal::getDailyGoal() const
{
    for (const auto &goal : m_goals) {
        if (goal.type == GoalType::Daily) {
            return goal;
        }
    }
    return Goal();
}

// 函数说明：读取 WritingGoal 当前保存的状态或计算结果。
WritingGoal::Goal WritingGoal::getWeeklyGoal() const
{
    for (const auto &goal : m_goals) {
        if (goal.type == GoalType::Weekly) {
            return goal;
        }
    }
    return Goal();
}

// 函数说明：读取 WritingGoal 当前保存的状态或计算结果。
WritingGoal::Goal WritingGoal::getSessionGoal() const
{
    for (const auto &goal : m_goals) {
        if (goal.type == GoalType::Session) {
            return goal;
        }
    }
    return Goal();
}

// 函数说明：读取 WritingGoal 当前保存的状态或计算结果。
WritingGoal::Goal WritingGoal::getDocumentGoal() const
{
    for (const auto &goal : m_goals) {
        if (goal.type == GoalType::Document) {
            return goal;
        }
    }
    return Goal();
}

// 函数说明：刷新 WritingGoal 的内部状态，并同步到相关界面。
void WritingGoal::updateStats()
{
    if (!m_editor) return;

    int currentWords = countWords(m_editor->toPlainText());
    QDate today = QDate::currentDate();

    // 更新今日字数
    m_stats.totalWordsToday = currentWords;

    // 更新最后写作时间
    m_stats.lastWritingTime = QDateTime::currentDateTime();

    // 计算会话时间
    if (m_sessionStartTime.isValid()) {
        int sessionMinutes = m_sessionStartTime.secsTo(QDateTime::currentDateTime()) / 60;
        m_stats.totalSessionMinutes = sessionMinutes;
    }

    // 更新连续天数
    if (m_stats.totalWordsToday > 0) {
        if (m_stats.lastWritingTime.date() == today) {
            // 今天已写作
        } else if (m_stats.lastWritingTime.date() == today.addDays(-1)) {
            // 昨天写过，今天继续
            m_stats.consecutiveDays++;
        } else {
            // 中断了
            m_stats.consecutiveDays = 1;
        }

        if (m_stats.consecutiveDays > m_stats.bestStreak) {
            m_stats.bestStreak = m_stats.consecutiveDays;
        }

        emit streakUpdated(m_stats.consecutiveDays);
    }

    emit statsUpdated(m_stats);
    saveStats();
}

// 函数说明：设置 WritingGoal 的运行参数，并触发必要的界面或数据刷新。
void WritingGoal::setNotifyOnComplete(bool enable)
{
    m_notifyOnComplete = enable;
}

// 函数说明：设置 WritingGoal 的运行参数，并触发必要的界面或数据刷新。
void WritingGoal::setShowInStatusBar(bool show)
{
    m_showInStatusBar = show;
}

// 函数说明：设置 WritingGoal 的运行参数，并触发必要的界面或数据刷新。
void WritingGoal::setAutoResetDaily(bool enable)
{
    m_autoResetDaily = enable;
}

// 函数说明：保存 WritingGoal 当前状态，保证用户修改可以持久化。
bool WritingGoal::saveGoals(const QString &filePath)
{
    QString path = filePath.isEmpty() ? getGoalsFilePath() : filePath;
    if (path.isEmpty()) return false;

    QJsonArray goalsArray;
    for (const auto &goal : m_goals) {
        QJsonObject obj;
        obj["id"] = goal.id;
        obj["name"] = goal.name;
        obj["type"] = goalTypeToString(goal.type);
        obj["targetWords"] = goal.targetWords;
        obj["currentWords"] = goal.currentWords;
        obj["startWords"] = goal.startWords;
        obj["startTime"] = goal.startTime.toString(Qt::ISODate);
        obj["endTime"] = goal.endTime.toString(Qt::ISODate);
        obj["completed"] = goal.completed;
        goalsArray.append(obj);
    }

    QJsonObject root;
    root["version"] = 1;
    root["goals"] = goalsArray;
    root["lastResetDate"] = m_lastResetDate.toString(Qt::ISODate);

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    file.write(QJsonDocument(root).toJson());
    file.close();
    return true;
}

// 函数说明：加载 WritingGoal 需要的数据、配置或外部资源。
bool WritingGoal::loadGoals(const QString &filePath)
{
    QString path = filePath.isEmpty() ? getGoalsFilePath() : filePath;
    if (path.isEmpty()) return false;

    QFile file(path);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (doc.isNull()) return false;

    QJsonObject root = doc.object();
    m_lastResetDate = QDate::fromString(root["lastResetDate"].toString(), Qt::ISODate);

    QJsonArray goalsArray = root["goals"].toArray();
    m_goals.clear();

    for (const QJsonValue &value : goalsArray) {
        QJsonObject obj = value.toObject();

        Goal goal;
        goal.id = obj["id"].toString();
        goal.name = obj["name"].toString();
        goal.type = stringToGoalType(obj["type"].toString());
        goal.targetWords = obj["targetWords"].toInt();
        goal.currentWords = obj["currentWords"].toInt();
        goal.startWords = obj["startWords"].toInt();
        goal.startTime = QDateTime::fromString(obj["startTime"].toString(), Qt::ISODate);
        goal.endTime = QDateTime::fromString(obj["endTime"].toString(), Qt::ISODate);
        goal.completed = obj["completed"].toBool();

        // 跳过会话目标（每次重新开始）
        if (goal.type != GoalType::Session) {
            m_goals.append(goal);
        }
    }

    updateDisplay();
    return true;
}

// 函数说明：保存 WritingGoal 当前状态，保证用户修改可以持久化。
bool WritingGoal::saveStats(const QString &filePath)
{
    QString path = filePath.isEmpty() ? getStatsFilePath() : filePath;
    if (path.isEmpty()) return false;

    QJsonObject root;
    root["version"] = 1;
    root["totalWordsToday"] = m_stats.totalWordsToday;
    root["totalWordsThisWeek"] = m_stats.totalWordsThisWeek;
    root["totalWordsThisMonth"] = m_stats.totalWordsThisMonth;
    root["consecutiveDays"] = m_stats.consecutiveDays;
    root["bestStreak"] = m_stats.bestStreak;
    root["averageWordsPerDay"] = m_stats.averageWordsPerDay;
    root["totalSessionMinutes"] = m_stats.totalSessionMinutes;
    root["lastWritingTime"] = m_stats.lastWritingTime.toString(Qt::ISODate);

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    file.write(QJsonDocument(root).toJson());
    file.close();
    return true;
}

// 函数说明：加载 WritingGoal 需要的数据、配置或外部资源。
bool WritingGoal::loadStats(const QString &filePath)
{
    QString path = filePath.isEmpty() ? getStatsFilePath() : filePath;
    if (path.isEmpty()) return false;

    QFile file(path);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (doc.isNull()) return false;

    QJsonObject root = doc.object();
    m_stats.totalWordsToday = root["totalWordsToday"].toInt();
    m_stats.totalWordsThisWeek = root["totalWordsThisWeek"].toInt();
    m_stats.totalWordsThisMonth = root["totalWordsThisMonth"].toInt();
    m_stats.consecutiveDays = root["consecutiveDays"].toInt();
    m_stats.bestStreak = root["bestStreak"].toInt();
    m_stats.averageWordsPerDay = root["averageWordsPerDay"].toInt();
    m_stats.totalSessionMinutes = root["totalSessionMinutes"].toInt();
    m_stats.lastWritingTime = QDateTime::fromString(root["lastWritingTime"].toString(), Qt::ISODate);

    return true;
}

// 函数说明：刷新 WritingGoal 的内部状态，并同步到相关界面。
void WritingGoal::updateProgress()
{
    if (!m_editor) return;

    int currentWords = countWords(m_editor->toPlainText());

    // 更新所有目标
    for (int i = 0; i < m_goals.size(); ++i) {
        Goal &goal = m_goals[i];

        if (goal.type == GoalType::Session) {
            goal.currentWords = currentWords;
        } else if (goal.type == GoalType::Document) {
            goal.currentWords = currentWords;
        } else if (goal.type == GoalType::Daily) {
            goal.currentWords = currentWords;
        }

        emit goalProgress(goal.type, goal.wordsWritten(), goal.targetWords);
    }

    checkGoalCompletion();
    updateDisplay();
}

// 函数说明：实现 WritingGoal::resetDailyGoal 的核心逻辑，供当前模块调用。
void WritingGoal::resetDailyGoal()
{
    Goal *daily = findGoal(GoalType::Daily);
    if (daily) {
        daily->startWords = daily->currentWords;
        daily->completed = false;
        daily->notified = false;
        daily->startTime = QDateTime::currentDateTime();
        daily->startTime.setTime(QTime(0, 0, 0));
    }

    m_lastResetDate = QDate::currentDate();
    m_stats.totalWordsToday = 0;

    saveGoals();
    saveStats();

    emit dailyGoalReset();
    updateDisplay();
}

// 函数说明：实现 WritingGoal::resetSessionGoal 的核心逻辑，供当前模块调用。
void WritingGoal::resetSessionGoal()
{
    if (m_editor) {
        m_sessionStartWords = countWords(m_editor->toPlainText());
        m_sessionStartTime = QDateTime::currentDateTime();
    }

    Goal *session = findGoal(GoalType::Session);
    if (session) {
        session->startWords = m_sessionStartWords;
        session->currentWords = m_sessionStartWords;
        session->completed = false;
        session->notified = false;
        session->startTime = m_sessionStartTime;
    }

    updateDisplay();
}

// 函数说明：响应 WritingGoal 收到的信号或异步回调，并更新界面状态。
void WritingGoal::onTextChanged()
{
    updateProgress();
    updateStats();
}

// 函数说明：实现 WritingGoal::checkDailyReset 的核心逻辑，供当前模块调用。
void WritingGoal::checkDailyReset()
{
    if (!m_autoResetDaily) return;

    QDate today = QDate::currentDate();
    if (m_lastResetDate != today) {
        resetDailyGoal();
    }
}

// 函数说明：实现 WritingGoal::notifyGoalComplete 的核心逻辑，供当前模块调用。
void WritingGoal::notifyGoalComplete(GoalType type)
{
    if (!m_notifyOnComplete) return;

    QString message;
    switch (type) {
        case GoalType::Daily:
            message = tr("🎉 恭喜！你已完成今日写作目标！");
            break;
        case GoalType::Weekly:
            message = tr("🎉 太棒了！你已完成本周写作目标！");
            break;
        case GoalType::Session:
            message = tr("🎉 很好！你已完成本次会话目标！");
            break;
        case GoalType::Document:
            message = tr("🎉 文档目标已完成！");
            break;
        default:
            message = tr("🎉 写作目标已完成！");
    }

    QMessageBox::information(this, tr("目标完成"), message);
}

// 函数说明：实现 WritingGoal::countWords 的核心逻辑，供当前模块调用。
int WritingGoal::countWords(const QString &text)
{
    if (text.isEmpty()) return 0;

    int count = 0;

    // 计算中文字符
    QRegularExpression chineseRegex("[\\x{4e00}-\\x{9fff}\\x{3400}-\\x{4dbf}]");
    QRegularExpressionMatchIterator chIt = chineseRegex.globalMatch(text);
    while (chIt.hasNext()) {
        chIt.next();
        count++;
    }

    // 计算英文单词
    QRegularExpression wordRegex("\\b[a-zA-Z]+\\b");
    QRegularExpressionMatchIterator enIt = wordRegex.globalMatch(text);
    while (enIt.hasNext()) {
        enIt.next();
        count++;
    }

    return count;
}

// 函数说明：实现 WritingGoal::findGoal 的核心逻辑，供当前模块调用。
WritingGoal::Goal* WritingGoal::findGoal(GoalType type)
{
    for (int i = 0; i < m_goals.size(); ++i) {
        if (m_goals[i].type == type) {
            return &m_goals[i];
        }
    }
    return nullptr;
}

// 函数说明：刷新 WritingGoal 的内部状态，并同步到相关界面。
void WritingGoal::updateDisplay()
{
    // 更新每日目标
    Goal dailyGoal = getDailyGoal();
    if (dailyGoal.targetWords > 0) {
        m_dailyProgress->setValue(dailyGoal.progress());
        m_dailyLabel->setText(tr("%1 / %2 字 (还需 %3)")
            .arg(dailyGoal.wordsWritten())
            .arg(dailyGoal.targetWords)
            .arg(dailyGoal.wordsRemaining()));

        // 完成时改变颜色
        if (dailyGoal.completed) {
            m_dailyProgress->setStyleSheet("QProgressBar::chunk { background-color: #4CAF50; }");
        } else {
            m_dailyProgress->setStyleSheet("");
        }
    } else {
        m_dailyProgress->setValue(0);
        m_dailyLabel->setText(tr("未设置目标"));
    }

    // 更新会话目标
    Goal sessionGoal = getSessionGoal();
    if (sessionGoal.targetWords > 0) {
        m_sessionProgress->setValue(sessionGoal.progress());
        m_sessionLabel->setText(tr("%1 / %2 字")
            .arg(sessionGoal.wordsWritten())
            .arg(sessionGoal.targetWords));

        if (sessionGoal.completed) {
            m_sessionProgress->setStyleSheet("QProgressBar::chunk { background-color: #4CAF50; }");
        } else {
            m_sessionProgress->setStyleSheet("");
        }
    } else {
        m_sessionProgress->setValue(0);
        m_sessionLabel->setText(tr("未设置目标"));
    }

    // 更新连续天数
    m_streakLabel->setText(tr("🔥 连续写作: %1 天").arg(m_stats.consecutiveDays));

    // 更新统计
    m_statsLabel->setText(tr("最长连续: %1 天\n日均: %2 字")
        .arg(m_stats.bestStreak)
        .arg(m_stats.averageWordsPerDay));
}

// 函数说明：实现 WritingGoal::checkGoalCompletion 的核心逻辑，供当前模块调用。
void WritingGoal::checkGoalCompletion()
{
    for (int i = 0; i < m_goals.size(); ++i) {
        Goal &goal = m_goals[i];

        if (!goal.completed && goal.progress() >= 100) {
            goal.completed = true;

            if (!goal.notified) {
                goal.notified = true;
                emit goalCompleted(goal.type);
                notifyGoalComplete(goal.type);
            }
        }
    }
}

// 函数说明：读取 WritingGoal 当前保存的状态或计算结果。
QString WritingGoal::getGoalsFilePath() const
{
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataPath);
    return dataPath + "/writing_goals.json";
}

// 函数说明：读取 WritingGoal 当前保存的状态或计算结果。
QString WritingGoal::getStatsFilePath() const
{
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataPath);
    return dataPath + "/writing_stats.json";
}

// 函数说明：实现 WritingGoal::goalTypeToString 的核心逻辑，供当前模块调用。
QString WritingGoal::goalTypeToString(GoalType type) const
{
    switch (type) {
        case GoalType::Daily: return "daily";
        case GoalType::Weekly: return "weekly";
        case GoalType::Monthly: return "monthly";
        case GoalType::Session: return "session";
        case GoalType::Document: return "document";
        case GoalType::Custom: return "custom";
        default: return "daily";
    }
}

// 函数说明：实现 WritingGoal::stringToGoalType 的核心逻辑，供当前模块调用。
WritingGoal::GoalType WritingGoal::stringToGoalType(const QString &str) const
{
    if (str == "daily") return GoalType::Daily;
    if (str == "weekly") return GoalType::Weekly;
    if (str == "monthly") return GoalType::Monthly;
    if (str == "session") return GoalType::Session;
    if (str == "document") return GoalType::Document;
    if (str == "custom") return GoalType::Custom;
    return GoalType::Daily;
}

