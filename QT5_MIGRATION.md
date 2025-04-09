# Qt5 Migration Guide for lpzrobots

This document provides instructions for migrating the lpzrobots project from Qt4 with Qt3 support to Qt5 for Ubuntu 24.

## Prerequisites

Before starting the migration, make sure you have the following Qt5 dependencies installed:

```bash
sudo apt-get install qtbase5-dev qtdeclarative5-dev libqt5opengl5-dev qttools5-dev qttools5-dev-tools
```

You can check if the dependencies are installed by running:

```bash
./check_qt5_deps.sh
```

## Migration Steps

1. Run the migration script:

```bash
./migrate_to_qt5.sh
```

This script will:
- Check for Qt5 dependencies
- Update Qt includes from Qt4 to Qt5 style
- Update specific Qt includes
- Update QtWidgets includes
- Update the build system for Qt5
- Check for any remaining Qt5 compatibility issues

2. Build the project with Qt5:

```bash
./build_with_qt5.sh
```

## Manual Fixes

If you encounter any issues during the migration, you may need to manually fix some files. Here are some common issues and their solutions:

### Qt3Support Classes

Replace Qt3Support classes with their Qt5 equivalents:

- `Q3ScrollView` -> `QScrollArea`
- `Q3ListView` -> `QTreeView` or `QListView`
- `Q3Header` -> `QHeaderView`
- `Q3Frame` -> `QFrame`
- `Q3TextEdit` -> `QTextEdit`
- `Q3Table` -> `QTableWidget`

### Deprecated Qt4 APIs

Replace deprecated Qt4 APIs with their Qt5 equivalents:

- `QStyleOptionProgressBarV2` -> `QStyleOptionProgressBar`
- `setMargin(int)` -> `setContentsMargins(int, int, int, int)`
- `QWorkspace` -> `QMdiArea`
- `QHttp` -> `QNetworkAccessManager`

### Qt5 Module Changes

In Qt5, the QtGui module has been split into QtGui, QtWidgets, and QtPrintSupport. Update your includes accordingly:

- `#include <QtGui>` -> `#include <QtWidgets>`
- `#include <qmainwindow.h>` -> `#include <QMainWindow>`
- `#include <qapplication.h>` -> `#include <QApplication>`

### Build System Changes

Update your build system to use Qt5:

- Replace `qmake` with `qmake-qt5` (with fallback to `qmake`)
- Update Qt module dependencies in .pro files
- Update library references in configuration files

## Troubleshooting

If you encounter any issues during the migration, try the following:

1. Check for Qt5 compatibility issues:

```bash
./check_qt5_compatibility.sh
```

2. Update specific Qt includes:

```bash
./update_qt_includes_specific.sh
```

3. Update QtWidgets includes:

```bash
./update_qt_widgets.sh
```

4. Update the build system:

```bash
./update_build_system.sh
```

If you still encounter issues, you may need to manually fix some files. Refer to the "Manual Fixes" section above for guidance.
