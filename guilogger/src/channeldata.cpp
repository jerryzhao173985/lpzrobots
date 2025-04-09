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

ChannelData::ChannelData(int buffersize)
  : numchannels(0), buffersize(0), time(0), initialized(false){
  setBufferSize(buffersize);

}

int ChannelData::getBuffersize(){
  return buffersize;
}

void ChannelData::setBufferSize(int newBuffersize){
  buffersize=newBuffersize;
  if(initialized){
    data.resize(buffersize);
    for(int i=0; i<buffersize;i++){
      data[i].resize(numchannels);
    }
    time = 0;
    emit(update());
  }
}


/// sets a new information about a channel (also works before initialization)
void ChannelData::setChannelInfo(const ChannelInfo& info){
  if(!initialized){
    preset[info.name]=info;
  } else {
    int index = getChannelIndex(info.name);
    if(index>=0 && index<channels.size())
      channels[index]=info;
    else // if not in known channels then add it to the preset info
      preset[info.name]=info;
  }
}

// sets the channels (and initialized the data buffer)
void ChannelData::setChannels(const QStringList& newchannels){
  // if we are allready initialized we do nothing (reinitialization not really supported)
  if(initialized){
    if(newchannels.size() != channels.size()){ // this is not allowed so far
      fprintf(stderr,"It is not allowed to reinitialize with new number of channels.");
    }
    return;
  }else{
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
        //        fprintf(stderr, "Ch %s : %i\n", n->toLatin1().data(), channels[i].type);
      }
      channels[i].row = channels[i].column = 0; // will be initialized later
      channelindex[*n] = i;
      i++;
    }
    // fill multichannel info
    multichannels.resize(numchannels); // we initialize with maximum number of multichannels
    int nummulti=0;
    for(int i=0; i<numchannels; i++){
      const MultiChannel& mc = extractMultiChannel(&i);
      multichannels[nummulti]=mc;
      multichannelindex[mc.info.name]=nummulti; // store index in hash
      nummulti++;
    }

    multichannels.resize(nummulti); // cut down to the size of actual multichannels
    // initialize data buffer
    data.resize(buffersize);
    for(int i=0; i<buffersize;i++){
      data[i].resize(numchannels);
    }
    initialized = true;
    emit channelsChanged();
    emit update();
  }
}


MultiChannel ChannelData::extractMultiChannel(int* i){
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

    QString root = getChannelNameRoot(channels[index].name);
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
          fprintf(stderr, "error while parsing matrix element %s!",
                  channels[index].name.toLatin1().data());
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
          fprintf(stderr, "error while parsing vector element %s!",
                  channels[index].name.toLatin1().data());
        }
        index++;
      }
    }
    mc.size = *i - index;
    *i = index-1;
  } else {
    fprintf(stderr, "Unknown channel type (%i) !\n", channels[index].type);
    mc.info = channels[index];
  }

  return mc;
}

QString ChannelData::getChannelNameRoot(const ChannelName& name) const {
  int ending = name.indexOf('[');
  if(ending>0)
    return name.left(ending);
  else
    return name;
}

/// returns the channel index (-1) if not found
int ChannelData::getChannelIndex(const ChannelName& name) const{
  if(channelindex.contains(name))
    return channelindex[name];
  else
    return -1;
}

const ChannelName& ChannelData::getChannelName(int index) const {
  if(index>=0 && index<channels.size()){
    return channels[index].name;
  }else{
    return emptyChannelName;
  }

}


int ChannelData::getMultiChannelIndex(const ChannelName& name) const {
  if(multichannelindex.contains(name))
    return multichannelindex[name];
  else
    return -1;
}


void ChannelData::setChannelDescription(const ChannelObjectName& objectName, const ChannelName& name, const ChannelDescr& description){
  if(initialized){
    int index = getChannelIndex(name);
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
  }else{
    // store in preset information
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
  if(newdata.size() != numchannels) {
    fprintf(stderr,"Number of data entries (%i) does not match number of channels (%i)",
            newdata.size(),numchannels);
  }else{
    time++;
    int index = time%buffersize;
    data[index] = newdata;
  }
}

/* returns the data of the given channels starting from history entries in the past.
    if history=0 then the entire history is given
*/
QVector<ChannelVals> ChannelData::getHistory(const IndexList& channels, int history) const {
  QVector<ChannelVals> rv;
  int start = history ==0 ? time-(buffersize-1) : time-history;
  start = start < 0 ? 0 : start; // not below zero
  rv.resize(time-start);
  for(int i=start; i<time; i++){
    rv[i-start] = getData(channels,i%buffersize);
  }
  return rv;
}

/* returns the data of the given channels starting from history entries in the past.
    if history=0 then the entire history is given
*/
QVector<ChannelVals> ChannelData::getHistory(const QList<ChannelName>& channels, int history) const {
  QVector<ChannelVals> rv;
  int start = history ==0 ? time-buffersize : time-history;
  start = start < 0 ? 0 : start; // not below zero
  rv.resize(time-start);
  for(int i=start; i<time; i++){
    rv[i-start] = getData(channels,i%buffersize);
  }
  return rv;
}

// returns the data of the given channel at the given index
ChannelVals ChannelData::getData(const IndexList& channels, int index) const {
  ChannelVals rv(channels.size());
  int i=0;
  FOREACHC(IndexList, channels, c){
    rv[i] = data[index][*c];
    i++;
  }
  return rv;
}

// returns the data of the given channel at the given index
ChannelVals ChannelData::getData(const QList<ChannelName>& channels, int index) const {
  ChannelVals rv(channels.size());
  int i=0;
  FOREACHC(QList<ChannelName>, channels, c){
    int k = channelindex[*c];
    rv[i] = data[index][k];
    i++;
  }
  return rv;
}


void ChannelData::receiveRawData(QString data){
  QStringList parsedString = data.trimmed().split(' ');  //parse data string with Space as separator
  QString& first = *(parsedString.begin());
  if(first == "#C")   //Channels einlesen
    {
      // printf("GuiLogger:: got channels!\n");
      parsedString.erase(parsedString.begin());
      //transmit channels to GNUPlot
      QStringList channels;
      FOREACH(QStringList, parsedString, s){
        channels.push_back((*s).trimmed());
      }
      setChannels(channels);
    }
  else if(first == "#I")   // Info Lines
    {
      parsedString.erase(parsedString.begin());
      QString type = *(parsedString.begin());
      if(!type.isEmpty() && (type == "D" || (type.startsWith("[")))){ // description
        // now we expect: [channelObjectName] D channelname description
        if (type.startsWith("[")) {
          // new style
          // 2 possibilities: [NameOfObject] [Name of Object] (with spaces)
          QString objectName = "";
          type = type.right(type.size()-1); // eliminate [
          while (!type.endsWith("]")) {
            objectName.append(type).append(" ");
            parsedString.erase(parsedString.begin());
            type = *(parsedString.begin());
          }
          objectName.append(type.left(type.size()-1)); // eliminate ]
          parsedString.erase(parsedString.begin());
          parsedString.erase(parsedString.begin());
          QString key = *(parsedString.begin());
          parsedString.erase(parsedString.begin());
          setChannelDescription(objectName, key, parsedString.join(" "));
        } else {
          // D channelname description (old style, used e.g. for timestep t)
          QString objectName = "";
          parsedString.erase(parsedString.begin());
          QString key = *(parsedString.begin());
          parsedString.erase(parsedString.begin());
          setChannelDescription(objectName, key, parsedString.join(" "));
        }
      }
    }
  else if (first == "#IN") { // name of PlotOption
    parsedString.erase(parsedString.begin());
    QString name = *(parsedString.begin());
    emit rootNameUpdate(name);
  } else if(first.length()>=2 &&  first[0] == '#' && first[1] == 'Q')   //Quit
    {
      printf("Guilogger: Received Quit\n");
      emit quit();
    }
  else if( first[0] != '#')  // Daten einlesen
    {
      QVector<double> dat(parsedString.size());
      int i=0;

      FOREACH(QStringList, parsedString, s){
        dat[i] = (*s).trimmed().toDouble();
        i++;
      }
      setData(dat);
    }
}

