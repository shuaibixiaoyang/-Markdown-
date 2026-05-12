// 文件说明：app-static\writing\writinggoal.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef WRITINGGOAL_H
#define WRITINGGOAL_H

#include <QWidget>
#include <QString>
#include <QDateTime>
#include <QTimer>
#include <QProgressBar>
#include <QLabel>
#include <QVBoxLayout>
#include <QPlainTextEdit>

/**
 * @brief 写作目标管理器
 *
 * 功能：
 * - 设定每日/每周/总字数目标
 * - 实时进度追踪
 * - 目标完成通知
 * - 写作历史统计
 * - 连续写作天数追踪
 */
class WritingGoal : public QWidget
{
    Q_OBJECT

public:
    // 目标类型
    enum class GoalType {
        Daily,      // 每日目标
        Weekly,     // 每周目标
        Monthly,    // 每月目标
        Session,    // 本次会话
        Document,   // 文档总目标
        Custom      // 自定义
    };
    Q_ENUM(GoalType)

    // 目标结构
    struct Goal {
        QString id;
        QString name;
        GoalType type;
        int targetWords;        // 目标字数
        int currentWords;       // 当前字数
        int startWords;         // 开始时字数
        QDateTime startTime;    // 开始时间
        QDateTime endTime;      // 结束时间
        bool completed;         // 是否完成
        bool notified;          // 是否已通知

        Goal() : type(GoalType::Daily), targetWords(0)
               , currentWords(0), startWords(0)
               , completed(false), notified(false) {}

        int progress() const {
            if (targetWords <= 0) return 0;
            return qMin(100, (currentWords - startWords) * 100 / targetWords);
        }

        int wordsWritten() const {
            return currentWords - startWords;
        }

        int wordsRemaining() const {
            return qMax(0, targetWords - (currentWords - startWords));
        }
    };

    // 写作统计
    struct WritingStats {
        int totalWordsToday;        // 今日总字数
        int totalWordsThisWeek;     // 本周总字数
        int totalWordsThisMonth;    // 本月总字数
        int consecutiveDays;        // 连续写作天数
        int bestStreak;             // 最长连续天数
        int averageWordsPerDay;     // 日均字数
        int totalSessionMinutes;    // 总写作时间（分钟）
        QDateTime lastWritingTime;  // 最后写作时间

        WritingStats() : totalWordsToday(0), totalWordsThisWeek(0)
                       , totalWordsThisMonth(0), consecutiveDays(0)
                       , bestStreak(0), averageWordsPerDay(0)
                       , totalSessionMinutes(0) {}
    };

    explicit WritingGoal(QWidget *parent = nullptr);
    ~WritingGoal();

    // 设置编辑器
    void setEditor(QPlainTextEdit *editor);

    // 目标管理
    void setDailyGoal(int words);
    void setWeeklyGoal(int words);
    void setSessionGoal(int words);
    void setDocumentGoal(int words);
    void setCustomGoal(const QString &name, int words, const QDateTime &endTime);
    void removeGoal(GoalType type);
    void clearAllGoals();

    // 获取目标
    Goal getDailyGoal() const;
    Goal getWeeklyGoal() const;
    Goal getSessionGoal() const;
    Goal getDocumentGoal() const;
    QVector<Goal> getAllGoals() const { return m_goals; }

    // 统计
    WritingStats getStats() const { return m_stats; }
    void updateStats();

    // 配置
    void setNotifyOnComplete(bool enable);
    void setShowInStatusBar(bool show);
    void setAutoResetDaily(bool enable);

    // 持久化
    bool saveGoals(const QString &filePath = QString());
    bool loadGoals(const QString &filePath = QString());
    bool saveStats(const QString &filePath = QString());
    bool loadStats(const QString &filePath = QString());

signals:
    void goalProgress(GoalType type, int current, int target);
    void goalCompleted(GoalType type);
    void dailyGoalReset();
    void streakUpdated(int days);
    void statsUpdated(const WritingStats &stats);

public slots:
    void updateProgress();
    void resetDailyGoal();
    void resetSessionGoal();

private slots:
    void onTextChanged();
    void checkDailyReset();
    void notifyGoalComplete(GoalType type);

private:
    void setupUi();
    void setupTimers();
    int countWords(const QString &text);
    Goal* findGoal(GoalType type);
    void updateDisplay();
    void checkGoalCompletion();
    QString getGoalsFilePath() const;
    QString getStatsFilePath() const;
    QString goalTypeToString(GoalType type) const;
    GoalType stringToGoalType(const QString &str) const;

    QPlainTextEdit *m_editor;
    QVector<Goal> m_goals;
    WritingStats m_stats;

    // UI 组件
    QVBoxLayout *m_layout;
    QLabel *m_titleLabel;
    QProgressBar *m_dailyProgress;
    QLabel *m_dailyLabel;
    QProgressBar *m_sessionProgress;
    QLabel *m_sessionLabel;
    QLabel *m_streakLabel;
    QLabel *m_statsLabel;

    // 配置
    bool m_notifyOnComplete;
    bool m_showInStatusBar;
    bool m_autoResetDaily;

    // 定时器
    QTimer *m_updateTimer;
    QTimer *m_dailyResetTimer;

    // 状态
    QDate m_lastResetDate;
    int m_sessionStartWords;
    QDateTime m_sessionStartTime;
};

#endif // WRITINGGOAL_H

