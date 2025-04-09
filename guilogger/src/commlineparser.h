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

/**
 * \brief Class for parsing parameter values from the command line
 * \author Dominic Schneider
 */
#ifndef COMMLINEPARSER_H
#define COMMLINEPARSER_H

#include <QList>
#include <QString>
#include <QMap>

class CommLineParser
{
private:
  QString mode;    // input streaming mode = serial | pipe | file
  QString port;    // serial port to read from
  QString file;    // input file for visualisation
  bool    logg;    // Logging on/off
  bool    help;    // display help or not
  int     delay;   // delay for pipe

  QMap<QString, QString> paramMap;
  bool mpparse;

public:

  CommLineParser()
  {
    logg = false;
    help = false;
    delay = 100;
    mpparse = false;
    mode = "pipe";
  }


  QString getMode() const  {return mode;}
  void    setMode(const QString& m)  {mode=m;}
  QString getPort() const  {return port;}
  QString getFile() const  {return file;}
  bool    getLogg() const  {return logg;}
  bool    getHelp() const  {return help;}
  int     getDelay() const {return delay;}


  // implementation for special use (read guilogger command line parameters)
  void parseCommandLine(int argc, char **argv)
  {
    QList<QString> ComLineParams;
    for(int i=1; i<argc; i++) ComLineParams.push_back(argv[i]);

    int i=0;
    if((i = ComLineParams.indexOf("-m")) != -1) mode = ComLineParams[i+1];
    if((i = ComLineParams.indexOf("-p")) != -1) port = ComLineParams[i+1];
    if((i = ComLineParams.indexOf("-f")) != -1) file = ComLineParams[i+1];
    if((i = ComLineParams.indexOf("-d")) != -1) delay = ComLineParams[i+1].toInt();
    if((i = ComLineParams.indexOf("-l")) != -1) logg = true;
    if((i = ComLineParams.indexOf("-h")) != -1) help = true;
    if((i = ComLineParams.indexOf("--help")) != -1) help = true;
  }


  // more common implementation for general purpose
  QMap<QString, QString> parseCommandLine2(int argc, char **argv)
  {
    for(int i=1; i<argc; i++)
      {
        if((argv[i][0] == '-') && (argv[i+1] != 0) && (argv[i+1][0] != '-'))
          {   paramMap.insert(argv[i], argv[i+1]);
            i++;
          }
        else if(argv[i][0] == '-' && argv[i+1] != 0 && argv[i+1][0] == '-') paramMap.insert(argv[i], "1");
        else if(argv[i][0] == '-' && argv[i+1] == 0 ) paramMap.insert(argv[i], "1");
      }

    mpparse = true;
    return paramMap;
  }


  QString getParamValue(QString key)
  {   if(!mpparse) {printf("getParamValue(): parseCommandLine2 not executed, please call it to use this function.\n"); return "";}

    QMap<QString, QString>::iterator it;

    it = paramMap.find(key);

    if(it == paramMap.end()) return "";
    else return *it;
  }

};
#endif
