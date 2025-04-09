#!/bin/bash

# Script to update specific Qt includes from Qt4 to Qt5 style
echo "Updating specific Qt includes from Qt4 to Qt5 style..."

# Find all C++ files
find . -name "*.cpp" -o -name "*.h" | while read file; do
    # Update specific includes
    sed -i 's/#include <qapplication\.h>/#include <QApplication>/g' "$file"
    sed -i 's/#include <qmenubar\.h>/#include <QMenuBar>/g' "$file"
    sed -i 's/#include <qtimer\.h>/#include <QTimer>/g' "$file"
    sed -i 's/#include <qregexp\.h>/#include <QRegExp>/g' "$file"
    sed -i 's/#include <qscrollarea\.h>/#include <QScrollArea>/g' "$file"
    sed -i 's/#include <qthread\.h>/#include <QThread>/g' "$file"
    sed -i 's/#include <qmainwindow\.h>/#include <QMainWindow>/g' "$file"
    sed -i 's/#include <qmenu\.h>/#include <QMenu>/g' "$file"
    sed -i 's/#include <qstringlist\.h>/#include <QStringList>/g' "$file"
    sed -i 's/#include <qlabel\.h>/#include <QLabel>/g' "$file"
    sed -i 's/#include <qdialog\.h>/#include <QDialog>/g' "$file"
    sed -i 's/#include <qlinkedlist\.h>/#include <QLinkedList>/g' "$file"
    sed -i 's/#include <qlayout\.h>/#include <QLayout>/g' "$file"
    sed -i 's/#include <qmutex\.h>/#include <QMutex>/g' "$file"
    sed -i 's/#include <qmap\.h>/#include <QMap>/g' "$file"
    sed -i 's/#include <qlist\.h>/#include <QList>/g' "$file"
    sed -i 's/#include <qboxlayout\.h>/#include <QBoxLayout>/g' "$file"
    sed -i 's/#include <qlineedit\.h>/#include <QLineEdit>/g' "$file"
    sed -i 's/#include <qpushbutton\.h>/#include <QPushButton>/g' "$file"
    sed -i 's/#include <qslider\.h>/#include <QSlider>/g' "$file"
    sed -i 's/#include <qtextedit\.h>/#include <QTextEdit>/g' "$file"
    
    echo "Updated $file"
done

echo "Done updating specific Qt includes."
