#!/bin/bash

# Script to fix Qt5 issues
echo "Fixing Qt5 issues..."

# Fix QGLWidget includes
find . -name "*.cpp" -o -name "*.h" | xargs grep -l "QGl" | while read file; do
    sed -i 's/#include <QGl>/#include <QGLWidget>/g' "$file"
    echo "Fixed QGl include in $file"
done

# Fix configure scripts
find . -name "configure" | while read file; do
    # Check if the file contains the problematic line
    if grep -q "\$QMAKE for Qt5 first" "$file"; then
        # Fix the problematic line
        sed -i 's/\$QMAKE for Qt5 first, then fall back to qmake/# Try to find qmake for Qt5 first, then fall back to qmake/g' "$file"
        echo "Fixed configure script $file"
    fi
done

# Fix QStyleOptionProgressBarV2
find . -name "*.cpp" -o -name "*.h" | xargs grep -l "QStyleOptionProgressBarV2" | while read file; do
    sed -i 's/QStyleOptionProgressBarV2/QStyleOptionProgressBar/g' "$file"
    echo "Fixed QStyleOptionProgressBarV2 in $file"
done

# Fix setMargin
find . -name "*.cpp" -o -name "*.h" | xargs grep -l "setMargin" | while read file; do
    sed -i 's/\([^>]\)setMargin(\([0-9]*\))/\1setContentsMargins(\2, \2, \2, \2)/g' "$file"
    echo "Fixed setMargin in $file"
done

echo "Done fixing Qt5 issues."
