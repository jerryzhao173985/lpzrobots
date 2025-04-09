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


#ifndef GUILOGGER_H
#define GUILOGGER_H

#define QUOTEIT(V) #V
#if defined(WIN32) || defined(_WIN32) || defined (__WIN32) || defined(__WIN32__) \
      || defined (_WIN64) || defined(__CYGWIN__) || defined(__MINGW32__)
#define VERSIONSTRING QUOTEIT(0.6-MS)
#else
#define VERSIONSTRING QUOTEIT(0.6)
#endif

// Qt5 includes
#include <QMainWindow>
#include <QMenu>
#include <QStringList>
#include <QLabel>
#include <QDialog>
#include <QLinkedList>
#include <QLayout>
#include <QStringList>
#include <QScrollArea>
//#include <QTimer>
#include <QMutex>
#include <QMap>
#include <QList>
#include <QBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QTextEdit>
#include <QTableView>

#include <list>
#include "gnuplot.h"
//#include <QUeue>
#include "inifile.h"
#include "commlineparser.h"
#include "plotchannelstablemodel.h"

class ChannelRow;
class ChannelSelectRow;
class QTimer;

typedef QVector<Gnuplot> PlotWindows;

/** \brief Base class for layout and all the visualisation stuff
 * \author Dominic Schneider
 */
class GuiLogger: public QMainWindow
{
  Q_OBJECT

  public:
  GuiLogger(const CommLineParser& configobj, const QRect& screenSize);
  ~GuiLogger();

  ChannelData& getChannelData() {return channelData;}

private slots:
  // called to notify that the user changed the channels to plot.
  void plotChannelsChanged(int window);

  void plotUpdate();
  /** updates plots:
      @param waitfordata if true then nothing is done unless new data was seen
      @param window which window to update. if -1 then all windows.
   */
  void plotUpdate(bool waitfordata, int window = -1);
  void save();
  void save(bool blank);
  void load();
  void editconfig();
  void dataSliderValueChanged(int value);
  void dataSliderReleased();
  void horizonSliderValueChanged(int value);
  void horizonSliderReleased();
  void sendButtonPressed();
  void doQuit();
  void updateRootName(QString name);

signals:
  void quit();

private:
  void updateSliderPlot();
  int  analyzeFile();

private:

  std::list<QString> inputbuffer;

  QBoxLayout* layout;
  QWidget* channelandslider;
  QBoxLayout* channelandsliderlayout;
  QBoxLayout* channellayout;
  QBoxLayout* commlayout;
  QBoxLayout* sendlayout;
  // QScrollArea* sv; // Already defined below
  QScrollArea* sv;
  QWidget* commWidget;
  QTableView* channelWidget;

  QTextEdit   *parameterlistbox;
  QLineEdit   *paramvaluelineedit;
  QPushButton *sendbutton;
  QSlider     *dataslider;
  QSlider     *horizonslider;
  QLabel      *dataslidervalue;
  QLabel      *horizonslidervalue;

  QMenu       *filemenu;


  QMap<int, QSize > windowsize;     // the window sizes from the cfg file are stored here
  QMap<int, QSize > windowposition; // the window positions from the cfg file are stored here
  QRect screenSize;                 // size of the screen for window positioning

  int plotwindows;
  int lastPlotTime;
  int datadelayrate;  // how much data traffic is neccessary to replot
  int filePlotHorizon; //

  QString gnuplotcmd;

  QString mode;
  QString filename;

  QTimer *plottimer;
  int startplottimer;

  QMutex queuemutex;

  IniFile cfgFile;

  /******** Data represenstation *********/

  /// stores data in channels
  ChannelData channelData;
  /// for each plotwindow we store here the selected channels and plotstyle
  QVector<PlotInfo*> plotInfos;
  /// windows (e.g. gnuplot) to actually show the data
  PlotWindows plotWindows;

  // Model
  PlotChannelsTableModel* tableModel;

};


#endif
