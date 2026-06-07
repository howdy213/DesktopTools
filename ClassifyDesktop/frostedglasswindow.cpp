#include "FrostedGlassWindow.h"
#include "SettingsWindow.h"
#include <QDebug>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QScreen>
#include <QSettings>
#include <QShortcut>
#include <QWindow>

#ifdef Q_OS_WIN
#include <dwmapi.h>
#include <windows.h>
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")
#endif

#include "fast_gaussian_blur_template.h"

bool FrostedGlassWindow::s_gridSnapEnabled = false;

FrostedGlassWindow::FrostedGlassWindow(const QString &title, QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowSystemMenuHint |
                          Qt::Window | Qt::WindowStaysOnTopHint) {
    setMinimumSize(50, 50);
    resize(400, 300);
    setWindowTitle(title);

    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);

    // 标题栏
    QWidget *titleBar = new QWidget(this);
    titleBar->setObjectName("titleBar");
    titleBar->setStyleSheet("background: transparent;");
    QHBoxLayout *lay = new QHBoxLayout(titleBar);
    lay->setContentsMargins(10, 0, 10, 0);

    m_titleLabel = new QLabel(title);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet("color: black; font-size: 14px;");
    lay->addWidget(m_titleLabel, 1);
    lay->addStretch();

    titleBar->setFixedHeight(40);
    titleBar->move(0, 0);
    titleBar->setFixedWidth(width());

    // Ctrl+S 快捷键
    m_shortcutSettings = new QShortcut(QKeySequence("Ctrl+S"), this);
    connect(m_shortcutSettings, &QShortcut::activated, this,
            &FrostedGlassWindow::onSettingsButtonClicked);

    loadWallpaper();
}

FrostedGlassWindow::~FrostedGlassWindow() {
    if (m_settingsWindow) {
        m_settingsWindow->close();
        delete m_settingsWindow;
    }
}

void FrostedGlassWindow::setGridSnapEnabled(bool enabled) {
    s_gridSnapEnabled = enabled;
}

bool FrostedGlassWindow::gridSnapEnabled() { return s_gridSnapEnabled; }

int FrostedGlassWindow::gridSize() { return s_gridSize; }

void FrostedGlassWindow::snapToGrid() {
    if (!s_gridSnapEnabled)
        return;
    QPoint pos = this->pos();
    int g = s_gridSize;
    int newX = ((pos.x() + g / 2) / g) * g;
    int newY = ((pos.y() + g / 2) / g) * g;
    if (pos.x() != newX || pos.y() != newY)
        move(newX, newY);
}

QRect FrostedGlassWindow::contentGlobalRect() const {
    QPoint topLeft = mapToGlobal(QPoint(0, 0));
    topLeft += QPoint(m_offsetX, m_offsetY);
    return QRect(topLeft, size());
}

void FrostedGlassWindow::loadWallpaper() {
#ifdef Q_OS_WIN
    wchar_t wallpaperPath[MAX_PATH];
    if (!SystemParametersInfoW(SPI_GETDESKWALLPAPER, MAX_PATH, wallpaperPath,
                               0)) {
        qWarning() << "Failed to get wallpaper path";
        return;
    }
    QString path = QString::fromWCharArray(wallpaperPath);
    if (!QFileInfo::exists(path)) {
        qWarning() << "Wallpaper file not found:" << path;
        return;
    }

    QPixmap original(path);
    if (original.isNull()) {
        qWarning() << "Failed to load wallpaper:" << path;
        return;
    }

    int vx = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int vy = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int vw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int vh = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    QSize virtualSize(vw, vh);

    QSettings wallSettings("HKEY_CURRENT_USER\\Control Panel\\Desktop",
                           QSettings::NativeFormat);
    int style = wallSettings.value("WallpaperStyle", 2).toInt();
    int tile = wallSettings.value("TileWallpaper", 0).toInt();

    m_fullWallpaper = QPixmap(virtualSize);
    m_fullWallpaper.fill(Qt::black);
    QPainter p(&m_fullWallpaper);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    if (tile != 0) {
        p.drawTiledPixmap(QRect(QPoint(0, 0), virtualSize), original);
    } else {
        QSize imgSize = original.size();
        QRectF dest;
        QSizeF drawSize = imgSize;

        switch (style) {
        case 0: // 居中
            dest = QRectF((vw - imgSize.width()) / 2.0, (vh - imgSize.height()) / 2.0,
                          imgSize.width(), imgSize.height());
            break;
        case 2: // 拉伸
            dest = QRectF(0, 0, vw, vh);
            break;
        case 6: // 适应
        {
            double s =
                qMin((double)vw / imgSize.width(), (double)vh / imgSize.height());
            QSizeF newSize(imgSize.width() * s, imgSize.height() * s);
            dest = QRectF((vw - newSize.width()) / 2.0, (vh - newSize.height()) / 2.0,
                          newSize.width(), newSize.height());
            break;
        }
        case 10: // 填充
        {
            double s =
                qMax((double)vw / imgSize.width(), (double)vh / imgSize.height());
            QSizeF newSize(imgSize.width() * s, imgSize.height() * s);
            dest = QRectF((vw - newSize.width()) / 2.0, (vh - newSize.height()) / 2.0,
                          newSize.width(), newSize.height());
            break;
        }
        default:
            dest = QRectF(0, 0, vw, vh);
            break;
        }

        p.drawPixmap(dest, original, QRectF(QPointF(0, 0), drawSize));
    }
    p.end();

    qDebug() << "Wallpaper loaded," << "style:" << style << "tile:" << tile;
#else
    m_fullWallpaper = QPixmap(800, 600);
    m_fullWallpaper.fill(Qt::darkGray);
#endif
}

void FrostedGlassWindow::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
#ifdef Q_OS_WIN
    if (QWindow *win = windowHandle()) {
        HWND hwnd = reinterpret_cast<HWND>(win->winId());
        DWMNCRENDERINGPOLICY ncrp = DWMNCRP_DISABLED;
        DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY, &ncrp, sizeof(ncrp));
        BOOL disableTrans = TRUE;
        DwmSetWindowAttribute(hwnd, DWMWA_TRANSITIONS_FORCEDISABLED, &disableTrans,
                              sizeof(disableTrans));
        const int DWMWA_WINDOW_CORNER_PREFERENCE = 33;
        const int DWMWCP_DONOTROUND = 1;
        DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE,
                              &DWMWCP_DONOTROUND, sizeof(DWMWCP_DONOTROUND));
    }
#endif
    updateBlurBackground();
    snapToGrid(); // 显示时吸附网格
}

void FrostedGlassWindow::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    QWidget *tb = findChild<QWidget *>("titleBar");
    if (tb)
        tb->setFixedWidth(width());
    updateBlurBackground();
}

void FrostedGlassWindow::moveEvent(QMoveEvent *event) {
    QWidget::moveEvent(event);
    updateBlurBackground();
}

void FrostedGlassWindow::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QPainterPath windowPath;
    windowPath.addRoundedRect(rect(), m_cornerRadius, m_cornerRadius);

    p.setClipPath(windowPath);
    if (!m_blurredBg.isNull()) {
        p.drawPixmap(rect(), m_blurredBg);
    } else {
        p.fillRect(rect(), QColor(60, 60, 60, 200));
    }

    QRect titleRect(0, 0, width(), 40);
    QPainterPath titleArea;
    titleArea.addRect(titleRect);
    p.setClipPath(windowPath.intersected(titleArea));
    p.fillRect(titleRect, QColor(255, 255, 255, 80));

    p.setClipping(false);
    QPen pen(QColor(255, 255, 255, 60));
    pen.setWidthF(1.0);
    p.setPen(pen);
    p.drawRoundedRect(rect(), m_cornerRadius, m_cornerRadius);
}

void FrostedGlassWindow::updateBlurBackground() {
    if (m_fullWallpaper.isNull())
        return;
    QRect globalRect = contentGlobalRect();
    if (globalRect.width() <= 0 || globalRect.height() <= 0)
        return;

    QPixmap clip = m_fullWallpaper.copy(globalRect);
    if (clip.isNull())
        return;

    QImage img = clip.toImage().convertToFormat(QImage::Format_RGBA8888);
    QImage blurred = applyGaussianBlur(img, m_sigma);
    m_blurredBg = QPixmap::fromImage(blurred);
    update();
}

QImage FrostedGlassWindow::applyGaussianBlur(const QImage &src,
                                             float sigma) const {
    int w = src.width(), h = src.height(), c = 4;
    uchar *buf1 = new uchar[w * h * c];
    uchar *buf2 = new uchar[w * h * c];
    std::memcpy(buf1, src.bits(), w * h * c);

    fast_gaussian_blur<uchar>(buf1, buf2, w, h, c, sigma, 3, kMirror);

    QImage result(buf2, w, h, QImage::Format_RGBA8888);
    QImage copy = result.copy();
    delete[] buf1;
    delete[] buf2;
    return copy;
}

void FrostedGlassWindow::setSigma(float sigma) {
    m_sigma = sigma;
    updateBlurBackground();
}

void FrostedGlassWindow::setOffsetX(int offset) {
    m_offsetX = offset;
    updateBlurBackground();
}

void FrostedGlassWindow::setOffsetY(int offset) {
    m_offsetY = offset;
    updateBlurBackground();
}

void FrostedGlassWindow::setCornerRadius(int radius) {
    m_cornerRadius = radius;
    update();
}

void FrostedGlassWindow::setWindowTitleText(const QString &title) {
    if (m_titleLabel) {
        m_titleLabel->setText(title);
    }
    setWindowTitle(title);
    updateSettingsWindowTitle();
}

void FrostedGlassWindow::setWindowSize(int width, int height) {
    width = qMax(minimumWidth(), width);
    height = qMax(minimumHeight(), height);
    resize(width, height);
}

void FrostedGlassWindow::updateSettingsWindowTitle() {
    if (m_settingsWindow) {
        m_settingsWindow->updateTitle(m_titleLabel ? m_titleLabel->text()
                                                   : QString());
    }
}

void FrostedGlassWindow::onSettingsButtonClicked() {
    if (!m_settingsWindow) {
        m_settingsWindow = new SettingsWindow(
            static_cast<int>(m_sigma), m_offsetX, m_offsetY, m_cornerRadius,
            m_titleLabel ? m_titleLabel->text() : QString(), width(), height());
        connect(m_settingsWindow, &SettingsWindow::sigmaChanged, this,
                &FrostedGlassWindow::setSigma);
        connect(m_settingsWindow, &SettingsWindow::offsetXChanged, this,
                &FrostedGlassWindow::setOffsetX);
        connect(m_settingsWindow, &SettingsWindow::offsetYChanged, this,
                &FrostedGlassWindow::setOffsetY);
        connect(m_settingsWindow, &SettingsWindow::cornerRadiusChanged, this,
                &FrostedGlassWindow::setCornerRadius);
        connect(m_settingsWindow, &SettingsWindow::windowTitleChanged, this,
                &FrostedGlassWindow::setWindowTitleText);
        connect(m_settingsWindow, &SettingsWindow::windowSizeChanged, this,
                &FrostedGlassWindow::setWindowSize);
        connect(m_settingsWindow, &QObject::destroyed, this,
                [this]() { m_settingsWindow = nullptr; });
    } else {
        m_settingsWindow->close();
        delete m_settingsWindow;
        m_settingsWindow = new SettingsWindow(
            static_cast<int>(m_sigma), m_offsetX, m_offsetY, m_cornerRadius,
            m_titleLabel ? m_titleLabel->text() : QString(), width(), height());
        connect(m_settingsWindow, &SettingsWindow::sigmaChanged, this,
                &FrostedGlassWindow::setSigma);
        connect(m_settingsWindow, &SettingsWindow::offsetXChanged, this,
                &FrostedGlassWindow::setOffsetX);
        connect(m_settingsWindow, &SettingsWindow::offsetYChanged, this,
                &FrostedGlassWindow::setOffsetY);
        connect(m_settingsWindow, &SettingsWindow::cornerRadiusChanged, this,
                &FrostedGlassWindow::setCornerRadius);
        connect(m_settingsWindow, &SettingsWindow::windowTitleChanged, this,
                &FrostedGlassWindow::setWindowTitleText);
        connect(m_settingsWindow, &SettingsWindow::windowSizeChanged, this,
                &FrostedGlassWindow::setWindowSize);
        connect(m_settingsWindow, &QObject::destroyed, this,
                [this]() { m_settingsWindow = nullptr; });
    }
    m_settingsWindow->show();
    m_settingsWindow->raise();
    m_settingsWindow->activateWindow();
}

void FrostedGlassWindow::mousePressEvent(QMouseEvent *event) {
    QRect tRect(0, 0, width(), 40);
    if (tRect.contains(event->pos())) {
        m_dragging = true;
        m_keyboardMoving = true;
        m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
        grabKeyboard(); // 抓取键盘，用于方向键微调
        setFocus();
    }
}

void FrostedGlassWindow::mouseMoveEvent(QMouseEvent *event) {
    if (m_dragging) {
        move(event->globalPos() - m_dragStartPos);
    }
}

void FrostedGlassWindow::mouseReleaseEvent(QMouseEvent *) {
    m_dragging = false;
    if (m_keyboardMoving) {
        m_keyboardMoving = false;
        releaseKeyboard();
        snapToGrid(); // 释放鼠标后吸附网格
    }
}

void FrostedGlassWindow::keyPressEvent(QKeyEvent *event) {
    if (m_keyboardMoving) {
        QPoint delta(0, 0);
        switch (event->key()) {
        case Qt::Key_Left:
            delta.setX(-1);
            break;
        case Qt::Key_Right:
            delta.setX(1);
            break;
        case Qt::Key_Up:
            delta.setY(-1);
            break;
        case Qt::Key_Down:
            delta.setY(1);
            break;
        default:
            QWidget::keyPressEvent(event);
            return;
        }
        move(pos() + delta);
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void FrostedGlassWindow::keyReleaseEvent(QKeyEvent *event) {
    if (m_keyboardMoving &&
        (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right ||
                             event->key() == Qt::Key_Up || event->key() == Qt::Key_Down)) {
        snapToGrid(); // 每次微调结束后吸附
        event->accept();
        return;
    }
    QWidget::keyReleaseEvent(event);
}

void FrostedGlassWindow::changeEvent(QEvent *event) {
    if (event->type() == QEvent::WindowStateChange) {
        updateBlurBackground();
    }
    QWidget::changeEvent(event);
}

void FrostedGlassWindow::closeEvent(QCloseEvent *event) {
    if (m_settingsWindow) {
        m_settingsWindow->close();
        m_settingsWindow = nullptr;
    }
    event->accept();
}

#ifdef Q_OS_WIN
bool FrostedGlassWindow::nativeEvent(const QByteArray &eventType, void *message,
                                     long long *result) {
    Q_UNUSED(eventType);
    Q_UNUSED(message);
    Q_UNUSED(result);
    return false; // 禁用默认边框调整
}
#endif