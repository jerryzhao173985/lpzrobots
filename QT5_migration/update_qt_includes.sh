#!/bin/bash

# Script to update Qt includes from Qt4 to Qt5 style
echo "Updating Qt includes from Qt4 to Qt5 style..."

# Find all C++ files
find . -name "*.cpp" -o -name "*.h" | while read file; do
    # Update QtGui to QtWidgets
    sed -i 's/#include <QtGui>/#include <QtWidgets>/g' "$file"
    
    # Update old-style includes to new-style
    sed -i 's/#include <q\([a-z]*\)\.h>/#include <Q\u\1>/g' "$file"
    
    # Update QStyleOptionProgressBarV2 to QStyleOptionProgressBar
    sed -i 's/QStyleOptionProgressBarV2/QStyleOptionProgressBar/g' "$file"
    
    # Update setMargin to setContentsMargins
    sed -i 's/\([^>]\)setMargin(\([0-9]*\))/\1setContentsMargins(\2, \2, \2, \2)/g' "$file"
    
    # Update qdom.h to QDomDocument
    sed -i 's/#include <qdom\.h>/#include <QDomDocument>/g' "$file"
    
    # Update QGL to QOpenGL
    sed -i 's/#include <qgl\.h>/#include <QGLWidget>/g' "$file"
    
    echo "Updated $file"
done

echo "Done updating Qt includes."
