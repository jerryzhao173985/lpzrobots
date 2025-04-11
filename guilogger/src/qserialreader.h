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
#ifndef QSERIALREADER_H
#define QSERIALREADER_H

#include <QObject>
#include <QString>
#include <QtSerialPort/QSerialPort> // Include QSerialPort header
#include <QByteArray> // For buffering data

/** \brief Class for reading complete lines from the serial port terminated by a special character
  * Uses QSerialPort for proper Qt integration.
  * \author Dominic Schneider, Gemini Refactoring
  */
class QSerialReader : public QObject
{
    Q_OBJECT

private:
    QSerialPort *serialPort; // Use QSerialPort
    QString portName;
    int baudRate;
    char blockTerminator;  // end-of-line symbol for readed data
    volatile bool stopRequested; // Flag to control stopping
    QByteArray readBuffer; // Buffer for incoming data

public:
    QSerialReader(char bt = '\n', QObject *parent = nullptr);
    ~QSerialReader();
    
    void setComPort(const QString& name){ this->portName = name; };   /// set com port name
    QString getComPort() const {return portName;};
    void setBaudrate(int rate){ baudRate = rate; };      /// set baud rate

private slots:
    void handleReadyRead();  // Slot to handle incoming data
    void handleError(QSerialPort::SerialPortError error); // Slot to handle errors

public slots:
    void process(); // Starts the connection
    void stop();    // Stops the connection

signals:
    void newData(const QString& datablock);
    void finished();
    void error(const QString& errorString);
};

#endif // QSERIALREADER_H
