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


#include "plotchannelstablemodel.h"

int PlotChannelsTableModel::rowCount(const QModelIndex&) const {
  if(plotInfos && plotInfos->size() > 0){
    return (*plotInfos)[0]->getChannelData().getNumChannels()+rowoffset;
  } else
    return 0;
}

int PlotChannelsTableModel::columnCount(const QModelIndex& /*parent */) const {
  if(plotInfos)
    return plotInfos->size()+2; // two additional column for description and object name
  else
    return 0;
}

QVariant PlotChannelsTableModel::data(const QModelIndex &index, int role) const {
  //printf("data: %i, %i\n",index.row(),index.column());

  if (!index.isValid() || !plotInfos || plotInfos->size() == 0)
    return QVariant();

  if (index.column() > plotInfos->size()+1)
    return QVariant();

  const ChannelData& cd = (*plotInfos)[0]->getChannelData();
  int row = index.row()-rowoffset;
  if (row >= cd.getNumChannels())
    return QVariant();

  // description in last-1 column
  if (index.column() == plotInfos->size()){
    if(role == Qt::DisplayRole){
      switch(index.row()){
      case 0: return "Enable/disable window";
      case 1: return "Channel to use as x-axis";
      default:
        if(row<cd.getInfos().size())
          return cd.getInfos()[row].descr;
        else
          return QVariant();
      }
    }else
      return QVariant();
  }
  // objectName in last column
  else if (index.column() == plotInfos->size()+1) {
   if(role == Qt::DisplayRole){
      switch(index.row()){
        case 0: return "                              ";
        case 1: return QVariant();
        default:
          if(row<cd.getInfos().size()) {
            return cd.getInfos()[row].objectName;
          }
          else
            return QVariant();
      }
    }else
      return QVariant();
  }

  // any normal column
  if(index.row()==0){ // enable/disable row
    if(role == Qt::CheckStateRole){
      return (*plotInfos)[index.column()]->getIsVisible() ? Qt::Checked : Qt::Unchecked;
    }else
      return QVariant();
  }else if(index.row()==1){ // reference1 row
    if(role == Qt::DisplayRole){
      return (*plotInfos)[index.column()]->getReference1Name();
    }else
      return QVariant();
  }

  // also normal row
// if (role == Qt::DisplayRole)
//     return QVariant((*plotInfos)[index.column()]->getChannelShow(index.row()));

  if(role == Qt::CheckStateRole){
    return (*plotInfos)[index.column()]->getChannelShow(row) ? Qt::Checked : Qt::Unchecked;
  } else
    return QVariant();
}

QVariant PlotChannelsTableModel::headerData(int section, Qt::Orientation orientation,
                                            int role) const {
  if (role != Qt::DisplayRole)
    return QVariant();

  if (orientation == Qt::Horizontal){
    if(section<plotInfos->size())
      return QString("  %1  ").arg(section+1);
    else if (section<plotInfos->size()+1)
      return QString("Description");
    else
      return QString("Object");
  }  else {
    if(section==0){
      return "Enable";
    }else if(section==1){
      return "Reference";
    } else
      return (*plotInfos)[0]->getChannelData().getChannelName(section-rowoffset);
    // QString("Channel %1").arg(section);
  }
}


Qt::ItemFlags PlotChannelsTableModel::flags(const QModelIndex &index) const {
  if (!index.isValid())
    return Qt::ItemIsEnabled;
  if(index.column()==plotInfos->size()){ // last column is description
    return QAbstractItemModel::flags(index) | Qt::ItemIsEnabled;
  }else if(index.row()==0){
    return QAbstractItemModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
  }else if(index.row()==1){
    return QAbstractItemModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsEditable;
  } else
    //  return QAbstractItemModel::flags(index) | Qt::ItemIsUserCheckable | Qt::ItemIsEditable;
    return QAbstractItemModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
}

bool PlotChannelsTableModel::setData(const QModelIndex &index, const QVariant &value, int role){
//   if (index.isValid() && role == Qt::EditRole) {
//     (*plotInfos)[index.column()]->setChannelShow(index.row(), value.toBool());
//     emit dataChanged(index, index);
//     return true;
//   }else


  if (!index.isValid() || index.column()>=plotInfos->size()) return false;
  if(index.row()==0){
    if(role == Qt::CheckStateRole){
      (*plotInfos)[index.column()]->setIsVisible(value.toBool());
      return true;
    }
  } else if(index.row()==1){
    if( role == Qt::EditRole){
      (*plotInfos)[index.column()]->setReference1(value.toString());
      return true;
    }
  } else{
    if( role == Qt::CheckStateRole) {
      int row = index.row() - rowoffset;
      (*plotInfos)[index.column()]->setChannelShow(row, value.toBool());
      emit dataChanged(index, index);
      emit updateWindow(index.column());
      return true;
    }
  }
  return false;
}


void PlotChannelsTableModel::update(){
  beginResetModel();
  endResetModel();
  //   QModelIndex start = index(0,0);
  //   QModelIndex end = index(columnCount(),rowCount());
  //   printf("udpate: %i, %i\n",columnCount(),rowCount());
  //  emit dataChanged(start,end);
}
