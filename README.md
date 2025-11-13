# Overview
This project serves as a technical blueprint for integrating three major parallel computing frameworks within a single cohesive application: Qt6 for cross-platform GUI development, OpenMP for shared-memory parallelism, and MPI for distributed computing architecture.

## What It Does
The application processes digital images through various computational filters (Gaussian blur, edge detection, sharpening) while leveraging multiple levels of parallelism. OpenMP parallelizes pixel-level operations across CPU cores, while MPI provides the infrastructure for distributed processing across multiple nodes or processes. The Qt framework delivers a responsive user interface that allows real-time configuration of processing parameters and visualization of performance metrics.

## Technical Highlights

Multi-threaded convolution operations using OpenMP directives with dynamic scheduling
MPI process architecture with master-worker pattern (GUI on rank 0, computation workers on remaining ranks)
Modern C++17 implementation with STL containers and smart memory management
Signal-slot architecture for asynchronous GUI updates and event handling
Real-time performance monitoring showing speedup from parallelization strategies

## Architecture
The application employs a layered architecture where the presentation layer (Qt widgets) communicates with the processing layer (OpenMP-parallelized algorithms) while maintaining an MPI communication layer for potential distributed workload distribution. Image data flows from file I/O through format conversion to parallel processing pipelines, with results displayed alongside performance analytics.
Use Cases
This codebase demonstrates patterns applicable to:

## Scientific computing applications requiring GUI frontends
High-performance image/signal processing systems
Hybrid parallel computing architectures combining threading and message-passing
Performance benchmarking and parallel algorithm analysis
Educational demonstrations of parallel programming paradigms


## Status: Production-ready demonstration project
Language: C++17
Frameworks: Qt6, OpenMP, MPI
