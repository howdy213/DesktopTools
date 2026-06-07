#pragma once
#include <QList>
#include <QTimer>
#include <QWidget>

class FrostedGlassWindow;
class QShortcut;
class QCheckBox;

class MainControlWindow : public QWidget {
    Q_OBJECT
public:
    explicit MainControlWindow(QWidget *parent = nullptr);
    ~MainControlWindow();

signals:
    void requestHideMainWindow();

public slots:
    void toggleVisibility();
    void showMainWindow();
    void hideMainWindowIfPossible();
    void onWindowDestroyed(QObject *window);
private slots:
    void onNewWindow();
    void onSaveLayout();
    void onLoadLayout();
    void onExitApplication();
    void onCloseAllOtherWindows();
    void onGridSnapToggled(bool checked);
    void checkForegroundWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void addWindow(FrostedGlassWindow *window);
    void bringAllToTop(); // 置顶所有窗口

    QList<FrostedGlassWindow *> m_windows;
    QShortcut *m_hotkey;
    QCheckBox *m_gridSnapCheckBox;
    QTimer *m_foregroundTimer;
    bool m_lastModifyDesktopActive = false;
};