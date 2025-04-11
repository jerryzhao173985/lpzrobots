## April 11, 2025

### Summary of Changes

The changes made:
1. Add thread-safety with mutex locks
2. Modernize the code to better handle Qt's event-driven model
3. Improve error handling

- channeldata.cpp/h:
  - Added thread-safety with QMutex locks
  - Improved error handling and null checks
- qpipereader.cpp/h:
  - Enhanced socket notifier handling
  - Better resource cleanup
- qserialreader.cpp/h:
  - Modernized serial port handling
  - Improved error reporting
- gnuplot.cpp/h:
  - Switched from FILE* pipe to QProcess
  - Added fallback terminal support
  - Better window management
- inifile.cpp:
  - Better parsing and error handling
- guilogger.cpp:
  - Improved initialization sequence
  - Better Gnuplot window handling
- main.cpp:
  - Fixed threading model
  - Better cleanup on exit

Note: Current version of guilogger uses the x11 or wxt terminal. Qt terminal need to be installed with gnuplot to be used alternatively. (As the default gnuplot package often doesn't include the Qt terminal.)
```
sudo apt-get install gnuplot-qt qt5-default
```
A version of guilogger with extensive debug outputs is uploaded to the repository. (commented out before committing)

---
