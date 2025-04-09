#!/bin/bash

# Script to update QtWidgets includes
echo "Updating QtWidgets includes..."

# Find all C++ files
find . -name "*.cpp" -o -name "*.h" | while read file; do
    # Update QtWidgets includes
    sed -i 's/#include <QtWidgets\/QtWidgets>/#include <QtWidgets>/g' "$file"
    sed -i 's/#include <QtGui>/#include <QtWidgets>/g' "$file"
    
    # Update QStyleOptionProgressBarV2 to QStyleOptionProgressBar
    sed -i 's/QStyleOptionProgressBarV2/QStyleOptionProgressBar/g' "$file"
    
    echo "Updated $file"
done

echo "Done updating QtWidgets includes."
