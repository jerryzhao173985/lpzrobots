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

#include <stdio.h>
#include "filelogger.h"

FileLogger::FileLogger(QString pf)
{   prefix = pf;
    log= false;
    instream=0;
}

FileLogger::~FileLogger(){
  if(instream) {
    fclose(instream);
  }
}


void FileLogger::openStream()
{
    filename = prefix + (QDateTime::currentDateTime()).toString("yyyy-MM-dd_hh-mm-ss") + ".log";

    if(instream) {
      fclose(instream);
    }
    instream = fopen(filename.toLatin1().data(),"w+");
    printf("Open Logfile: %s\n",filename.toLatin1().data());
}


void FileLogger::writeChannelData(QString datablock)
{
    if(!log) return;

    if(!instream)
    {
      openStream();
    }
    if(instream)
      fprintf(instream, "%s", datablock.toLatin1().data());
}
