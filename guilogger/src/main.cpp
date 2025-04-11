/***************************************************************************
 *   Copyright (C) 2005-2011 LpzRobots development team                    *
 *    Dominic Schneider (original author)                                  *
 *    Georg Martius  <georg dot martius at web dot de>                     *
 *    Frank Guettler <guettler at informatik dot uni-leipzig dot de        *
 *    Frank Hesse    <frank at nld dot ds dot mpg dot de>                  *
 *    Ralf Der       <ralfder at mis dot mpg dot de>                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                         *
 ***************************************************************************/
#define DEBUG // Keep DEBUG defined if it was originally

#include <signal.h>

#include <QApplication>
#include <QScreen>
#include <QThread>
#include <QDebug>
#include "guilogger.h"
#include "filelogger.h"
#include "qserialreader.h"
#include "qpipereader.h"
#include "commlineparser.h"


void signal_handler_exit(void){
  signal(SIGINT,SIG_DFL);
}

void control_c(int ){
    qDebug() << "Ctrl-C caught, ignoring...";
}


/**
 * We need to catch Ctrl-C (SIGINT) because if we are called from another program (like ode simulation) that reacts on Ctrl-C we are killed.
 * SIGPIPE is emitted if the stdin or stdout breaks.
 * We need to terminate if the stdin breaks to get closed with the calling application (in  pipe mode).
*/
void signal_handler_init(){
  signal(SIGINT,control_c);
  atexit(signal_handler_exit);
#if defined(WIN32) || defined(_WIN32) || defined (__WIN32) || defined(__WIN32__) \
  || defined (_WIN64) || defined(__CYGWIN__) || defined(__MINGW32__)
#else
  signal(SIGPIPE, SIG_IGN);
#endif
}

void printUsage(){
  printf("guilogger parameter listing\n");
  printf("   -m [mode]  mode = serial | pipe (def) | fpipe| file\n");
  printf("   -p [port]  port = serial port to read from\n");
  printf("   -d [delay] delay = ms to wait between data (for fpipe)\n");
  printf("   -f [file]  input file\n");
  printf("      only viewing, no streaming\n");
  printf("   -l turns logging on\n");
  printf("   --help displays this message.\n");
}

/**
  * \brief Main Programm
  * \author Dominic Schneider
  */
int main( int argc, char ** argv ) {
   signal_handler_init();
   CommLineParser params;
   params.parseCommandLine(argc, argv);

   if(params.getHelp()) {
     printUsage();
     return 1;
   }

    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication a( argc, argv );
    // qDebug() << "Main thread ID:" << QThread::currentThreadId();

    // Using QPointer for safe access but removing redundant tracking variable
    QThread *dataSourceThread = nullptr;

    if(params.getMode().isEmpty() && !params.getFile().isEmpty()) {
      params.setMode("file");
    }

    QRect screenRect(0,0,1024,768);
    if (QGuiApplication::primaryScreen()) { // Check if primaryScreen is valid
        screenRect = QGuiApplication::primaryScreen()->geometry();
    }
    GuiLogger *gl = new GuiLogger(params, screenRect);
    ChannelData* cd = &gl->getChannelData();

    FileLogger fl;
    if(params.getLogg())
    {
        fl.setLogging(true);
        // printf("Guilogger: Logging is on\n");
    }

    if(params.getMode()=="serial")
    {
        QSerialReader *qserial = new QSerialReader();
        if(params.getPort() != "") qserial->setComPort(params.getPort());
        // printf("Guilogger: Using serial port %s as source.\n", qserial->getComPort().toLatin1().constData());

        dataSourceThread = new QThread();
        qserial->moveToThread(dataSourceThread);

        QObject::connect(dataSourceThread, &QThread::started, qserial, &QSerialReader::process);
        QObject::connect(qserial, &QSerialReader::finished, dataSourceThread, &QThread::quit);
        QObject::connect(qserial, &QSerialReader::finished, qserial, &QSerialReader::deleteLater);
        QObject::connect(dataSourceThread, &QThread::finished, dataSourceThread, &QThread::deleteLater);

        QObject::connect(qserial, &QSerialReader::newData, cd, &ChannelData::receiveRawData, Qt::QueuedConnection);
        QObject::connect(qserial, &QSerialReader::error, [&](const QString& errorString){
            // qWarning() << "Serial Error:" << errorString;
        });

        // Re-enable FileLogger connection if needed
        if(params.getLogg()) {
            QObject::connect(qserial, &QSerialReader::newData, &fl, &FileLogger::writeChannelData);
        }

        QObject::connect(&a, &QCoreApplication::aboutToQuit, qserial, &QSerialReader::stop);

        // qDebug() << "Starting serial worker thread from main thread:" << QThread::currentThreadId();
        dataSourceThread->start();
    }
    else if(params.getMode()=="pipe")
    {
        // Create QPipeReader in the main thread but don't move it to a worker thread
        // This should resolve the QSocketNotifier threading issue
        QPipeReader *qpipe = new QPipeReader();
        // printf("Guilogger: Using pipe input (fd %d)\n", qpipe->getFileno());
        
        // Connect data handling signals directly in the main thread
        QObject::connect(qpipe, &QPipeReader::newData, cd, &ChannelData::receiveRawData, Qt::QueuedConnection);
        
        // Connect error signal
        QObject::connect(qpipe, &QPipeReader::errorOccurred, [](const QString& errorMsg){
            // qWarning() << "Pipe Reader Error:" << errorMsg;
        });

        // Connect clean shutdown
        QObject::connect(&a, &QCoreApplication::aboutToQuit, [qpipe](){
            // Cleanup as needed
            qpipe->deleteLater();
        });

        // Re-enable FileLogger connection if needed
        if(params.getLogg()) {
             QObject::connect(qpipe, &QPipeReader::newData, &fl, &FileLogger::writeChannelData);
        }

        // qDebug() << "Guilogger: Pipe reader using main thread:" << QThread::currentThreadId();
        // No separate thread needed - initializeNotifier will be called via the QTimer
    }
    else if(params.getMode()=="fpipe")
    {
        FILE* f = fopen(params.getFile().toLatin1().constData(), "r");
        if (!f) {
            // qCritical("Guilogger: Could not open file %s for fpipe mode.", params.getFile().toLatin1().constData());
            delete gl;
            return 1;
        }
        
        // Create QPipeReader in the main thread as with regular pipe mode
        QPipeReader *qpipe = new QPipeReader(0, f);
        // printf("Guilogger: Using file-pipe input from %s (fd %d)\n", params.getFile().toLatin1().constData(), qpipe->getFileno());
        
        // Connect signals for data handling
        QObject::connect(qpipe, &QPipeReader::newData, cd, &ChannelData::receiveRawData, Qt::QueuedConnection);
        
        // Connect error signal
        QObject::connect(qpipe, &QPipeReader::errorOccurred, [](const QString& errorMsg){
            // qWarning() << "File Pipe Reader Error:" << errorMsg;
        });

        // Connect clean shutdown
        QObject::connect(&a, &QCoreApplication::aboutToQuit, [qpipe](){
            // Cleanup as needed
            qpipe->deleteLater();
        });

        // Re-enable FileLogger connection if needed
        if(params.getLogg()) {
             QObject::connect(qpipe, &QPipeReader::newData, &fl, &FileLogger::writeChannelData);
        }

        // qDebug() << "Guilogger: File pipe reader using main thread:" << QThread::currentThreadId();
        // No separate thread needed - initializeNotifier will be called via the QTimer
    }
    else if(params.getMode()=="file") {
      // printf("Guilogger: Using file mode with %s\n", params.getFile().toLatin1().constData());
    } else {
      printUsage();
      delete gl;
      return 1;
    }

    gl->setWindowTitle( "GUI Logger" );
    gl->show();
    // Connect using new Qt5 syntax
    QObject::connect( gl, &GuiLogger::quit, &a, &QApplication::quit );

    int retCode = a.exec();

    // Cleanup is handled by deleteLater signals connected above
    return retCode;
}
