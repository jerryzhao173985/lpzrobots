#!/bin/bash

# Script to update the build system for Qt5
echo "Updating build system for Qt5..."

# Update configure scripts to use qmake-qt5
find . -name "configure" | while read file; do
    # Replace qmake with qmake-qt5 with fallback
    sed -i 's/qmake /QMAKE=qmake-qt5\nif ! which $QMAKE > \/dev\/null 2>\&1; then\n    QMAKE=qmake\nfi\n$QMAKE /g' "$file"
    echo "Updated $file"
done

# Update Makefile.conf to use Qt5
if [ -f "Makefile.conf" ]; then
    # Add Qt5 dependencies
    echo "Updating Makefile.conf..."
    sed -i 's/libqt4-dev/qtbase5-dev qtdeclarative5-dev libqt5opengl5-dev qttools5-dev qttools5-dev-tools/g' Makefile.conf
fi

echo "Done updating build system for Qt5."
