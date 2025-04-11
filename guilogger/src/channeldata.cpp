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

#include "channeldata.h"
#include "stl_adds.h"
#include <stdio.h>
#include <QString>
#include <QStringList>
#include <QMutexLocker>
#include <QDebug>
#include <QCoreApplication>
#include <QThread>

ChannelData::ChannelData(int buffersize)
  : numchannels(0), buffersize(buffersize), time(0), initialized(false) {
  // qDebug() << "ChannelData::ChannelData - Constructor started in thread:" << QThread::currentThreadId();
  
  // Use direct initialization with no mutex locks
  // qDebug() << "ChannelData::ChannelData - Constructor completed";
}

int ChannelData::getBuffersize() {
  // No mutex during initialization
  if (!initialized) {
    return buffersize;
  }
  
  QMutexLocker locker(&dataMutex);
  return buffersize;
}

void ChannelData::setBufferSize(int newBuffersize) {
  // qDebug() << "ChannelData::setBufferSize - Called with size:" << newBuffersize 
  //         << "in thread:" << QThread::currentThreadId();
           
  // Skip mutex if not yet initialized
  if (!initialized) {
    buffersize = newBuffersize;
    // qDebug() << "ChannelData::setBufferSize - Set size without locking (not initialized)";
    return;
  }
  
  // Only use mutex for initialized state
  QMutexLocker locker(&dataMutex);
  buffersize = newBuffersize;
  data.resize(buffersize);
  for (int i = 0; i < buffersize; i++) {
    data[i].resize(numchannels);
  }
  time = 0;
  emit(update());
  // qDebug() << "ChannelData::setBufferSize - Completed with mutex";
}

/// sets a new information about a channel (also works before initialization)
void ChannelData::setChannelInfo(const ChannelInfo& info){
  // Skip mutex during initialization
  if (!initialized) {
    preset[info.name] = info;
    return;
  }
  
  QMutexLocker locker(&dataMutex);
  int index = getChannelIndexNoLock(info.name);
  if(index>=0 && index<channels.size())
    channels[index]=info;
  else // if not in known channels then add it to the preset info
    preset[info.name]=info;
}

// Helper method to avoid mutex recursion during initialization
int ChannelData::getChannelIndexNoLock(const ChannelName& name) const {
  if(channelindex.contains(name))
    return channelindex[name];
  else
    return -1;
}

// sets the channels (and initialized the data buffer)
void ChannelData::setChannels(const QStringList& newchannels){
  // qDebug() << "ChannelData::setChannels - Called in thread:" << QThread::currentThreadId();
  
  // No mutex needed for initialization case - we're still in constructor chain
  if (!initialized) {
    // qDebug() << "ChannelData::setChannels - Initializing channels (first time)";
    numchannels = newchannels.size();
    channels.resize(numchannels);
    int i=0;
    QRegExp vectorRE;
    vectorRE.setPatternSyntax(QRegExp::RegExp);
    vectorRE.setPattern(".+\\[\\d+\\]"); // regexp for a vector (e.g. v[0])
    QRegExp matrixRE;
    matrixRE.setPatternSyntax(QRegExp::RegExp);
    matrixRE.setPattern(".+\\[\\d+,\\d+\\]"); // regexp for a matrix (e.g. A[0,2])
    FOREACHC(QStringList, newchannels, n){
      if(preset.contains(*n)){
        channels[i] = preset[*n];
      } else {
        channels[i].name  = *n;
        channels[i].descr = "";
        channels[i].objectName = "";
        channels[i].type  = AutoDetection;
      }
      if(channels[i].type == AutoDetection){
        if(vectorRE.exactMatch(*n)){
          channels[i].type  = VectorElement;
        } else if(matrixRE.exactMatch(*n)){
          channels[i].type  = MatrixElement;
        } else {
          channels[i].type  = Single;
        }
      }
      channels[i].row = channels[i].column = 0; // will be initialized later
      channelindex[*n] = i;
      i++;
    }
    
    // fill multichannel info - NO RECURSION allowed
    multichannels.resize(numchannels); // we initialize with maximum number of multichannels
    int nummulti=0;
    for(i=0; i<numchannels; i++){
      // Must use direct call that doesn't recursively lock mutexes
      const MultiChannel& mc = extractMultiChannelDirect(&i);
      multichannels[nummulti]=mc;
      multichannelindex[mc.info.name]=nummulti; // store index in hash
      nummulti++;
    }

    multichannels.resize(nummulti); // cut down to the size of actual multichannels
    
    // initialize data buffer
    data.resize(buffersize);
    for(i=0; i<buffersize;i++){
      data[i].resize(numchannels);
    }
    
    // Mark as initialized before emitting signals
    initialized = true;
    
    // Now emit signals after initialization complete
    // qDebug() << "ChannelData::setChannels - Initialization complete, emitting signals";
    QMetaObject::invokeMethod(this, "channelsChangedDeferred", Qt::QueuedConnection);
    return;
  }
  
  // Using mutex for re-initialization (with existing channels)
  QMutexLocker locker(&dataMutex);
  
  if(newchannels.size() != channels.size()){ // this is not allowed so far
    qWarning() << "It is not allowed to reinitialize with new number of channels.";
  }
  
  // No actual work done in reinitialization case
}

// Deferred signal emission to avoid threading issues
void ChannelData::channelsChangedDeferred() {
  // qDebug() << "ChannelData::channelsChangedDeferred - Emitting signals in thread:" << QThread::currentThreadId();
  emit channelsChanged();
  emit update();
}

// Non-recursive multichannel extraction that doesn't use mutex locks
MultiChannel ChannelData::extractMultiChannelDirect(int* i){
  MultiChannel mc;
  int index = *i;
  QRegExp vectorRE;
  vectorRE.setPatternSyntax(QRegExp::RegExp);
  vectorRE.setPattern(".+\\[(\\d+)\\]"); // regexp for a vector (e.g. v[0])

  if(channels[index].type == Single){ // just a normal channel
    mc.info=channels[index];
    mc.startindex = index;
    mc.rows=1;
    mc.columns=1;
    mc.size=1;
  }else if(channels[index].type == MatrixElement
           || channels[index].type == VectorElement){ // A matrix or vector channel

    QString root = getChannelNameRootDirect(channels[index].name);
    QString rootwithbracket = root + "[";

    mc.info.name = root + "_";
    if(preset.contains(mc.info.name)){
      mc.info.descr= preset[mc.info.name].descr;
      mc.info.objectName= preset[mc.info.name].objectName;
    } else {
      mc.info.descr="";
      mc.info.objectName="";
    }
    mc.startindex = index;
    mc.columns = 1;
    mc.rows    = 1;

    if(channels[index].type == MatrixElement ){
      mc.info.type=Matrix;
      QRegExp matrixRE;
      matrixRE.setPatternSyntax(QRegExp::RegExp);
      matrixRE.setPattern(".+\\[(\\d+),(\\d+)\\]"); // regexp for a matrix (e.g. A[0,2])

      // scan through
      while(index<numchannels && channels[index].name.startsWith(rootwithbracket)){ // one pass is assured
        if(matrixRE.exactMatch(channels[index].name)){
          QStringList matches = matrixRE.capturedTexts();
          int col = matches[1].toInt();
          int row = matches[2].toInt();
          channels[index].row    = row;
          channels[index].column = col;
          if(channels[index].descr.isEmpty()) {
            channels[index].descr = (mc.info.descr.isEmpty() ? root : mc.info.descr)
              + QString(" elem %1,%2").arg(col+1).arg(row+1);
          }
          if(channels[index].objectName.isEmpty())
            channels[index].objectName = mc.info.objectName;
          mc.columns = mc.columns < col+1 ? col+1 : mc.columns;
          mc.rows    = mc.rows    < row+1 ? row+1 : mc.rows;
        } else {
          qWarning() << "error while parsing matrix element" << channels[index].name;
        }
        index++;
      }
    }else{// A vector channel
      QRegExp vectorRE;
      vectorRE.setPatternSyntax(QRegExp::RegExp);
      vectorRE.setPattern(".+\\[(\\d+)\\]"); // regexp for a matrix (e.g. v[0])
      mc.info.type=Vector;
      mc.columns=1;
      // scan through
      while(index<numchannels && channels[index].name.startsWith(rootwithbracket)){ // one pass is assured
        if(vectorRE.exactMatch(channels[index].name)){
          QStringList matches = vectorRE.capturedTexts();
          int row = matches[1].toInt();
          channels[index].row    = row;
          if(channels[index].descr.isEmpty()) {
            channels[index].descr = (mc.info.descr.isEmpty() ? root : mc.info.descr)
              + QString(" elem %1").arg(row+1);
          }
          if(channels[index].objectName.isEmpty())
            channels[index].objectName = mc.info.objectName;
          mc.rows    = mc.rows    < row+1 ? row+1 : mc.rows;
        } else {
          qWarning() << "error while parsing vector element" << channels[index].name;
        }
        index++;
      }
    }
    mc.size = *i - index;
    *i = index-1;
  } else {
    qWarning() << "Unknown channel type" << channels[index].type;
    mc.info = channels[index];
  }

  return mc;
}

QString ChannelData::getChannelNameRootDirect(const ChannelName& name) const {
  int ending = name.indexOf('[');
  if(ending>0)
    return name.left(ending);
  else
    return name;
}

// Always uses the mutex version
MultiChannel ChannelData::extractMultiChannel(int* i){
  QMutexLocker locker(&dataMutex);
  return extractMultiChannelDirect(i);
}

// Always uses the mutex version
QString ChannelData::getChannelNameRoot(const ChannelName& name) const {
  QMutexLocker locker(&dataMutex);
  return getChannelNameRootDirect(name);
}

/// returns the channel index (-1) if not found
int ChannelData::getChannelIndex(const ChannelName& name) const{
  // Skip mutex during initialization
  if (!initialized) {
    return getChannelIndexNoLock(name);
  }
  
  QMutexLocker locker(&dataMutex);
  return getChannelIndexNoLock(name);
}

const ChannelName& ChannelData::getChannelName(int index) const {
  // During initialization, access directly
  if (!initialized) {
    if(index>=0 && index<channels.size()){
      return channels[index].name;
    } else {
      return emptyChannelName;
    }
  }
  
  QMutexLocker locker(&dataMutex);
  if(index>=0 && index<channels.size()){
    return channels[index].name;
  } else {
    return emptyChannelName;
  }
}

int ChannelData::getMultiChannelIndex(const ChannelName& name) const {
  // Skip mutex during initialization
  if (!initialized) {
    if(multichannelindex.contains(name))
      return multichannelindex[name];
    else
      return -1;
  }
  
  QMutexLocker locker(&dataMutex);
  if(multichannelindex.contains(name))
    return multichannelindex[name];
  else
    return -1;
}

void ChannelData::setChannelDescription(const ChannelObjectName& objectName, const ChannelName& name, const ChannelDescr& description){
  // Skip mutex during initialization
  if (!initialized) {
    if(!preset.contains(name)){
      preset[name].name = name;
      preset[name].type = AutoDetection;
    }
    preset[name].descr = description;
    preset[name].objectName = objectName;
    return;
  }
  
  QMutexLocker locker(&dataMutex);
  int index = getChannelIndexNoLock(name);
  if(index>=0 && index < numchannels) {
    channels[index].descr = description;
    channels[index].objectName = objectName;
  }
  else {
    if(!preset.contains(name)){
      preset[name].name  = name;
      preset[name].type  = AutoDetection;
    }
    preset[name].descr = description;
    preset[name].objectName = objectName;
  }
}

// inserts a new set of data into the ring buffer
void ChannelData::setData(const QVector<double>& newdata){
  if(!initialized){
      qWarning() << "ChannelData::setData() called before initialization!";
      return;
  }
  
  QMutexLocker locker(&dataMutex);
  if(newdata.size() != numchannels) {
    qWarning() << "Number of data entries (" << newdata.size() << ") does not match number of channels (" << numchannels << ")";
  }else{
    time++;
    int index = time%buffersize;
    data[index] = newdata;
    // Optionally emit update signal here if plotting should happen immediately
    // emit update(); 
  }
}

/* returns the data of the given channels starting from history entries in the past.
    if history=0 then the entire history is given
*/
QVector<ChannelVals> ChannelData::getHistory(const IndexList& channels, int history) const {
  if (!initialized) {
    return QVector<ChannelVals>();
  }
  
  QMutexLocker locker(&dataMutex);
  QVector<ChannelVals> rv;
  int start = history ==0 ? time-(buffersize-1) : time-history;
  start = start < 0 ? 0 : start; // not below zero
  rv.resize(time-start);
  for(int i=start; i<time; i++){
    rv[i-start] = getDataNoLock(channels, i%buffersize);
  }
  return rv;
}

/* returns the data of the given channels starting from history entries in the past.
    if history=0 then the entire history is given
*/
QVector<ChannelVals> ChannelData::getHistory(const QList<ChannelName>& channels, int history) const {
  if (!initialized) {
    return QVector<ChannelVals>();
  }
  
  QMutexLocker locker(&dataMutex);
  QVector<ChannelVals> rv;
  int start = history ==0 ? time-buffersize : time-history;
  start = start < 0 ? 0 : start; // not below zero
  rv.resize(time-start);
  for(int i=start; i<time; i++){
    rv[i-start] = getDataNoLock(channels, i%buffersize);
  }
  return rv;
}

// Direct data access without mutex locking (for internal use only)
ChannelVals ChannelData::getDataNoLock(const IndexList& channels, int index) const {
  ChannelVals rv(channels.size());
  int i=0;
  FOREACHC(IndexList, channels, c){
    rv[i] = data[index][*c];
    i++;
  }
  return rv;
}

// Direct data access without mutex locking (for internal use only)
ChannelVals ChannelData::getDataNoLock(const QList<ChannelName>& channels, int index) const {
  ChannelVals rv(channels.size());
  int i=0;
  FOREACHC(QList<ChannelName>, channels, c){
    int k = channelindex[*c];
    rv[i] = data[index][k];
    i++;
  }
  return rv;
}

// returns the data of the given channel at the given index
ChannelVals ChannelData::getData(const IndexList& channels, int index) const {
  if (!initialized) {
    return ChannelVals();
  }
  
  QMutexLocker locker(&dataMutex);
  return getDataNoLock(channels, index);
}

// returns the data of the given channel at the given index
ChannelVals ChannelData::getData(const QList<ChannelName>& channels, int index) const {
  if (!initialized) {
    return ChannelVals();
  }
  
  QMutexLocker locker(&dataMutex);
  return getDataNoLock(channels, index);
}

void ChannelData::receiveRawData(QString data){
  QStringList parsedString = data.trimmed().split(' ', Qt::SkipEmptyParts); // Updated split
  if (parsedString.isEmpty()) {
      return;
  }
  
  QString& first = parsedString.first();
  if(first == "#C")   //Channels einlesen
    {
      // qDebug() << "ChannelData::receiveRawData() - Received #C (Channels)";
      parsedString.removeFirst();
      //transmit channels to GNUPlot
      QStringList channels;
      for(const QString& s : parsedString){
        channels.push_back(s.trimmed());
      }
      setChannels(channels);
      // qDebug() << "ChannelData::receiveRawData() - Set channels:" << channels;
    }
  else if(first == "#I")   // Info Lines
    {
      // qDebug() << "ChannelData::receiveRawData() - Received #I (Info)";
      parsedString.removeFirst();
      if (parsedString.isEmpty()) return;
      QString type = parsedString.first();
      if(!type.isEmpty() && (type == "D" || (type.startsWith("[")))){ // description
        // now we expect: [channelObjectName] D channelname description
        QString objectName = "";
        QString key = "";
        QString description = "";
        if (type.startsWith("[")) {
          // new style
          // 2 possibilities: [NameOfObject] [Name of Object] (with spaces)
          type.remove(0, 1); // eliminate [
          while (!type.endsWith("]")) {
            objectName.append(type).append(" ");
            parsedString.removeFirst();
            if (parsedString.isEmpty()) { qWarning("Malformed #I line"); return; }
            type = parsedString.first();
          }
          objectName.append(type.chopped(1)); // eliminate ]
          parsedString.removeFirst();
          if (parsedString.isEmpty() || parsedString.first() != "D") { qWarning("Malformed #I line, expected D"); return; }
          parsedString.removeFirst(); // remove D
          if (parsedString.isEmpty()) { qWarning("Malformed #I line, expected key"); return; }
          key = parsedString.first();
          parsedString.removeFirst();
          description = parsedString.join(" ");
          setChannelDescription(objectName, key, description);
        } else if (type == "D") {
          // D channelname description (old style, used e.g. for timestep t)
          parsedString.removeFirst(); // remove D
          if (parsedString.isEmpty()) { qWarning("Malformed #I D line, expected key"); return; }
          key = parsedString.first();
          parsedString.removeFirst();
          description = parsedString.join(" ");
          setChannelDescription(objectName, key, description);
        } else {
            qWarning() << "ChannelData::receiveRawData() - Unknown #I type:" << type;
        }
      }
    }
  else if (first == "#IN") { // name of PlotOption
    // qDebug() << "ChannelData::receiveRawData() - Received #IN (Root Name)";
    parsedString.removeFirst();
    if (!parsedString.isEmpty()) {
        QString name = parsedString.first();
        // qDebug() << "ChannelData::receiveRawData() - Root name update:" << name;
        emit rootNameUpdate(name);
    }
  } else if(first.length()>=2 &&  first[0] == '#' && first[1] == 'Q')   //Quit
    {
      // qDebug() << "ChannelData::receiveRawData() - Received #Q (Quit)";
      printf("Guilogger: Received Quit\n");
      emit quit();
    } else if (first.length() >=1 && first[0] == '#') {
        // Other comment line, ignore
    }
  else // Daten einlesen
    {
      if(!initialized) {
          qWarning() << "ChannelData::receiveRawData() - Received data before channels defined!";
          return;
      }
      
      QVector<double> dat(parsedString.size());
      bool ok = true;
      for(int i=0; i<parsedString.size(); ++i){
        dat[i] = parsedString[i].trimmed().toDouble(&ok);
        if (!ok) {
            qWarning() << "ChannelData::receiveRawData() - Failed to parse data value:" << parsedString[i];
            return; // Skip this line if data is invalid
        }
      }
      setData(dat);
    }
}

