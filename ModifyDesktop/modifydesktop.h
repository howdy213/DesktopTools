#include <QApplication>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QImage>
#include <QKeyEvent>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPushButton>
#include <QScreen>
#include <QSettings>
#include <QSlider>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QWidget>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

class ModifyDesktop : public QWidget {
    Q_OBJECT

public:
    ModifyDesktop(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onScaleXChanged(double value);
    void onScaleYChanged(double value);
    void onResetClicked();

private:
    // 重置为等比覆盖全屏并居中
    void resetToCover();

    // 获取当前桌面壁纸路径
    QString getCurrentWallpaperPath();
    bool loadWallpaper(const QString &path);

    // 等比缩放时保持鼠标点固定
    void keepPointFixed(const QPoint &mousePos, double factor);
    void keepPointFixedHorizontal(const QPoint &mousePos, double factor);
    void keepPointFixedVertical(const QPoint &mousePos, double factor);

    // 捕获当前视图（不含UI控件）
    QImage captureCurrentView();
    void saveCurrentView();
    void setupControlPanel();
    void updateSpinBoxes();
    void resizeEventForPanel();

private:
    QPixmap originalPixmap;
    double scaleX = 1.0, scaleY = 1.0;
    double offsetX = 0.0, offsetY = 0.0;

    bool isDragging = false;
    QPoint lastMousePos;

    QFrame *controlPanel = nullptr;
    QDoubleSpinBox *scaleXSpinBox = nullptr;
    QDoubleSpinBox *scaleYSpinBox = nullptr;
};
