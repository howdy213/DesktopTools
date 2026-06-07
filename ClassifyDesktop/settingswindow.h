#pragma once
#include <QLabel>
#include <QLineEdit>
#include <QSlider>
#include <QSpinBox>
#include <QWidget>

class SettingsWindow : public QWidget {
    Q_OBJECT
public:
    explicit SettingsWindow(int initSigma, int initOffsetX, int initOffsetY,
                            int initCornerRadius, const QString &initTitle,
                            int initWidth, int initHeight,
                            QWidget *parent = nullptr);

signals:
    void sigmaChanged(float value);
    void offsetXChanged(int value);
    void offsetYChanged(int value);
    void cornerRadiusChanged(int value);
    void windowTitleChanged(const QString &title);
    void windowSizeChanged(int width, int height);

public slots:
    void updateTitle(const QString &title);

private:
    QSlider *m_sigmaSlider;
    QSlider *m_offsetXSlider;
    QSlider *m_offsetYSlider;
    QSlider *m_cornerRadiusSlider;

    QLabel *m_sigmaValueLabel;
    QLabel *m_offsetXValueLabel;
    QLabel *m_offsetYValueLabel;
    QLabel *m_cornerRadiusValueLabel;

    QLineEdit *m_titleEdit;
    QSpinBox *m_widthSpin;
    QSpinBox *m_heightSpin;
};