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
#include "qpipereader.h"
#include <stdio.h>
#include <unistd.h>  // Keep for fileno()
#include <stdlib.h>
#include <QDebug>
#include <QThread> // Keep for QThread::currentThread() if needed for debugging
#include <QSocketNotifier>
#include <errno.h>
#include <string.h>
#include <QTimer>
#include <QCoreApplication>

QPipeReader::QPipeReader(int delay, FILE* f, QObject *parent) :
    QObject(parent), f(f), notifier(nullptr), delay(delay)
{
    // qDebug() << "QPipeReader::QPipeReader - Constructor called in thread:" << QThread::currentThreadId();
    if (!f) {
        qCritical("QPipeReader: Invalid FILE pointer provided in constructor!");
    }
    
    // Create a single-shot timer to defer notifier creation to event loop
    QTimer::singleShot(0, this, &QPipeReader::initializeNotifier);
}

QPipeReader::~QPipeReader()
{
    // qDebug() << "QPipeReader: Destructor called";
    // Delete notifier explicitly
    if (notifier) {
        notifier->setEnabled(false);
        delete notifier;
        notifier = nullptr;
    }
    
    // Do not close stdin if f points to it
    if (f != stdin && f != nullptr) {
         // qDebug() << "QPipeReader: Closing provided file pointer";
         fclose(f);
         f = nullptr;
    }
}

void QPipeReader::initializeNotifier()
{
    // This will now run in the event loop of the thread this object belongs to
    // qDebug() << "QPipeReader::initializeNotifier - Creating socket notifier in thread:" << QThread::currentThreadId();

    if (notifier) {
        qWarning() << "QPipeReader::initializeNotifier - Notifier already initialized";
        return;
    }
    
    if (!f) {
        qCritical("QPipeReader::initializeNotifier - Invalid FILE pointer!");
        emit errorOccurred("Cannot initialize notifier: Invalid file pointer");
        emit finished();
        return;
    }

    int fd = fileno(f);
    if (fd == -1) {
        QString errorMsg = QString("QPipeReader: Failed to get file descriptor: %1").arg(strerror(errno));
        qCritical() << errorMsg;
        emit errorOccurred(errorMsg);
        emit finished();
        return;
    }

    // Safety check to ensure we're in a Qt thread
    if (QThread::currentThread() != QCoreApplication::instance()->thread() &&
        !qobject_cast<QThread*>(QThread::currentThread())) {
        qCritical() << "QPipeReader::initializeNotifier - Must be called from a Qt thread! Current thread ID:" 
                   << QThread::currentThreadId();
        
        // Try to recover by deferring to a safe context
        QMetaObject::invokeMethod(this, "initializeNotifier", Qt::QueuedConnection);
        return;
    }
    
    // Create the notifier in the current thread
    notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
    connect(notifier, &QSocketNotifier::activated, this, &QPipeReader::handleReadyRead, Qt::DirectConnection);
    notifier->setEnabled(true);
    // qDebug() << "QPipeReader::initializeNotifier - Notifier created and connected for fd:" << fd;
}

int QPipeReader::getFileno() const {
    if (f) {
        return fileno(f);
    }
    return -1;
}

void QPipeReader::handleReadyRead()
{
    if (!notifier || !f) {
        qWarning("QPipeReader::handleReadyRead called with null notifier or file pointer");
        return;
    }
    
    // Temporarily disable notifier while reading to prevent reactivation
    notifier->setEnabled(false);

    // Buffer size increased to handle large data blocks but reduced from previous extreme size
    const int bufferSize = 16384; // 16KB is usually sufficient for line-based data
    char *s = (char*) malloc(bufferSize * sizeof(char));
    if (!s) {
        qWarning("QPipeReader: Failed to allocate read buffer");
        emit errorOccurred("Failed to allocate read buffer");
        notifier->setEnabled(true); // Re-enable notifier to try again later
        return;
    }

    // Read the line
    char* ctrl = fgets(s, bufferSize, f);

    if (ctrl) {
        // Successfully read a line
        QString data = QString::fromUtf8(s);
        emit newData(data.trimmed());
        // Re-enable notifier for next line
        notifier->setEnabled(true);
    } else {
        // Error or EOF
        if (feof(f)) {
            qDebug("QPipeReader: End of file reached");
            emit finished();
        } else if (ferror(f)) {
            QString errorMsg = QString("QPipeReader: Error reading from pipe: %1").arg(strerror(errno));
            qWarning() << errorMsg;
            // Clear the error flag to allow future reads
            clearerr(f);
            emit errorOccurred(errorMsg);
            notifier->setEnabled(true); // Re-enable notifier to try again later
        } else {
            // Re-enable notifier if no data was available but no error occurred
            notifier->setEnabled(true);
        }
    }

    free(s);
}

// Removed process() method
// Removed stop() method


