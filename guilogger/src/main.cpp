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
#define DEBUG

#include <signal.h>

#include <QApplication>
#include <QDesktopWidget>
#include "guilogger.h"
#include "filelogger.h"
#include "qserialreader.h"
#include "qpipereader.h"
#include "commlineparser.h"


void signal_handler_exit(void){
  signal(SIGINT,SIG_DFL);
}

void control_c(int ){ }


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
  signal(SIGPIPE, SIG_DFL);
#endif
}

void printUsage(){
  printf("guilogger parameter listing\n");
  printf("   -m [mode]  mode = serial | pipe (def) | fpipe| file\n");
  printf("   -p [port]  port = serial port to read from\n");
  printf("   -d [delay] delay = ms to wait between data (for fpipe)\n");
  printf("   -f [file]  input file\n");
  printf("      only viwewing, no streaming\n");
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

    QApplication a( argc, argv );

    QDataSource *qsource=0;


    if(params.getMode().isEmpty() && !params.getFile().isEmpty()) {
      params.setMode("file");
    }

    QRect screenRect(0,0,1024,768);
    if(a.desktop()){
      screenRect = a.desktop()->screenGeometry();
    }
    GuiLogger *gl = new GuiLogger(params, screenRect);
    ChannelData* cd = &gl->getChannelData();

    if(params.getMode()=="serial")
    {   QSerialReader *qserial = new QSerialReader();
        if(params.getPort() != "") qserial->setComPort(params.getPort());
        printf("Guilogger: Using serial port %s as source.\n", qserial->getComPort().toLatin1().data());
        qsource = qserial;
        a.connect(qsource, SIGNAL(newData(QString)), cd, SLOT(receiveRawData(QString)));
        qsource->start();
    }else if(params.getMode()=="pipe") {
      QPipeReader *qpipe = new QPipeReader();
      //        if(params.getDelay() >= 0) qpipe->setDelay(params.getDelay());
      //        printf("Using pipe input with delay %i.\n", qpipe->getDelay());
      printf("Guilogger: Using pipe input\n");
      qsource = qpipe;
      a.connect(qsource, SIGNAL(newData(QString)), cd, SLOT(receiveRawData(QString)));
      qsource->start();
    }else if(params.getMode()=="fpipe") {
      FILE* f = fopen(params.getFile().toLatin1().data(),"r");
      QPipeReader *qpipe = new QPipeReader(0,f);
      if(params.getDelay() >= 0) qpipe->setDelay(params.getDelay());
      printf("Guilogger: Using file-pipe input\n");
      qsource = qpipe;
      a.connect(qsource, SIGNAL(newData(QString)), cd, SLOT(receiveRawData(QString)));
      qsource->start();
    } else if(params.getMode()=="file") {
      // will be connected within guilogger class
      // printf("Sorry, there are no native segfaults any more.\n");
//        printf("But nevertheless I further provide segfaults for convenience by using free(0)\n");
//        printf("Just kidding! Have a nice day.\n");
    } else {
      printUsage();
      return 1;
    }


    FileLogger fl;
    if(params.getLogg())
    {   fl.setLogging(true);
        printf("Guilogger: Logging is on\n");
        a.connect(qsource, SIGNAL(newData(QString)), &fl, SLOT(writeChannelData(QString)));  // the filelogger is listening
    }

//    if(params.getMode() != "file") qsource->start();

    gl->setWindowTitle( "GUI Logger" );
    gl->show();
    a.connect( gl, SIGNAL(quit()), &a, SLOT(quit()) );
    a.connect( &a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()) );
    return a.exec();
}
