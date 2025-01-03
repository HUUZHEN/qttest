#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTimer>
#include <QVector>
#include <QChartView>
#include <QLineSeries>
#include <QProgressBar>
#include <QChart>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updatePerformance();
    void updateCpuUsage();
    void updateMemoryUsage();
    void stopMonitoring();
    void setUpChart();
    void updateGpuUsage();

private:
    double extractCpuUsage(const QString &text);
    double extractMemoryUsage(const QString &text);

    QLabel *cpuLabel;
    QLabel *hourlyCpuLabel;
    QLabel *memoryLabel;
    QLabel *hourlyMemoryLabel;
    QPushButton *stopButton;
    QVBoxLayout *layout;
    QTimer *timer;

    QVector<double> cpuUsageHistory;
    QVector<double> memoryUsageHistory;

    QLabel *gpuUsageLabel;
    QProgressBar *cpuProgressBar;
    QProgressBar *memoryProgressBar;
    QChart *chart;
    QChartView *chartView;
    QLineSeries *cpuSeries;
    QLineSeries *memorySeries;

    static const int maxHistorySize = 3600; // 1 小時的數據 (每秒更新)
};

#endif // MAINWINDOW_H
