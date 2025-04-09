#!/bin/bash

# Check for Qt5 dependencies on Ubuntu 24
echo "Checking for Qt5 dependencies..."

# Check for Qt5 base packages
if ! dpkg -l | grep -q "qtbase5-dev"; then
    echo "Missing qtbase5-dev package. Please install it with:"
    echo "sudo apt-get install qtbase5-dev"
    exit 1
fi

# Check for Qt5 declarative packages
if ! dpkg -l | grep -q "qtdeclarative5-dev"; then
    echo "Missing qtdeclarative5-dev package. Please install it with:"
    echo "sudo apt-get install qtdeclarative5-dev"
    exit 1
fi

# Check for Qt5 OpenGL packages
if ! dpkg -l | grep -q "libqt5opengl5-dev"; then
    echo "Missing libqt5opengl5-dev package. Please install it with:"
    echo "sudo apt-get install libqt5opengl5-dev"
    exit 1
fi

# Check for Qt5 tools packages
if ! dpkg -l | grep -q "qttools5-dev"; then
    echo "Missing qttools5-dev package. Please install it with:"
    echo "sudo apt-get install qttools5-dev qttools5-dev-tools"
    exit 1
fi

echo "All Qt5 dependencies are installed."
exit 0
