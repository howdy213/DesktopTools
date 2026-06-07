#include "SettingsWindow.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

SettingsWindow::SettingsWindow(int initSigma, int initOffsetX, int initOffsetY,
                               int initCornerRadius, const QString &initTitle,
                               int initWidth, int initHeight, QWidget *parent)
    : QWidget(parent, Qt::Window | Qt::WindowTitleHint |
                          Qt::WindowCloseButtonHint |
                          Qt::WindowStaysOnTopHint) {
    setWindowTitle(QStringLiteral("设置 - %1").arg(initTitle));
    setFixedSize(380, 400);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 模糊强度
    QHBoxLayout *row1 = new QHBoxLayout();
    QLabel *labelSigma = new QLabel(QStringLiteral("模糊强度"));
    m_sigmaSlider = new QSlider(Qt::Horizontal);
    m_sigmaSlider->setRange(3, 100);
    m_sigmaSlider->setValue(initSigma);
    m_sigmaValueLabel = new QLabel(QString::number(initSigma));
    m_sigmaValueLabel->setFixedWidth(30);
    row1->addWidget(labelSigma);
    row1->addWidget(m_sigmaSlider);
    row1->addWidget(m_sigmaValueLabel);
    mainLayout->addLayout(row1);

    // 偏移 X
    QHBoxLayout *row2 = new QHBoxLayout();
    QLabel *labelX = new QLabel(QStringLiteral("偏移 X"));
    m_offsetXSlider = new QSlider(Qt::Horizontal);
    m_offsetXSlider->setRange(-100, 100);
    m_offsetXSlider->setValue(initOffsetX);
    m_offsetXValueLabel = new QLabel(QString::number(initOffsetX));
    m_offsetXValueLabel->setFixedWidth(30);
    row2->addWidget(labelX);
    row2->addWidget(m_offsetXSlider);
    row2->addWidget(m_offsetXValueLabel);
    mainLayout->addLayout(row2);

    // 偏移 Y
    QHBoxLayout *row3 = new QHBoxLayout();
    QLabel *labelY = new QLabel(QStringLiteral("偏移 Y"));
    m_offsetYSlider = new QSlider(Qt::Horizontal);
    m_offsetYSlider->setRange(-100, 100);
    m_offsetYSlider->setValue(initOffsetY);
    m_offsetYValueLabel = new QLabel(QString::number(initOffsetY));
    m_offsetYValueLabel->setFixedWidth(30);
    row3->addWidget(labelY);
    row3->addWidget(m_offsetYSlider);
    row3->addWidget(m_offsetYValueLabel);
    mainLayout->addLayout(row3);

    // 圆角半径
    QHBoxLayout *row4 = new QHBoxLayout();
    QLabel *labelRadius = new QLabel(QStringLiteral("圆角半径"));
    m_cornerRadiusSlider = new QSlider(Qt::Horizontal);
    m_cornerRadiusSlider->setRange(0, 50);
    m_cornerRadiusSlider->setValue(initCornerRadius);
    m_cornerRadiusValueLabel = new QLabel(QString::number(initCornerRadius));
    m_cornerRadiusValueLabel->setFixedWidth(30);
    row4->addWidget(labelRadius);
    row4->addWidget(m_cornerRadiusSlider);
    row4->addWidget(m_cornerRadiusValueLabel);
    mainLayout->addLayout(row4);

    // 窗口标题
    QHBoxLayout *row5 = new QHBoxLayout();
    QLabel *labelTitle = new QLabel(QStringLiteral("窗口标题"));
    m_titleEdit = new QLineEdit(initTitle);
    row5->addWidget(labelTitle);
    row5->addWidget(m_titleEdit);
    mainLayout->addLayout(row5);

    // 窗口尺寸（范围50~1000）
    QHBoxLayout *row6 = new QHBoxLayout();
    QLabel *labelWidth = new QLabel(QStringLiteral("宽度"));
    m_widthSpin = new QSpinBox();
    m_widthSpin->setRange(50, 1000);
    m_widthSpin->setValue(initWidth);
    QLabel *labelHeight = new QLabel(QStringLiteral("高度"));
    m_heightSpin = new QSpinBox();
    m_heightSpin->setRange(50, 1000);
    m_heightSpin->setValue(initHeight);
    row6->addWidget(labelWidth);
    row6->addWidget(m_widthSpin);
    row6->addWidget(labelHeight);
    row6->addWidget(m_heightSpin);
    mainLayout->addLayout(row6);

    // 信号连接
    connect(m_sigmaSlider, &QSlider::valueChanged, this, [this](int v) {
        m_sigmaValueLabel->setText(QString::number(v));
        emit sigmaChanged(static_cast<float>(v));
    });
    connect(m_offsetXSlider, &QSlider::valueChanged, this, [this](int v) {
        m_offsetXValueLabel->setText(QString::number(v));
        emit offsetXChanged(v);
    });
    connect(m_offsetYSlider, &QSlider::valueChanged, this, [this](int v) {
        m_offsetYValueLabel->setText(QString::number(v));
        emit offsetYChanged(v);
    });
    connect(m_cornerRadiusSlider, &QSlider::valueChanged, this, [this](int v) {
        m_cornerRadiusValueLabel->setText(QString::number(v));
        emit cornerRadiusChanged(v);
    });
    connect(m_titleEdit, &QLineEdit::textChanged, this,
            &SettingsWindow::windowTitleChanged);
    connect(m_widthSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this](int w) { emit windowSizeChanged(w, m_heightSpin->value()); });
    connect(m_heightSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this](int h) { emit windowSizeChanged(m_widthSpin->value(), h); });

    QPushButton *btnClose = new QPushButton(QStringLiteral("关闭"));
    connect(btnClose, &QPushButton::clicked, this, &QWidget::close);
    mainLayout->addWidget(btnClose, 0, Qt::AlignRight);
}

void SettingsWindow::updateTitle(const QString &title) {
    setWindowTitle(QStringLiteral("设置 - %1").arg(title));
}