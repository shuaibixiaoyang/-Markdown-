// 文件说明：app-static\security\securitywidgets.h
// 该文件属于项目代码，用于实现当前模块的核心职责。
// 下方代码为项目原有实现，本次补充的是便于阅读和维护的中文注释。
#ifndef SECURITYWIDGETS_H
#define SECURITYWIDGETS_H

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QTimer>
#include <QPainter>
#include <QStyleOption>

#include <QDateTime>
#include "passwordstrength.h"
#include "biometricauth.h"
#include "fileencryption.h"

/**
 * @brief 密码强度指示器 - 带动画的可视化组件
 */
class PasswordStrengthIndicator : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int strength READ strength WRITE setStrength)

public:
    explicit PasswordStrengthIndicator(QWidget *parent = nullptr)
        : QWidget(parent)
        , m_strength(0)
        , m_analyzer()
    {
        setupUI();
    }

    int strength() const { return m_strength; }

public slots:
    void setStrength(int value) {
        m_strength = qBound(0, value, 100);
        m_progressBar->setValue(m_strength);
        updateAppearance();
    }

    void analyzePassword(const QString &password) {
        PasswordStrength::Analysis analysis = m_analyzer.analyze(password);
        
        // 动画过渡
        QPropertyAnimation *anim = new QPropertyAnimation(this, "strength");
        anim->setDuration(200);
        anim->setStartValue(m_strength);
        anim->setEndValue(analysis.score);
        anim->start(QAbstractAnimation::DeleteWhenStopped);

        m_levelLabel->setText(analysis.levelText);
        m_crackTimeLabel->setText(tr("破解时间: %1").arg(analysis.crackTimeDisplay));

        // 更新警告
        m_warningsLabel->clear();
        if (!analysis.warnings.isEmpty()) {
            m_warningsLabel->setText("⚠ " + analysis.warnings.join(" | "));
        }

        // 更新建议
        m_suggestionsLabel->clear();
        if (analysis.score < 60 && !analysis.suggestions.isEmpty()) {
            m_suggestionsLabel->setText("💡 " + analysis.suggestions.first());
        }
    }

private:
    void setupUI() {
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        // 进度条
        QHBoxLayout *barLayout = new QHBoxLayout();
        
        m_progressBar = new QProgressBar(this);
        m_progressBar->setRange(0, 100);
        m_progressBar->setTextVisible(false);
        m_progressBar->setFixedHeight(8);
        barLayout->addWidget(m_progressBar, 1);

        m_levelLabel = new QLabel(this);
        m_levelLabel->setFixedWidth(60);
        m_levelLabel->setAlignment(Qt::AlignRight);
        barLayout->addWidget(m_levelLabel);

        layout->addLayout(barLayout);

        // 破解时间
        m_crackTimeLabel = new QLabel(this);
        m_crackTimeLabel->setStyleSheet("color: #666; font-size: 11px;");
        layout->addWidget(m_crackTimeLabel);

        // 警告
        m_warningsLabel = new QLabel(this);
        m_warningsLabel->setStyleSheet("color: #f44336; font-size: 11px;");
        m_warningsLabel->setWordWrap(true);
        layout->addWidget(m_warningsLabel);

        // 建议
        m_suggestionsLabel = new QLabel(this);
        m_suggestionsLabel->setStyleSheet("color: #2196f3; font-size: 11px;");
        m_suggestionsLabel->setWordWrap(true);
        layout->addWidget(m_suggestionsLabel);
    }

    void updateAppearance() {
        QString color;
        if (m_strength < 25) {
            color = "#f44336";  // 红色
        } else if (m_strength < 50) {
            color = "#ff9800";  // 橙色
        } else if (m_strength < 75) {
            color = "#ffeb3b";  // 黄色
        } else {
            color = "#4caf50";  // 绿色
        }

        m_progressBar->setStyleSheet(QString(
            "QProgressBar { background: #e0e0e0; border-radius: 4px; }"
            "QProgressBar::chunk { background: %1; border-radius: 4px; }"
        ).arg(color));

        m_levelLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(color));
    }

    int m_strength;
    PasswordStrength m_analyzer;
    QProgressBar *m_progressBar;
    QLabel *m_levelLabel;
    QLabel *m_crackTimeLabel;
    QLabel *m_warningsLabel;
    QLabel *m_suggestionsLabel;
};


/**
 * @brief 密码输入框 - 带显示/隐藏切换和生成器
 */
class SecurePasswordEdit : public QWidget
{
    Q_OBJECT

public:
    explicit SecurePasswordEdit(QWidget *parent = nullptr)
        : QWidget(parent)
        , m_showPassword(false)
    {
        setupUI();
    }

    QString password() const { return m_lineEdit->text(); }
    void setPassword(const QString &password) { m_lineEdit->setText(password); }
    void clear() { m_lineEdit->clear(); }

    QLineEdit* lineEdit() const { return m_lineEdit; }

signals:
    void passwordChanged(const QString &password);

public slots:
    void generatePassword(int length = 16) {
        QString password = FileEncryption::generatePassword(length);
        m_lineEdit->setText(password);
        m_lineEdit->setEchoMode(QLineEdit::Normal);
        m_showPassword = true;
        updateToggleButton();
    }

private slots:
    void togglePasswordVisibility() {
        m_showPassword = !m_showPassword;
        m_lineEdit->setEchoMode(m_showPassword ? QLineEdit::Normal : QLineEdit::Password);
        updateToggleButton();
    }

private:
    void setupUI() {
        QHBoxLayout *layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(4);

        m_lineEdit = new QLineEdit(this);
        m_lineEdit->setEchoMode(QLineEdit::Password);
        m_lineEdit->setPlaceholderText(tr("输入密码"));
        connect(m_lineEdit, &QLineEdit::textChanged, this, &SecurePasswordEdit::passwordChanged);
        layout->addWidget(m_lineEdit, 1);

        m_toggleBtn = new QPushButton(this);
        m_toggleBtn->setFixedSize(28, 28);
        m_toggleBtn->setFlat(true);
        m_toggleBtn->setCursor(Qt::PointingHandCursor);
        connect(m_toggleBtn, &QPushButton::clicked, this, &SecurePasswordEdit::togglePasswordVisibility);
        layout->addWidget(m_toggleBtn);

        m_generateBtn = new QPushButton(tr("生成"), this);
        m_generateBtn->setFixedWidth(50);
        connect(m_generateBtn, &QPushButton::clicked, this, [this]() { generatePassword(); });
        layout->addWidget(m_generateBtn);

        updateToggleButton();
    }

    void updateToggleButton() {
        m_toggleBtn->setText(m_showPassword ? "👁" : "👁‍🗨");
        m_toggleBtn->setToolTip(m_showPassword ? tr("隐藏密码") : tr("显示密码"));
    }

    QLineEdit *m_lineEdit;
    QPushButton *m_toggleBtn;
    QPushButton *m_generateBtn;
    bool m_showPassword;
};


/**
 * @brief 生物特征认证按钮
 */
class BiometricAuthButton : public QPushButton
{
    Q_OBJECT

public:
    explicit BiometricAuthButton(QWidget *parent = nullptr)
        : QPushButton(parent)
        , m_isAuthenticating(false)
    {
        updateAvailability();
        connect(this, &QPushButton::clicked, this, &BiometricAuthButton::startAuthentication);
    }

signals:
    void authenticationSucceeded();
    void authenticationFailed(const QString &reason);

public slots:
    void startAuthentication() {
        if (m_isAuthenticating) return;

        m_isAuthenticating = true;
        setText(tr("认证中..."));
        setEnabled(false);

        BiometricAuth::instance().authenticate(
            tr("解锁文档"),
            [this](BiometricAuth::AuthResult result, const QString &message) {
                m_isAuthenticating = false;
                updateAvailability();
                setEnabled(true);

                if (result == BiometricAuth::AuthResult::Success) {
                    emit authenticationSucceeded();
                } else {
                    emit authenticationFailed(message);
                }
            }
        );
    }

private:
    void updateAvailability() {
        BiometricAuth::Availability avail = BiometricAuth::instance().checkAvailability();
        
        if (avail == BiometricAuth::Availability::Available) {
            setText(tr("使用 %1").arg(BiometricAuth::platformName()));
            setEnabled(true);
            setToolTip(tr("使用生物特征认证"));
        } else if (avail == BiometricAuth::Availability::NotConfigured) {
            setText(tr("未配置 %1").arg(BiometricAuth::platformName()));
            setEnabled(false);
            setToolTip(tr("请先在系统设置中配置生物特征"));
        } else {
            setText(tr("不可用"));
            setEnabled(false);
            setToolTip(tr("此设备不支持生物特征认证"));
        }
    }

    bool m_isAuthenticating;
};


/**
 * @brief 安全状态指示器
 */
class SecurityStatusIndicator : public QWidget
{
    Q_OBJECT

public:
    enum class Status {
        Secure,
        Warning,
        Danger,
        Unknown
    };
    Q_ENUM(Status)

    explicit SecurityStatusIndicator(QWidget *parent = nullptr)
        : QWidget(parent)
        , m_status(Status::Unknown)
    {
        setFixedSize(16, 16);
        setCursor(Qt::PointingHandCursor);
    }

    Status status() const { return m_status; }

public slots:
    void setStatus(Status status, const QString &tooltip = QString()) {
        m_status = status;
        setToolTip(tooltip);
        update();
    }

    void setSecure(const QString &tooltip = QString()) {
        setStatus(Status::Secure, tooltip.isEmpty() ? tr("安全") : tooltip);
    }

    void setWarning(const QString &tooltip = QString()) {
        setStatus(Status::Warning, tooltip.isEmpty() ? tr("警告") : tooltip);
    }

    void setDanger(const QString &tooltip = QString()) {
        setStatus(Status::Danger, tooltip.isEmpty() ? tr("危险") : tooltip);
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        QColor color;
        switch (m_status) {
            case Status::Secure: color = QColor("#4caf50"); break;
            case Status::Warning: color = QColor("#ff9800"); break;
            case Status::Danger: color = QColor("#f44336"); break;
            default: color = QColor("#9e9e9e"); break;
        }

        // 绘制圆形指示器
        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(2, 2, 12, 12);

        // 绘制图标
        painter.setPen(Qt::white);
        QFont font = painter.font();
        font.setPixelSize(10);
        font.setBold(true);
        painter.setFont(font);

        QString icon;
        switch (m_status) {
            case Status::Secure: icon = "✓"; break;
            case Status::Warning: icon = "!"; break;
            case Status::Danger: icon = "✗"; break;
            default: icon = "?"; break;
        }
        painter.drawText(rect(), Qt::AlignCenter, icon);
    }

private:
    Status m_status;
};


/**
 * @brief 加密进度对话框
 */
class EncryptionProgressDialog : public QWidget
{
    Q_OBJECT

public:
    explicit EncryptionProgressDialog(QWidget *parent = nullptr)
        : QWidget(parent, Qt::Dialog | Qt::FramelessWindowHint)
    {
        setAttribute(Qt::WA_TranslucentBackground);
        setFixedSize(300, 120);
        setupUI();
    }

public slots:
    void setProgress(int percent) {
        m_progressBar->setValue(percent);
        m_percentLabel->setText(QString("%1%").arg(percent));
    }

    void setStatus(const QString &status) {
        m_statusLabel->setText(status);
    }

    void showEncrypting() {
        m_titleLabel->setText(tr("🔒 加密中..."));
        m_statusLabel->setText(tr("正在加密文件"));
        show();
    }

    void showDecrypting() {
        m_titleLabel->setText(tr("🔓 解密中..."));
        m_statusLabel->setText(tr("正在解密文件"));
        show();
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        // 绘制背景
        painter.setBrush(QColor(255, 255, 255, 240));
        painter.setPen(QColor(200, 200, 200));
        painter.drawRoundedRect(rect().adjusted(5, 5, -5, -5), 10, 10);
    }

private:
    void setupUI() {
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setContentsMargins(20, 15, 20, 15);

        m_titleLabel = new QLabel(this);
        m_titleLabel->setAlignment(Qt::AlignCenter);
        m_titleLabel->setStyleSheet("font-size: 16px; font-weight: bold;");
        layout->addWidget(m_titleLabel);

        QHBoxLayout *progressLayout = new QHBoxLayout();
        m_progressBar = new QProgressBar(this);
        m_progressBar->setRange(0, 100);
        m_progressBar->setTextVisible(false);
        m_progressBar->setStyleSheet(
            "QProgressBar { background: #e0e0e0; border-radius: 5px; height: 10px; }"
            "QProgressBar::chunk { background: #2196f3; border-radius: 5px; }"
        );
        progressLayout->addWidget(m_progressBar, 1);

        m_percentLabel = new QLabel("0%", this);
        m_percentLabel->setFixedWidth(40);
        m_percentLabel->setAlignment(Qt::AlignRight);
        progressLayout->addWidget(m_percentLabel);

        layout->addLayout(progressLayout);

        m_statusLabel = new QLabel(this);
        m_statusLabel->setAlignment(Qt::AlignCenter);
        m_statusLabel->setStyleSheet("color: #666; font-size: 12px;");
        layout->addWidget(m_statusLabel);
    }

    QLabel *m_titleLabel;
    QProgressBar *m_progressBar;
    QLabel *m_percentLabel;
    QLabel *m_statusLabel;
};


/**
 * @brief 过期倒计时显示
 */
class ExpirationCountdown : public QLabel
{
    Q_OBJECT

public:
    explicit ExpirationCountdown(QWidget *parent = nullptr)
        : QLabel(parent)
        , m_timer(new QTimer(this))
    {
        connect(m_timer, &QTimer::timeout, this, &ExpirationCountdown::updateDisplay);
        setStyleSheet("font-size: 12px; padding: 4px 8px; border-radius: 4px;");
    }

public slots:
    void setExpiration(const QDateTime &expiresAt) {
        m_expiresAt = expiresAt;
        
        if (expiresAt.isValid()) {
            m_timer->start(1000);
            updateDisplay();
            show();
        } else {
            m_timer->stop();
            hide();
        }
    }

    void stop() {
        m_timer->stop();
        hide();
    }

signals:
    void expired();
    void warningThreshold(int secondsRemaining);

private slots:
    void updateDisplay() {
        if (!m_expiresAt.isValid()) {
            return;
        }

        int remaining = QDateTime::currentDateTime().secsTo(m_expiresAt);

        if (remaining <= 0) {
            setText(tr("⚠️ 已过期"));
            setStyleSheet("background: #f44336; color: white; font-size: 12px; "
                         "padding: 4px 8px; border-radius: 4px;");
            m_timer->stop();
            emit expired();
            return;
        }

        QString timeStr;
        if (remaining < 60) {
            timeStr = tr("%1 秒").arg(remaining);
        } else if (remaining < 3600) {
            timeStr = tr("%1 分钟").arg(remaining / 60);
        } else if (remaining < 86400) {
            timeStr = tr("%1 小时").arg(remaining / 3600);
        } else {
            timeStr = tr("%1 天").arg(remaining / 86400);
        }

        setText(tr("⏱ 剩余: %1").arg(timeStr));

        // 更新样式
        if (remaining < 300) {  // 5分钟
            setStyleSheet("background: #f44336; color: white; font-size: 12px; "
                         "padding: 4px 8px; border-radius: 4px;");
            emit warningThreshold(remaining);
        } else if (remaining < 3600) {  // 1小时
            setStyleSheet("background: #ff9800; color: white; font-size: 12px; "
                         "padding: 4px 8px; border-radius: 4px;");
        } else {
            setStyleSheet("background: #e0e0e0; color: #333; font-size: 12px; "
                         "padding: 4px 8px; border-radius: 4px;");
        }
    }

private:
    QDateTime m_expiresAt;
    QTimer *m_timer;
};

#endif // SECURITYWIDGETS_H

