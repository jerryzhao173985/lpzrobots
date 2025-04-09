#!/bin/bash

# Master script to migrate from Qt4 to Qt5
echo "Starting migration from Qt4 to Qt5..."

# Check for Qt5 dependencies
./check_qt5_deps.sh
if [ $? -ne 0 ]; then
    echo "Qt5 dependencies are missing. Please install them first."
    exit 1
fi

# Update Qt includes
./update_qt_includes.sh

# Update specific Qt includes
./update_qt_includes_specific.sh

# Update QtWidgets includes
./update_qt_widgets.sh

# Update build system
./update_build_system.sh

# Fix any remaining Qt5 issues
./fix_qt5_issues.sh

# Check for Qt5 compatibility issues
./check_qt5_compatibility.sh

echo "Migration from Qt4 to Qt5 completed."
echo "You can now build the project with Qt5 using ./build_with_qt5.sh"
