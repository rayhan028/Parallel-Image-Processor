#include "mainwindow.h"
#include <QApplication>
#include <mpi.h>

int main(int argc, char *argv[])
{
    // Initialize MPI
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Only rank 0 runs the GUI
    int result = 0;
    if (rank == 0) {
        QApplication app(argc, argv);
        app.setApplicationName("Parallel Image Processor");
        
        MainWindow window(size);
        window.show();
        
        result = app.exec();
    } 
    else {
        // Worker processes wait for tasks
        MainWindow::workerLoop(rank);
    }
    
    MPI_Finalize();
    return result;
}
