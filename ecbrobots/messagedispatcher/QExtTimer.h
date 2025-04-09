/***************************************************************************
 *   Copyright (C) 2005 by Robot Group Leipzig                             *
 *    martius@informatik.uni-leipzig.de                                    *
 *    fhesse@informatik.uni-leipzig.de                                     *
 *    der@informatik.uni-leipzig.de                                        *
 *    guettler@informatik.uni-leipzig.de                                   *
 *    jhoffmann@informatik.uni-leipzig.de                                  *
 *    wolfgang.rabe@01019freenet.de                                        *
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
 ***************************************************************************
 *                                                                         *
 *  DESCRIPTION                                                            *
 *                                                                         *
 *   $Log$
 *   Revision 1.2  2011-04-05 12:16:04  guettler
 *   - new tabWidget
 *   - all found DNS devices are shown in tabWidget with a QDNSDeviceWidget each
 *   - QDNSDeviceWidget shows DNS device name, USB adapter name and type,
 *     response time and incoming/outgoing status (if messages are currently sent
 *     or received)
 *
 *   Revision 1.1  2010/11/18 16:58:18  wrabe
 *   - current state of work
 *
 *                                                                         *
 ***************************************************************************/

#ifndef __QEXTTIMER_H_
#define __QEXTTIMER_H_

#include <QTimer>

namespace lpzrobots {
  
  class QExtTimer : public QTimer {

     Q_OBJECT

     public:
       QExtTimer();

       virtual ~QExtTimer();

       virtual void start(int msec, uint eventId);
       virtual void start(uint eventId);

       virtual unsigned int stop();

       virtual unsigned int getTimeRan();

     signals:
       void timeout(uint eventId);

     private slots:
       virtual void sl_timeout();
       virtual void sl_tickTimeout();

     private:
       unsigned int eventId;
       QTimer tickTimer;
       unsigned int timeRan;

   };

}

#endif /* __QEXTTIMER_H_ */
