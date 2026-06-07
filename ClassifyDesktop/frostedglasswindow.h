#pragma once
#include <QLabel>
#include <QPixmap>
#include <QShortcut>
#include <QWidget>

class SettingsWindow;

class FrostedGlassWindow : public QWidget {
    Q_OBJECT
public:
    explicit FrostedGlassWindow(const QString &title, QWidget *parent = nullptr);
    ~FrostedGlassWindow() override;

    void setSigma(float sigma);
    void setOffsetX(int offset);
    void setOffsetY(int offset);
    void setCornerRadius(int radius);
    void setWindowTitleText(const QString &title);
    void setWindowSize(int width, int height);

    float sigma() const { return m_sigma; }
    int offsetX() const { return m_offsetX; }
    int offsetY() const { return m_offsetY; }
    int cornerRadius() const { return m_cornerRadius; }
    QString windowTitleText() const {
        return m_titleLabel ? m_titleLabel->text() : QString();
    }

    // 网格对齐设置（由主控制窗口统一控制）
    static bool gridSnapEnabled();
    static void setGridSnapEnabled(bool enabled);
    static int gridSize();

protected:
    void paintEvent(QPaintEvent *) override;
    void moveEvent(QMoveEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void showEvent(QShowEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    void keyReleaseEvent(QKeyEvent *) override;
    void changeEvent(QEvent *) override;
    void closeEvent(QCloseEvent *) override;
    bool nativeEvent(const QByteArray &eventType, void *message,
                     long long *result) override;

private slots:
    void onSettingsButtonClicked();

private:
    void loadWallpaper();
    void updateBlurBackground();
    QImage applyGaussianBlur(const QImage &src, float sigma) const;
    QRect contentGlobalRect() const;
    void updateSettingsWindowTitle();
    void snapToGrid(); // 将窗口位置吸附到网格

    float m_sigma = 15.0f;
    int m_offsetX = 4;
    int m_offsetY = -18;
    int m_cornerRadius = 10;

    QPixmap m_fullWallpaper;
    QPixmap m_blurredBg;

    bool m_dragging = false;
    QPoint m_dragStartPos;

    SettingsWindow *m_settingsWindow = nullptr;
    QShortcut *m_shortcutSettings = nullptr;
    QLabel *m_titleLabel = nullptr;

    bool m_keyboardMoving = false; // 方向键微调激活标志

    static bool s_gridSnapEnabled;
    static const int s_gridSize = 30;
};