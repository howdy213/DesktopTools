#include "modifydesktop.h"
#include <QWindow>
#include <dwmapi.h>

ModifyDesktop::ModifyDesktop(QWidget *parent) : QWidget(parent) {
    // 设置窗口属性：无边框、全屏、保持焦点
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_TranslucentBackground, false);
    showFullScreen();

    // 获取当前桌面壁纸路径并加载
    QString wallpaperPath = getCurrentWallpaperPath();
    if (!loadWallpaper(wallpaperPath)) {
        // 如果获取失败，使用默认渐变或颜色提示
        originalPixmap = QPixmap(1920, 1080);
        originalPixmap.fill(Qt::darkGray);
        QPainter painter(&originalPixmap);
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter,
                         "无法读取壁纸\n请检查系统或手动指定图片");
    }

    // 初始化变换参数：等比覆盖全屏并居中（取大缩放因子）
    resetToCover();

    // 创建浮动控制面板
    setupControlPanel();

    // 设置鼠标追踪，以便更好的交互
    setMouseTracking(true);

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
}

void ModifyDesktop::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    // 绘制背景（黑色，作为超出图片部分的填充）
    painter.fillRect(rect(), Qt::black);

    // 绘制壁纸，根据当前变换参数
    if (!originalPixmap.isNull()) {
        // 计算目标矩形
        double targetWidth = originalPixmap.width() * scaleX;
        double targetHeight = originalPixmap.height() * scaleY;
        QRectF targetRect(offsetX, offsetY, targetWidth, targetHeight);
        painter.drawPixmap(targetRect, originalPixmap, originalPixmap.rect());
    }
    if (controlPanel->isVisible()) {
    // 绘制简单的提示文字
    painter.setPen(Qt::white);
    painter.setFont(QFont("Microsoft YaHei", 10));
    painter.drawText(rect().adjusted(20, 20, -20, -20),
                     Qt::AlignTop | Qt::AlignRight,
                     "鼠标拖拽移动图片\n"
                     "滚轮：等比缩放\n"
                     "Ctrl+滚轮：水平拉伸\n"
                     "Shift+滚轮：垂直拉伸\n"
                     "方向键：微调位置\n"
                     "Ctrl+S：保存当前视图\n"
                     "ESC：退出");
    }
}

void ModifyDesktop::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        lastMousePos = event->pos();
        isDragging = true;
    }
}

void ModifyDesktop::mouseMoveEvent(QMouseEvent *event) {
    if (isDragging && (event->buttons() & Qt::LeftButton)) {
        QPoint delta = event->pos() - lastMousePos;
        offsetX += delta.x();
        offsetY += delta.y();
        lastMousePos = event->pos();
        update();
        updateSpinBoxes();
    }
}

void ModifyDesktop::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        isDragging = false;
    }
}

void ModifyDesktop::wheelEvent(QWheelEvent *event) {
    double factor = (event->angleDelta().y() > 0) ? 1.1 : 0.9;
    Qt::KeyboardModifiers modifiers = event->modifiers();

    if (modifiers & Qt::ControlModifier && modifiers & Qt::ShiftModifier) {
        // 同时按Ctrl+Shift则等比例缩放
        scaleX *= factor;
        scaleY *= factor;
        keepPointFixed(event->position().toPoint(), factor);
    } else if (modifiers & Qt::ControlModifier) {
        // Ctrl + 滚轮: 水平拉伸
        double oldScaleX = scaleX;
        scaleX *= factor;
        keepPointFixedHorizontal(event->position().toPoint(), factor);
    } else if (modifiers & Qt::ShiftModifier) {
        // Shift + 滚轮: 垂直拉伸
        double oldScaleY = scaleY;
        scaleY *= factor;
        keepPointFixedVertical(event->position().toPoint(), factor);
    } else {
        // 无修饰键：等比缩放
        scaleX *= factor;
        scaleY *= factor;
        keepPointFixed(event->position().toPoint(), factor);
    }

    // 限制缩放范围
    scaleX = qBound(0.1, scaleX, 10.0);
    scaleY = qBound(0.1, scaleY, 10.0);

    update();
    updateSpinBoxes();
}

void ModifyDesktop::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        close();
    } else if (event->key() == Qt::Key_S &&
               event->modifiers() == Qt::ControlModifier) {
        saveCurrentView();
    } else if (event->key() == Qt::Key_H &&
               event->modifiers() == Qt::ControlModifier) {
        controlPanel->setVisible(!controlPanel->isVisible());
        update();
    }

    else {
        // 方向键移动，步长为窗口宽高的2%（最小5像素）
        int stepX = 1;
        int stepY = 1;
        switch (event->key()) {
        case Qt::Key_Left:
            offsetX -= stepX;
            update();
            break;
        case Qt::Key_Right:
            offsetX += stepX;
            update();
            break;
        case Qt::Key_Up:
            offsetY -= stepY;
            update();
            break;
        case Qt::Key_Down:
            offsetY += stepY;
            update();
            break;
        default:
            QWidget::keyPressEvent(event);
            return;
        }
        updateSpinBoxes();
        event->accept();
    }
}

void ModifyDesktop::onScaleXChanged(double value) {
    scaleX = value;
    update();
}

void ModifyDesktop::onScaleYChanged(double value) {
    scaleY = value;
    update();
}

void ModifyDesktop::onResetClicked() {
    resetToCover();
    update();
    updateSpinBoxes();
}

void ModifyDesktop::resetToCover() {
    if (originalPixmap.isNull()) {
        scaleX = scaleY = 1.0;
        offsetX = offsetY = 0.0;
        return;
    }
    QSize winSize = size();
    double ratioW = (double)winSize.width() / originalPixmap.width();
    double ratioH = (double)winSize.height() / originalPixmap.height();
    double scale = qMax(ratioW, ratioH); // 取大缩放因子，确保完全覆盖窗口
    scaleX = scaleY = scale;
    // 居中计算
    double scaledW = originalPixmap.width() * scale;
    double scaledH = originalPixmap.height() * scale;
    offsetX = (winSize.width() - scaledW) / 2.0;
    offsetY = (winSize.height() - scaledH) / 2.0;
}

QString ModifyDesktop::getCurrentWallpaperPath() {
#ifdef Q_OS_WIN
    QSettings settings("HKEY_CURRENT_USER\\Control Panel\\Desktop",
                       QSettings::NativeFormat);
    QString path = settings.value("Wallpaper").toString();
    if (!path.isEmpty() && QFile::exists(path)) {
        return path;
    }
#endif
    return QString();
}

bool ModifyDesktop::loadWallpaper(const QString &path) {
    if (path.isEmpty())
        return false;
    if (!originalPixmap.load(path))
        return false;
    return !originalPixmap.isNull();
}

void ModifyDesktop::keepPointFixed(const QPoint &mousePos, double factor) {
    double origX = (mousePos.x() - offsetX) / (scaleX / factor);
    double origY = (mousePos.y() - offsetY) / (scaleY / factor);
    offsetX = mousePos.x() - origX * scaleX;
    offsetY = mousePos.y() - origY * scaleY;
}

void ModifyDesktop::keepPointFixedHorizontal(const QPoint &mousePos,
                                             double factor) {
    double origX = (mousePos.x() - offsetX) / (scaleX / factor);
    offsetX = mousePos.x() - origX * scaleX;
    // 垂直方向不动
}

void ModifyDesktop::keepPointFixedVertical(const QPoint &mousePos,
                                           double factor) {
    double origY = (mousePos.y() - offsetY) / (scaleY / factor);
    offsetY = mousePos.y() - origY * scaleY;
}

QImage ModifyDesktop::captureCurrentView() {
    QSize winSize = size();
    QImage result(winSize, QImage::Format_ARGB32_Premultiplied);
    result.fill(Qt::black);

    if (originalPixmap.isNull()) {
        return result;
    }

    QPainter painter(&result);
    double targetWidth = originalPixmap.width() * scaleX;
    double targetHeight = originalPixmap.height() * scaleY;
    QRectF targetRect(offsetX, offsetY, targetWidth, targetHeight);
    painter.drawPixmap(targetRect, originalPixmap, originalPixmap.rect());
    return result;
}

void ModifyDesktop::saveCurrentView() {
    QImage view = captureCurrentView();
    if (view.isNull()) {
        QMessageBox::warning(this, "错误", "无法捕获当前视图");
        return;
    }

    QString defaultName = "wallpaper_cropped.png";
    QString filePath = QFileDialog::getSaveFileName(
        this, "保存裁剪后的壁纸", defaultName, "图片文件 (*.png *.jpg *.bmp)");
    if (filePath.isEmpty())
        return;

    if (view.save(filePath)) {
        QMessageBox::information(this, "成功", "图片已保存到:\n" + filePath);
    } else {
        QMessageBox::warning(this, "错误", "保存图片失败");
    }
}

void ModifyDesktop::setupControlPanel() {
    controlPanel = new QFrame(this);
    controlPanel->setFrameShape(QFrame::StyledPanel);
    controlPanel->setStyleSheet("QFrame { background-color: rgba(40,40,40,180); "
                                "border-radius: 8px; color: white; }");
    controlPanel->setFixedWidth(260);

    QVBoxLayout *layout = new QVBoxLayout(controlPanel);

    QLabel *titleLabel = new QLabel("调整控制");
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    layout->addWidget(titleLabel);

    // 水平缩放控制
    QHBoxLayout *scaleXLayout = new QHBoxLayout();
    QLabel *scaleXLabel = new QLabel("水平缩放:");
    scaleXSpinBox = new QDoubleSpinBox();
    scaleXSpinBox->setRange(0.1, 10.0);
    scaleXSpinBox->setSingleStep(0.05);
    scaleXSpinBox->setDecimals(2);
    scaleXSpinBox->setValue(scaleX);
    scaleXSpinBox->setStyleSheet(
        "background-color: rgba(0,0,0,150); color: white;");
    connect(scaleXSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ModifyDesktop::onScaleXChanged);
    scaleXLayout->addWidget(scaleXLabel);
    scaleXLayout->addWidget(scaleXSpinBox);
    layout->addLayout(scaleXLayout);

    // 垂直缩放控制
    QHBoxLayout *scaleYLayout = new QHBoxLayout();
    QLabel *scaleYLabel = new QLabel("垂直缩放:");
    scaleYSpinBox = new QDoubleSpinBox();
    scaleYSpinBox->setRange(0.1, 10.0);
    scaleYSpinBox->setSingleStep(0.05);
    scaleYSpinBox->setDecimals(2);
    scaleYSpinBox->setValue(scaleY);
    scaleYSpinBox->setStyleSheet(
        "background-color: rgba(0,0,0,150); color: white;");
    connect(scaleYSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ModifyDesktop::onScaleYChanged);
    scaleYLayout->addWidget(scaleYLabel);
    scaleYLayout->addWidget(scaleYSpinBox);
    layout->addLayout(scaleYLayout);

    // 重置按钮
    QPushButton *resetBtn = new QPushButton("重置（居中覆盖）");
    resetBtn->setStyleSheet(
        "QPushButton { background-color: #4CAF50; border-radius: 4px; padding: "
        "5px; } QPushButton:hover { background-color: #45a049; }");
    connect(resetBtn, &QPushButton::clicked, this,
            &ModifyDesktop::onResetClicked);
    layout->addWidget(resetBtn);

    controlPanel->move(20, 20);
    controlPanel->show();
}

void ModifyDesktop::updateSpinBoxes() {
    if (scaleXSpinBox)
        scaleXSpinBox->blockSignals(true);
    if (scaleYSpinBox)
        scaleYSpinBox->blockSignals(true);
    if (scaleXSpinBox)
        scaleXSpinBox->setValue(scaleX);
    if (scaleYSpinBox)
        scaleYSpinBox->setValue(scaleY);
    if (scaleXSpinBox)
        scaleXSpinBox->blockSignals(false);
    if (scaleYSpinBox)
        scaleYSpinBox->blockSignals(false);
}

void ModifyDesktop::resizeEventForPanel() {
    if (controlPanel) {
        controlPanel->move(20, 20);
    }
}

void ModifyDesktop::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    resizeEventForPanel();
    // 窗口大小改变时不自动重置变换，保持用户调整的状态
}
