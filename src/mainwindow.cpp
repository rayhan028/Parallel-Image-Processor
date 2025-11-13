#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <omp.h>
#include <mpi.h>
#include <cmath>

#define TAG_IMAGE_DATA 1
#define TAG_IMAGE_ROWS 2
#define TAG_IMAGE_COLS 3
#define TAG_RESULT 4

MainWindow::MainWindow(int mpiSize, QWidget *parent)
    : QMainWindow(parent), mpiProcessCount(mpiSize)
{
    setupUI();
    logMessage(QString("Application started with %1 MPI processes").arg(mpiSize));
}

MainWindow::~MainWindow() {}

void MainWindow::setupUI()
{
    setWindowTitle("Parallel Image Processor (Qt + OpenMP + MPI)");
    resize(1400, 800);
    
    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    
    QHBoxLayout *mainLayout = new QHBoxLayout(central);
    
    //  Controls
    QVBoxLayout *leftLayout = new QVBoxLayout();
    
    QGroupBox *fileGroup = new QGroupBox("File Operations");
    QVBoxLayout *fileLayout = new QVBoxLayout(fileGroup);
    loadBtn = new QPushButton("Load Image");
    saveBtn = new QPushButton("Save Result");
    saveBtn->setEnabled(false);
    fileLayout->addWidget(loadBtn);
    fileLayout->addWidget(saveBtn);
    leftLayout->addWidget(fileGroup);
    
    QGroupBox *filterGroup = new QGroupBox("Filter Settings");
    QVBoxLayout *filterLayout = new QVBoxLayout(filterGroup);
    
    filterLayout->addWidget(new QLabel("Filter Type:"));
    filterCombo = new QComboBox();
    filterCombo->addItems({"Gaussian Blur", "Edge Detection", "Sharpen"});
    filterLayout->addWidget(filterCombo);
    
    filterLayout->addWidget(new QLabel("Kernel Size:"));
    kernelSlider = new QSlider(Qt::Horizontal);
    kernelSlider->setRange(3, 15);
    kernelSlider->setValue(5);
    kernelSlider->setSingleStep(2);
    QLabel *kernelLabel = new QLabel("5x5");
    filterLayout->addWidget(kernelSlider);
    filterLayout->addWidget(kernelLabel);
    
    filterLayout->addWidget(new QLabel("OpenMP Threads:"));
    threadCombo = new QComboBox();
    int maxThreads = omp_get_max_threads();
    for (int i = 1; i <= maxThreads; i *= 2) {
        threadCombo->addItem(QString::number(i));
    }
    filterLayout->addWidget(threadCombo);
    
    processBtn = new QPushButton("Process Image");
    processBtn->setEnabled(false);
    processBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; padding: 10px; }");
    filterLayout->addWidget(processBtn);
    
    leftLayout->addWidget(filterGroup);
    
    progressBar = new QProgressBar();
    progressBar->setVisible(false);
    leftLayout->addWidget(progressBar);
    
    statsLabel = new QLabel("No statistics yet");
    statsLabel->setWordWrap(true);
    statsLabel->setStyleSheet("QLabel { background-color: #f0f0f0; padding: 10px; border-radius: 5px; }");
    leftLayout->addWidget(statsLabel);
    
    QGroupBox *logGroup = new QGroupBox("Processing Log");
    QVBoxLayout *logLayout = new QVBoxLayout(logGroup);
    logText = new QTextEdit();
    logText->setReadOnly(true);
    logText->setMaximumHeight(200);
    logLayout->addWidget(logText);
    leftLayout->addWidget(logGroup);
    
    leftLayout->addStretch();
    
    //  Images
    QVBoxLayout *rightLayout = new QVBoxLayout();
    
    QLabel *origTitle = new QLabel("Original Image");
    origTitle->setAlignment(Qt::AlignCenter);
    origTitle->setStyleSheet("QLabel { font-size: 14pt; font-weight: bold; }");
    rightLayout->addWidget(origTitle);
    
    originalLabel = new QLabel();
    originalLabel->setMinimumSize(600, 400);
    originalLabel->setAlignment(Qt::AlignCenter);
    originalLabel->setStyleSheet("QLabel { border: 2px solid #ccc; background-color: white; }");
    originalLabel->setScaledContents(true);
    rightLayout->addWidget(originalLabel);
    
    QLabel *procTitle = new QLabel("Processed Image");
    procTitle->setAlignment(Qt::AlignCenter);
    procTitle->setStyleSheet("QLabel { font-size: 14pt; font-weight: bold; }");
    rightLayout->addWidget(procTitle);
    
    processedLabel = new QLabel();
    processedLabel->setMinimumSize(600, 400);
    processedLabel->setAlignment(Qt::AlignCenter);
    processedLabel->setStyleSheet("QLabel { border: 2px solid #ccc; background-color: white; }");
    processedLabel->setScaledContents(true);
    rightLayout->addWidget(processedLabel);
    
    mainLayout->addLayout(leftLayout, 1);
    mainLayout->addLayout(rightLayout, 2);
    
    // Connections
    connect(loadBtn, &QPushButton::clicked, this, &MainWindow::loadImage);
    connect(saveBtn, &QPushButton::clicked, this, &MainWindow::saveImage);
    connect(processBtn, &QPushButton::clicked, this, &MainWindow::processImage);
    connect(kernelSlider, &QSlider::valueChanged, this, &MainWindow::updateKernelSize);
    connect(threadCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MainWindow::updateThreadCount);
}

void MainWindow::loadImage()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open Image", "", 
                                                     "Images (*.png *.jpg *.jpeg *.bmp)");
    if (fileName.isEmpty()) return;
    
    originalImage.load(fileName);
    if (originalImage.isNull()) {
        QMessageBox::warning(this, "Error", "Failed to load image");
        return;
    }
    
    originalLabel->setPixmap(QPixmap::fromImage(originalImage));
    processBtn->setEnabled(true);
    
    logMessage(QString("Loaded image: %1 (%2x%3 pixels)")
               .arg(fileName).arg(originalImage.width()).arg(originalImage.height()));
}

void MainWindow::processImage()
{
    if (originalImage.isNull()) return;
    
    processBtn->setEnabled(false);
    progressBar->setVisible(true);
    progressBar->setRange(0, 0);
    
    timer.start();
    
    int threads = threadCombo->currentText().toInt();
    omp_set_num_threads(threads);
    
    QString filterType = filterCombo->currentText();
    int kernelSize = kernelSlider->value();
    
    logMessage(QString("Processing with %1 (OpenMP: %2 threads, MPI: %3 processes)")
               .arg(filterType).arg(threads).arg(mpiProcessCount));
    
    // Apply filter using OpenMP parallelization
    if (filterType == "Gaussian Blur") {
        processedImage = applyGaussianBlur(originalImage, kernelSize, threads);
    } else if (filterType == "Edge Detection") {
        processedImage = applyEdgeDetection(originalImage, threads);
    } else if (filterType == "Sharpen") {
        processedImage = applySharpening(originalImage, threads);
    }
    
    qint64 elapsed = timer.elapsed();
    
    processedLabel->setPixmap(QPixmap::fromImage(processedImage));
    saveBtn->setEnabled(true);
    processBtn->setEnabled(true);
    progressBar->setVisible(false);
    
    statsLabel->setText(QString(
        "Processing Time: %1 ms\n"
        "OpenMP Threads: %2\n"
        "MPI Processes: %3\n"
        "Image Size: %4x%5\n"
        "Filter: %6")
        .arg(elapsed).arg(threads).arg(mpiProcessCount)
        .arg(originalImage.width()).arg(originalImage.height())
        .arg(filterType));
    
    logMessage(QString("Processing completed in %1 ms").arg(elapsed));
}

QImage MainWindow::applyGaussianBlur(const QImage &img, int kernelSize, int threads)
{
    QImage result = img.convertToFormat(QImage::Format_RGB888);
    int w = result.width();
    int h = result.height();
    int radius = kernelSize / 2;
    
    // Create Gaussian kernel
    std::vector<std::vector<float>> kernel(kernelSize, std::vector<float>(kernelSize));
    float sigma = kernelSize / 6.0f;
    float sum = 0.0f;
    
    for (int i = 0; i < kernelSize; i++) {
        for (int j = 0; j < kernelSize; j++) {
            int x = i - radius;
            int y = j - radius;
            kernel[i][j] = exp(-(x*x + y*y) / (2*sigma*sigma));
            sum += kernel[i][j];
        }
    }
    
    // Normalize kernel
    for (int i = 0; i < kernelSize; i++) {
        for (int j = 0; j < kernelSize; j++) {
            kernel[i][j] /= sum;
        }
    }
    
    QImage temp(w, h, QImage::Format_RGB888);
    
    // Apply convolution with OpenMP parallelization
    #pragma omp parallel for collapse(2) schedule(dynamic)
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            float r = 0, g = 0, b = 0;
            
            for (int ky = 0; ky < kernelSize; ky++) {
                for (int kx = 0; kx < kernelSize; kx++) {
                    int px = std::min(std::max(x + kx - radius, 0), w - 1);
                    int py = std::min(std::max(y + ky - radius, 0), h - 1);
                    
                    QRgb pixel = result.pixel(px, py);
                    float weight = kernel[ky][kx];
                    
                    r += qRed(pixel) * weight;
                    g += qGreen(pixel) * weight;
                    b += qBlue(pixel) * weight;
                }
            }
            
            temp.setPixel(x, y, qRgb((int)r, (int)g, (int)b));
        }
    }
    
    return temp;
}

QImage MainWindow::applyEdgeDetection(const QImage &img, int threads)
{
    QImage result = img.convertToFormat(QImage::Format_RGB888);
    int w = result.width();
    int h = result.height();
    
    // Sobel operators
    int gx[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    int gy[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};
    
    QImage temp(w, h, QImage::Format_RGB888);
    
    #pragma omp parallel for collapse(2) schedule(dynamic)
    for (int y = 1; y < h - 1; y++) {
        for (int x = 1; x < w - 1; x++) {
            int sumX = 0, sumY = 0;
            
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    QRgb pixel = result.pixel(x + kx, y + ky);
                    int gray = qGray(pixel);
                    sumX += gray * gx[ky + 1][kx + 1];
                    sumY += gray * gy[ky + 1][kx + 1];
                }
            }
            
            int magnitude = std::min(255, (int)sqrt(sumX*sumX + sumY*sumY));
            temp.setPixel(x, y, qRgb(magnitude, magnitude, magnitude));
        }
    }
    
    return temp;
}

QImage MainWindow::applySharpening(const QImage &img, int threads)
{
    QImage result = img.convertToFormat(QImage::Format_RGB888);
    int w = result.width();
    int h = result.height();
    
    // Sharpening kernel
    int kernel[3][3] = {{0, -1, 0}, {-1, 5, -1}, {0, -1, 0}};
    
    QImage temp(w, h, QImage::Format_RGB888);
    
    #pragma omp parallel for collapse(2) schedule(dynamic)
    for (int y = 1; y < h - 1; y++) {
        for (int x = 1; x < w - 1; x++) {
            int r = 0, g = 0, b = 0;
            
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    QRgb pixel = result.pixel(x + kx, y + ky);
                    int weight = kernel[ky + 1][kx + 1];
                    r += qRed(pixel) * weight;
                    g += qGreen(pixel) * weight;
                    b += qBlue(pixel) * weight;
                }
            }
            
            r = std::min(255, std::max(0, r));
            g = std::min(255, std::max(0, g));
            b = std::min(255, std::max(0, b));
            
            temp.setPixel(x, y, qRgb(r, g, b));
        }
    }
    
    return temp;
}

void MainWindow::saveImage()
{
    if (processedImage.isNull()) return;
    
    QString fileName = QFileDialog::getSaveFileName(this, "Save Image", "", 
                                                     "PNG (*.png);;JPEG (*.jpg)");
    if (fileName.isEmpty()) return;
    
    if (processedImage.save(fileName)) {
        logMessage(QString("Image saved: %1").arg(fileName));
        QMessageBox::information(this, "Success", "Image saved successfully");
    } else {
        QMessageBox::warning(this, "Error", "Failed to save image");
    }
}

void MainWindow::updateKernelSize(int value)
{
    // Ensure odd kernel size
    if (value % 2 == 0) value++;
    kernelSlider->blockSignals(true);
    kernelSlider->setValue(value);
    kernelSlider->blockSignals(false);
    
    QLabel *label = qobject_cast<QLabel*>(
        kernelSlider->parentWidget()->layout()->itemAt(3)->widget());
    if (label) {
        label->setText(QString("%1x%1").arg(value));
    }
}

void MainWindow::updateThreadCount(int index)
{
    logMessage(QString("Thread count changed to: %1").arg(threadCombo->currentText()));
}

void MainWindow::logMessage(const QString &msg)
{
    logText->append(QString("[%1] %2")
                    .arg(QTime::currentTime().toString("HH:mm:ss"))
                    .arg(msg));
}

void MainWindow::workerLoop(int rank)
{
    // MPI worker processes handle distributed tasks here
    // workers simply wait and could be extended
    
    while (true) {
        int command;
        MPI_Status status;
        MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        
        if (status.MPI_TAG == TAG_IMAGE_DATA) {
            // Handle distributed image processing
            break;
        }
    }
}
