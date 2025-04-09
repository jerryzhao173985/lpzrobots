#!/bin/bash

# Script to build the project with Qt5
echo "Building the project with Qt5..."

# First, check for Qt5 dependencies
./check_qt5_deps.sh
if [ $? -ne 0 ]; then
    echo "Qt5 dependencies are missing. Please install them first."
    exit 1
fi

# Fix any remaining Qt5 issues
./fix_qt5_issues.sh

# Configure and build the project
echo "Configuring and building the project..."

# Configure guilogger
echo "Configuring guilogger..."
cd guilogger
./configure
make
cd ..

# Configure matrixviz
echo "Configuring matrixviz..."
cd matrixviz
./configure
make
cd ..

# Configure configurator
echo "Configuring configurator..."
cd configurator
./configure
make
cd ..

# Build the main project
echo "Building the main project..."
make all

echo "Done building the project with Qt5."
