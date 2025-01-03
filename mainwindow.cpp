#include "mainwindow.h"
#include <QPainter>
#include <QMessageBox>
#include <windows.h>
#include <numeric>
#include <QDebug>
#include <QApplication>
#include <QRegularExpression>
#include <QProgressBar>
#include <QChartView>
#include <QLineSeries>
#include <QValueAxis>
#include <QProcess>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    timer(new QTimer(this)),
    gpuUsageLabel(new QLabel("GPU Usage: N/A", this)),
    cpuProgressBar(new QProgressBar(this)),
    memoryProgressBar(new QProgressBar(this))
{
    // 創建控件
    cpuLabel = new QLabel("CPU Usage: N/A", this);
    hourlyCpuLabel = new QLabel("Hourly Avg CPU: N/A", this);
    memoryLabel = new QLabel("Memory Usage: N/A", this);
    hourlyMemoryLabel = new QLabel("Hourly Avg Memory: N/A", this);
    stopButton = new QPushButton("Stop Monitoring", this);

    // 設置控件樣式
    cpuProgressBar->setRange(0, 100);
    cpuProgressBar->setValue(0);
    memoryProgressBar->setRange(0, 100);
    memoryProgressBar->setValue(0);

    // 設置佈局
    layout = new QVBoxLayout;
    layout->addWidget(cpuLabel);
    layout->addWidget(cpuProgressBar);
    layout->addWidget(hourlyCpuLabel);
    layout->addWidget(memoryLabel);
    layout->addWidget(memoryProgressBar);
    layout->addWidget(hourlyMemoryLabel);
    layout->addWidget(gpuUsageLabel);
    layout->addWidget(stopButton);

    // 設置UI佈局
    QWidget *centralWidget = new QWidget(this);
    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);

    // 設置視窗為半透明
    setWindowOpacity(0.8);  // 設置視窗透明度
    setAttribute(Qt::WA_TranslucentBackground);  // 使背景透明

    // 初始化窗口
    resize(500, 500);
    setWindowTitle("Performance Monitor");

    // 初始化定時器
    connect(timer, &QTimer::timeout, this, &MainWindow::updatePerformance);
    timer->start(1000); // 每秒更新一次

    // 初始化歷史數據
    cpuUsageHistory.reserve(maxHistorySize);
    memoryUsageHistory.reserve(maxHistorySize);

    // 連接按鈕信號到槽函數
    connect(stopButton, &QPushButton::clicked, this, &MainWindow::stopMonitoring);

    // 設置圖表
    setUpChart();
}

MainWindow::~MainWindow()
{
    // 不需要手動釋放子控件，因為 Qt 會自動管理它們的生命週期
}

void MainWindow::updatePerformance()
{
    updateCpuUsage();
    updateMemoryUsage();
    updateGpuUsage();

    // 保持 CPU 和記憶體歷史數據在 1 小時內
    if (cpuUsageHistory.size() >= maxHistorySize) cpuUsageHistory.pop_front();
    if (memoryUsageHistory.size() >= maxHistorySize) memoryUsageHistory.pop_front();

    // 提取當前的 CPU 和記憶體使用率
    double cpuCurrent = extractCpuUsage(cpuLabel->text());
    double memoryCurrent = extractMemoryUsage(memoryLabel->text());

    // 添加到歷史數據
    cpuUsageHistory.append(cpuCurrent);
    memoryUsageHistory.append(memoryCurrent);

    // 計算平均值
    if (!cpuUsageHistory.isEmpty()) {
        double cpuHourlyAverage = std::accumulate(cpuUsageHistory.begin(), cpuUsageHistory.end(), 0.0) / cpuUsageHistory.size();
        hourlyCpuLabel->setText(QString("Hourly Avg CPU: %1%").arg(cpuHourlyAverage, 0, 'f', 2));
    }

    if (!memoryUsageHistory.isEmpty()) {
        double memoryHourlyAverage = std::accumulate(memoryUsageHistory.begin(), memoryUsageHistory.end(), 0.0) / memoryUsageHistory.size();
        hourlyMemoryLabel->setText(QString("Hourly Avg Memory: %1%").arg(memoryHourlyAverage, 0, 'f', 2));
    }

    // 更新進度條
    cpuProgressBar->setValue(cpuCurrent);
    memoryProgressBar->setValue(memoryCurrent);

    // 更新圖表數據
    cpuSeries->append(cpuUsageHistory.size(), cpuCurrent);
    memorySeries->append(memoryUsageHistory.size(), memoryCurrent);
}

void MainWindow::updateCpuUsage()
{
    static FILETIME prevIdleTime, prevKernelTime, prevUserTime;

    FILETIME idleTime, kernelTime, userTime;
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        ULARGE_INTEGER idle, kernel, user, prevIdle, prevKernel, prevUser;

        idle.LowPart = idleTime.dwLowDateTime;
        idle.HighPart = idleTime.dwHighDateTime;

        kernel.LowPart = kernelTime.dwLowDateTime;
        kernel.HighPart = kernelTime.dwHighDateTime;

        user.LowPart = userTime.dwLowDateTime;
        user.HighPart = userTime.dwHighDateTime;

        prevIdle.LowPart = prevIdleTime.dwLowDateTime;
        prevIdle.HighPart = prevIdleTime.dwHighDateTime;

        prevKernel.LowPart = prevKernelTime.dwLowDateTime;
        prevKernel.HighPart = prevKernelTime.dwHighDateTime;

        prevUser.LowPart = prevUserTime.dwLowDateTime;
        prevUser.HighPart = prevUserTime.dwHighDateTime;

        ULONGLONG idleDiff = idle.QuadPart - prevIdle.QuadPart;
        ULONGLONG totalDiff = (kernel.QuadPart + user.QuadPart) - (prevKernel.QuadPart + prevUser.QuadPart);

        double cpuUsage = totalDiff ? (100.0 * (1.0 - static_cast<double>(idleDiff) / totalDiff)) : 0.0;

        cpuLabel->setText(QString("CPU Usage: %1%").arg(cpuUsage, 0, 'f', 2));

        prevIdleTime = idleTime;
        prevKernelTime = kernelTime;
        prevUserTime = userTime;
    } else {
        cpuLabel->setText("CPU Usage: Error");
    }
}

void MainWindow::updateMemoryUsage()
{
    MEMORYSTATUSEX memoryStatus;
    memoryStatus.dwLength = sizeof(MEMORYSTATUSEX);

    if (GlobalMemoryStatusEx(&memoryStatus)) {
        qint64 totalMemory = memoryStatus.ullTotalPhys / (1024 * 1024); // MB
        qint64 usedMemory = (memoryStatus.ullTotalPhys - memoryStatus.ullAvailPhys) / (1024 * 1024); // MB
        double memoryUsage = memoryStatus.dwMemoryLoad; // 百分比

        memoryLabel->setText(QString("Memory Usage: %1% (%2 MB used of %3 MB)")
                                 .arg(memoryUsage, 0, 'f', 2)
                                 .arg(usedMemory)
                                 .arg(totalMemory));
    } else {
        memoryLabel->setText("Memory Usage: Error");
    }
}

void MainWindow::updateGpuUsage()
{
    // 使用 nvidia-smi 命令來獲取 GPU 使用率
    QProcess process;
    process.start("nvidia-smi --query-gpu=utilization.gpu --format=csv,noheader,nounits");
    process.waitForFinished();
    QString gpuUsageStr = process.readAllStandardOutput().trimmed();

    bool ok;
    int gpuUsage = gpuUsageStr.toInt(&ok);
    if (ok) {
        gpuUsageLabel->setText(QString("GPU Usage: %1%").arg(gpuUsage));
    } else {
        gpuUsageLabel->setText("GPU Usage: Error");
    }
}

void MainWindow::stopMonitoring()
{
    timer->stop();
    QMessageBox::information(this, "Stopped Monitoring", "Monitoring has been stopped.");
    QApplication::quit();
}

double MainWindow::extractCpuUsage(const QString &text)
{
    QRegularExpression regex("CPU Usage: ([\\d\\.]+)%");
    QRegularExpressionMatch match = regex.match(text);
    if (match.hasMatch()) {
        return match.captured(1).toDouble();
    }
    return 0.0; // 如果解析失敗，返回 0
}

double MainWindow::extractMemoryUsage(const QString &text)
{
    QRegularExpression regex("Memory Usage: ([\\d\\.]+)%");
    QRegularExpressionMatch match = regex.match(text);
    if (match.hasMatch()) {
        return match.captured(1).toDouble();
    }
    return 0.0; // 如果解析失敗，返回 0
}

void MainWindow::setUpChart()
{
    // 設置圖表來顯示 CPU 和 Memory 的歷史數據
    cpuSeries = new QLineSeries();
    memorySeries = new QLineSeries();

    chart = new QChart();
    chart->addSeries(cpuSeries);
    chart->addSeries(memorySeries);
    chart->createDefaultAxes();

    // 設置 X 軸範圍
    QValueAxis *xAxis = new QValueAxis();
    xAxis->setRange(0, maxHistorySize); // 根據 maxHistorySize 設置範圍
    chart->setAxisX(xAxis, cpuSeries);
    chart->setAxisX(xAxis, memorySeries);

    // 設置 Y 軸範圍
    QValueAxis *yAxis = new QValueAxis();
    yAxis->setRange(0, 100); // 假設最大使用率是 100%
    chart->setAxisY(yAxis, cpuSeries);
    chart->setAxisY(yAxis, memorySeries);

    chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    layout->addWidget(chartView);
}
