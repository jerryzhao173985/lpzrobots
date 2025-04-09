#!/bin/bash

# Script to check for Qt5 compatibility issues
echo "Checking for Qt5 compatibility issues..."

# Check for Qt3Support classes
echo "Checking for Qt3Support classes..."
find . -name "*.cpp" -o -name "*.h" | xargs grep -l "Q3" | sort

# Check for deprecated Qt4 APIs
echo "Checking for deprecated Qt4 APIs..."
find . -name "*.cpp" -o -name "*.h" | xargs grep -l "QStyleOptionProgressBarV2" | sort
find . -name "*.cpp" -o -name "*.h" | xargs grep -l "setMargin" | sort

# Check for old-style includes
echo "Checking for old-style includes..."
find . -name "*.cpp" -o -name "*.h" | xargs grep -l "#include <q.*\.h>" | sort

echo "Done checking for Qt5 compatibility issues."
