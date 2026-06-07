#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void saveLayout();
    void restoreLayout();

private:
    QPushButton *m_saveBtn;
    QPushButton *m_restoreBtn;
};

#endif // MAINWINDOW_H