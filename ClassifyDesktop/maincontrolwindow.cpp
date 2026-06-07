#include "MainControlWindow.h"
#include "FrostedGlassWindow.h"
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDebug>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QShortcut>
#include <QVBoxLayout>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

MainControlWindow::MainControlWindow(QWidget *parent)
    : QWidget(parent, Qt::WindowStaysOnTopHint) // 主窗口也置顶
{
    setWindowTitle("主控制窗口");
    setFixedSize(320, 280);

    QVBoxLayout *layout = new QVBoxLayout(this);

    QPushButton *btnNew = new QPushButton("新建毛玻璃窗口");
    QPushButton *btnSave = new QPushButton("保存布局");
    QPushButton *btnLoad = new QPushButton("读取布局");
    QPushButton *btnCloseAll = new QPushButton("关闭所有窗口（不含主窗口）");
    QPushButton *btnExit = new QPushButton("退出程序");
    m_gridSnapCheckBox = new QCheckBox("启用网格吸附 (30px)");
    m_gridSnapCheckBox->setChecked(FrostedGlassWindow::gridSnapEnabled());

    layout->addWidget(btnNew);
    layout->addWidget(btnSave);
    layout->addWidget(btnLoad);
    layout->addWidget(btnCloseAll);
    layout->addWidget(m_gridSnapCheckBox);
    layout->addWidget(btnExit);

    connect(btnNew, &QPushButton::clicked, this, &MainControlWindow::onNewWindow);
    connect(btnSave, &QPushButton::clicked, this,
            &MainControlWindow::onSaveLayout);
    connect(btnLoad, &QPushButton::clicked, this,
            &MainControlWindow::onLoadLayout);
    connect(btnCloseAll, &QPushButton::clicked, this,
            &MainControlWindow::onCloseAllOtherWindows);
    connect(btnExit, &QPushButton::clicked, this,
            &MainControlWindow::onExitApplication);
    connect(m_gridSnapCheckBox, &QCheckBox::toggled, this,
            &MainControlWindow::onGridSnapToggled);

    // 全局快捷键 Ctrl+H
    m_hotkey = new QShortcut(QKeySequence("Ctrl+H"), QApplication::instance());
    m_hotkey->setContext(Qt::ApplicationShortcut);
    connect(m_hotkey, &QShortcut::activated, this,
            &MainControlWindow::toggleVisibility);

    // 定时器检测前台窗口标题
    m_foregroundTimer = new QTimer(this);
    connect(m_foregroundTimer, &QTimer::timeout, this,
            &MainControlWindow::checkForegroundWindow);
    m_foregroundTimer->start(500); // 每500ms检测一次

    // 默认创建一个窗口
    onNewWindow();
}

MainControlWindow::~MainControlWindow() {
    for (FrostedGlassWindow *w : std::as_const(m_windows)) {
        w->close();
        delete w;
    }
    m_windows.clear();
}

void MainControlWindow::onNewWindow() {
    static int counter = 1;
    QString title = QString("毛玻璃窗口 %1").arg(counter++);
    FrostedGlassWindow *win = new FrostedGlassWindow(title, nullptr);
    addWindow(win);
    win->show();
}

void MainControlWindow::addWindow(FrostedGlassWindow *window) {
    m_windows.append(window);
    connect(window, &FrostedGlassWindow::destroyed, this,
            &MainControlWindow::onWindowDestroyed);
}

void MainControlWindow::onWindowDestroyed(QObject *window) {
    FrostedGlassWindow *w = static_cast<FrostedGlassWindow *>(window);
    m_windows.removeOne(w);
}

void MainControlWindow::onSaveLayout() {
    QSettings settings("layout.ini", QSettings::IniFormat);
    settings.clear();

    settings.beginWriteArray("windows");
    for (int i = 0; i < m_windows.size(); ++i) {
        FrostedGlassWindow *win = m_windows[i];
        settings.setArrayIndex(i);
        settings.setValue("geometry", win->saveGeometry());
        settings.setValue("sigma", win->sigma());
        settings.setValue("offsetX", win->offsetX());
        settings.setValue("offsetY", win->offsetY());
        settings.setValue("cornerRadius", win->cornerRadius());
        settings.setValue("title", win->windowTitleText());
    }
    settings.endArray();
    qDebug() << "Layout saved with" << m_windows.size() << "windows.";
}

void MainControlWindow::onLoadLayout() {
    QSettings settings("layout.ini", QSettings::IniFormat);
    int size = settings.beginReadArray("windows");
    if (size == 0) {
        qDebug() << "No layout data found.";
        settings.endArray();
        return;
    }

    // 关闭所有现有窗口
    for (FrostedGlassWindow *win : std::as_const(m_windows)) {
        win->close();
        delete win;
    }
    m_windows.clear();

    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        QByteArray geom = settings.value("geometry").toByteArray();
        float sigma = settings.value("sigma", 15.0f).toFloat();
        int offsetX = settings.value("offsetX", 4).toInt();
        int offsetY = settings.value("offsetY", -18).toInt();
        int cornerRadius = settings.value("cornerRadius", 10).toInt();
        QString title = settings.value("title", "毛玻璃窗口").toString();

        FrostedGlassWindow *win = new FrostedGlassWindow(title, nullptr);
        if (!geom.isEmpty())
            win->restoreGeometry(geom);
        win->setSigma(sigma);
        win->setOffsetX(offsetX);
        win->setOffsetY(offsetY);
        win->setCornerRadius(cornerRadius);

        addWindow(win);
        win->show();
    }
    settings.endArray();
    qDebug() << "Layout loaded with" << size << "windows.";
}

void MainControlWindow::onExitApplication() { QApplication::exit(0); }

void MainControlWindow::onCloseAllOtherWindows() {
    if (m_windows.isEmpty())
        return;

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "关闭所有窗口", "是否在关闭前保存当前布局？",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (reply == QMessageBox::Cancel)
        return;
    if (reply == QMessageBox::Save)
        onSaveLayout();

    // 关闭所有毛玻璃窗口
    for (FrostedGlassWindow *win : m_windows) {
        win->close();
        delete win;
    }
    m_windows.clear();
    // 注意：当 m_windows 为空时，主窗口不会自动退出（因为还有主窗口）
    // 但根据需求，仅关闭所有其他窗口，主窗口仍存在
}

void MainControlWindow::onGridSnapToggled(bool checked) {
    FrostedGlassWindow::setGridSnapEnabled(checked);
}

void MainControlWindow::checkForegroundWindow() {
#ifdef Q_OS_WIN
    HWND hwnd = GetForegroundWindow();
    wchar_t windowTitle[256];
    GetWindowTextW(hwnd, windowTitle, 256);
    QString title = QString::fromWCharArray(windowTitle);
    if (title.contains("ModifyDesktop", Qt::CaseInsensitive)) {
        if (!m_lastModifyDesktopActive) {
            bringAllToTop();
            m_lastModifyDesktopActive = true;
        }
    } else {
        m_lastModifyDesktopActive = false;
    }
#else
    Q_UNUSED(m_lastModifyDesktopActive);
#endif
}

void MainControlWindow::bringAllToTop() {
    // 将所有毛玻璃窗口置顶（已经拥有 WindowStaysOnTopHint，只做 raise）
    for (FrostedGlassWindow *win : std::as_const(m_windows)) {
        win->raise();
        win->activateWindow();
    }
    this->raise();
    this->activateWindow();
}

void MainControlWindow::toggleVisibility() {
    if (isVisible()) {
        hideMainWindowIfPossible();
    } else {
        showMainWindow();
    }
}

void MainControlWindow::showMainWindow()
{
    if (isMinimized())
        setWindowState(windowState() & ~Qt::WindowMinimized);
    show();                     // 显示窗口
    raise();                    // 提升 Z 序
    activateWindow();           // 激活窗口
}

void MainControlWindow::hideMainWindowIfPossible() {
    if (m_windows.size() > 0) {
        hide();
    } else {
        qDebug() << "Cannot hide main window: no other windows exist.";
    }
}

void MainControlWindow::closeEvent(QCloseEvent *event) {
    if (m_windows.size() > 0) {
        hide();
        event->ignore();
    } else {
        QApplication::quit();
        event->accept();
    }
}