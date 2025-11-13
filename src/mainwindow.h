#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QImage>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QSlider>
#include <QProgressBar>
#include <QElapsedTimer>
#include <QTextEdit>
#include <vector>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(int mpiSize, QWidget *parent = nullptr);
    ~MainWindow();
    
    static void workerLoop(int rank);

private slots:
    void loadImage();
    void processImage();
    void saveImage();
    void updateKernelSize(int value);
    void updateThreadCount(int index);

private:
    void setupUI();
    void updateImageDisplay();
    void logMessage(const QString &msg);
    
    // Image processing 
    QImage applyGaussianBlur(const QImage &img, int kernelSize, int threads);
    QImage applyEdgeDetection(const QImage &img, int threads);
    QImage applySharpening(const QImage &img, int threads);
    
    // MPI communication
    void distributeImageMPI(const QImage &img);
    QImage gatherImageMPI();
    
    // UI 
    QLabel *originalLabel;
    QLabel *processedLabel;
    QLabel *statsLabel;
    QPushButton *loadBtn;
    QPushButton *processBtn;
    QPushButton *saveBtn;
    QComboBox *filterCombo;
    QComboBox *threadCombo;
    QSlider *kernelSlider;
    QProgressBar *progressBar;
    QTextEdit *logText;
    
    // Data
    QImage originalImage;
    QImage processedImage;
    int mpiProcessCount;
    QElapsedTimer timer;
};
#endif // MAINWINDOW_H
