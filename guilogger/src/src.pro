# Diese Datei wurde mit dem qmake-Manager von KDevelop erstellt.
# -------------------------------------------
# Unterverzeichnis relativ zum Projektverzeichnis: ./src
# Das Target ist eine Anwendung: ../bin/guilogger

# Project Template
TEMPLATE = app

# Output target (corrected to your original desired path)
TARGET = bin/guilogger

# Qt modules required
QT += core gui widgets serialport

# Configuration (Debug mode enabled explicitly for development convenience)
CONFIG += debug \
          warn_on \
          thread \
          qt \
          console \
          moc

# Include and dependency paths
INCLUDEPATH += .
DEPENDPATH += .

# DEFINES (Uncomment the line below to explicitly enable debugging symbols if needed)
# DEFINES += DEBUG

# Header files
HEADERS += guilogger.h \
           gnuplot.h \
           filelogger.h \
           qserialreader.h \
           qpipereader.h \
           inifile.h \
           stl_adds.h \
           plotchannelstablemodel.h \
           plotinfo.h \
           channeldata.h \
           commlineparser.h \
           gnuplot_unix.h \
           quickmp.h

# Source files
SOURCES += guilogger.cpp \
           main.cpp \
           gnuplot.cpp \
           filelogger.cpp \
           qserialreader.cpp \
           qpipereader.cpp \
           inifile.cpp \
           stl_adds.cpp \
           plotchannelstablemodel.cpp \
           plotinfo.cpp \
           channeldata.cpp

# OpenMP parallelization (uncomment these lines if OpenMP is needed)
# QMAKE_CXXFLAGS += -fopenmp
# QMAKE_LFLAGS += -fopenmp -lgomp

# Installation settings (retain original installation path, uncomment if installation required)
target.path = /usr/bin
INSTALLS += target
