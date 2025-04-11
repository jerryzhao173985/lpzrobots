/***************************************************************************
 *   Copyright (C) 2005-2011 LpzRobots development team                    *
 *   Refactored to use QSerialPort in 2024 by Gemini                       *
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
#include "qserialreader.h"
#include <QtSerialPort/QSerialPortInfo>
#include <QDebug>
#include <QThread> // For QThread::msleep if needed, though aiming for event-driven

QSerialReader::QSerialReader(char bt, QObject *parent)
    : QObject(parent),
      serialPort(new QSerialPort(this)), // Create QSerialPort instance
      portName("/dev/ttyS0"), // Default port name
      baudRate(19200),      // Default baud rate
      blockTerminator(bt),
      stopRequested(false)
{
}

QSerialReader::~QSerialReader()
{
    // QSerialPort is deleted automatically due to parent-child relationship
    // Ensure port is closed if open
    if (serialPort->isOpen()) {
        serialPort->close();
    }
}

void QSerialReader::process()
{
    stopRequested = false;
    readBuffer.clear();

    serialPort->setPortName(portName);

    if (!serialPort->setBaudRate(baudRate)) {
        emit error(QString("QSerialReader: Failed to set baud rate %1 for %2: %3")
                   .arg(baudRate).arg(portName).arg(serialPort->errorString()));
        emit finished();
        return;
    }

    // Standard serial settings (8N1, No Flow Control)
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setParity(QSerialPort::NoParity);
    serialPort->setStopBits(QSerialPort::OneStop);
    serialPort->setFlowControl(QSerialPort::NoFlowControl);

    // Connect signals for reading and errors
    connect(serialPort, &QSerialPort::readyRead, this, &QSerialReader::handleReadyRead);
    connect(serialPort, &QSerialPort::errorOccurred, this, &QSerialReader::handleError);

    if (serialPort->open(QIODevice::ReadOnly)) {
        qDebug() << "QSerialReader: Successfully opened" << portName;
    } else {
        emit error(QString("QSerialPortReader: Cannot open serial port %1: %2")
                   .arg(portName).arg(serialPort->errorString()));
        emit finished();
    }
    // No loop here - reading is driven by the readyRead signal
}

void QSerialReader::stop()
{
    qDebug() << "QSerialReader: Stopping...";
    stopRequested = true;
    if (serialPort->isOpen()) {
        serialPort->close();
        qDebug() << "QSerialReader: Port closed.";
    }
    // Disconnect signals to avoid potential issues after stop request
    disconnect(serialPort, &QSerialPort::readyRead, this, &QSerialReader::handleReadyRead);
    disconnect(serialPort, &QSerialPort::errorOccurred, this, &QSerialReader::handleError);
    
    emit finished();
}

void QSerialReader::handleReadyRead()
{
    if (stopRequested || !serialPort->isOpen()) return;

    readBuffer.append(serialPort->readAll());

    // Process buffer line by line
    while (true) {
        int termIndex = readBuffer.indexOf(blockTerminator);
        int crIndex = readBuffer.indexOf('\r');
        int lfIndex = readBuffer.indexOf('\n');
        
        // Find the first line terminator (CR, LF, or custom block terminator)
        int firstTerminator = -1; // Initialize index for the earliest terminator found
        if (termIndex != -1) firstTerminator = termIndex;
        if (crIndex != -1 && (firstTerminator == -1 || crIndex < firstTerminator)) firstTerminator = crIndex;
        if (lfIndex != -1 && (firstTerminator == -1 || lfIndex < firstTerminator)) firstTerminator = lfIndex;

        if (firstTerminator == -1) {
            // No complete line found yet
            break; 
        }
        
        // Extract line data (up to the terminator)
        QByteArray lineData = readBuffer.left(firstTerminator);
        // Remove the line and the terminator(s) from the buffer
        // Handle potential CRLF endings by checking if LF follows CR
        int removeLen = firstTerminator + 1; // Remove at least the terminator
        if (readBuffer.at(firstTerminator) == '\r' && firstTerminator + 1 < readBuffer.size() && readBuffer.at(firstTerminator + 1) == '\n') {
            removeLen++; // Remove LF as well
        }
        readBuffer = readBuffer.mid(removeLen);

        // Handle '#' reset logic if needed (assuming # always starts a line)
        if (!lineData.isEmpty() && lineData.at(0) == '#') {
             // Process #C or other special lines if necessary
        }

        if (!lineData.isEmpty()) { // Don't send empty lines
             // Assuming data is UTF-8 or compatible (like ASCII)
             // Use fromLatin1 if you specifically expect 8-bit encoding
             emit newData(QString::fromUtf8(lineData)); 
        }
    }
    // Optional: Limit buffer size to prevent memory exhaustion
    // if (readBuffer.size() > MAX_BUFFER_SIZE) { /* handle error or clear buffer */ }
}

void QSerialReader::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError || stopRequested) {
        return; // Ignore NoError or errors occurred after stop request
    }

    QString errorMsg = QString("QSerialReader: An error occurred on %1: %2 (Code: %3)")
                       .arg(portName)
                       .arg(serialPort->errorString())
                       .arg(error);
    qWarning() << errorMsg;
    emit this->error(errorMsg);
    
    // Optionally close port and emit finished on critical errors
    // if (error != QSerialPort::TimeoutError) { // Example: ignore timeouts
         stop(); // Close port and signal finished
    // }
}
