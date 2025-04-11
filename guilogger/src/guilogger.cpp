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
#include "guilogger.h"

#include "stl_adds.h"

#include <utility>
#include <QThread>
#include <QApplication>
#include <QMenuBar>
#include <QTimer>
#include <QRegularExpression>
#include <QScrollArea>
#include <QDebug>
#include <QPointer>

#include "quickmp.h"


GuiLogger::GuiLogger(const CommLineParser& configobj, const QRect& screenSize)
  : QMainWindow( 0), screenSize(screenSize), channelData(0) {
  // qDebug() << "GuiLogger::GuiLogger() - Constructor START in thread:" << QThread::currentThreadId();
  setWindowTitle(QString("Guilogger-") + QString(VERSIONSTRING));
  mode     = configobj.getMode();
  filename = configobj.getFile();

  // Make sure ChannelData is created and initialized before connecting signals
  // qDebug() << "GuiLogger::GuiLogger() - Creating ChannelData instance";
  
  // Connect signals after creating ChannelData object
  connect(&channelData, SIGNAL(quit()), this, SLOT(doQuit()));
  connect(&channelData, SIGNAL(rootNameUpdate(QString)), this, SLOT(updateRootName(QString)));

  load();  // load Config File

  lastPlotTime = 0;
  int linecount = 0;
  const int maxplottimer = 5000;
  filePlotHorizon = 500;

  if(mode == "file" && !filename.isEmpty()) {
      qDebug() << "GuiLogger::GuiLogger() - Analyzing file:" << filename;
      linecount = analyzeFile();
      qDebug() << "GuiLogger::GuiLogger() - File analysis complete, linecount:" << linecount;
  }

  setCentralWidget(new QWidget(this));
  layout           = new QVBoxLayout(centralWidget());
  channelandslider = new QWidget(centralWidget());
  QSizePolicy sp1(QSizePolicy::Preferred, QSizePolicy::Preferred);
  sp1.setHorizontalStretch(0);
  sp1.setVerticalStretch(10);
  channelandslider->setSizePolicy(sp1);



  layout->addWidget(channelandslider);
  channelandsliderlayout = new QHBoxLayout(channelandslider);

  sv = new QScrollArea(centralWidget());
  channelandsliderlayout->addWidget(sv);
  //    sv = new QScrollArea(centralWidget());
  // sv->setWidgetResizable(true);
  //    channelWidget = new QWidget(sv->viewport());
  //  channelWidget = new QWidget(channelandslider);

  // create Model for channel - plotwindow association
  tableModel = new PlotChannelsTableModel(&plotInfos, channelandslider);
  connect(&channelData, SIGNAL(update()), tableModel, SLOT(update()));
  connect(tableModel, SIGNAL(updateWindow(int)), this, SLOT(plotChannelsChanged(int)));
  // create View for channel - plotwindow association
  channelWidget = new QTableView(channelandslider);
  channelWidget->setModel(tableModel);
  channelWidget->resizeColumnsToContents();
  channelWidget->setSelectionBehavior(QAbstractItemView::SelectItems);
  channelWidget->setSelectionMode(QAbstractItemView::SingleSelection);

  // I tried to get a tree view working but it took too much time. see descarded folder
//   // create Model for channel - plotwindow association
//   treeModel = new PlotChannelsTreeModel(&plotInfos, channelandslider);
//   connect(&channelData, SIGNAL(update()), treeModel, SLOT(update()));
//   connect(treeModel, SIGNAL(updateWindow(int)), this, SLOT(plotChannelsChanged(int)));
//   // create View for channel - plotwindow association
//   channelTreeWidget = new QTreeView(channelandslider);
//   channelTreeWidget->setModel(treeModel);
//   //channelTreeWidget->resizeColumnsToContents();
//   channelTreeWidget->setSelectionBehavior(QAbstractItemView::SelectItems);
//   channelTreeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
//   channelandsliderlayout->addWidget(channelTreeWidget);

  // // create editor for reference
//   QItemEditorFactory *editorFactory = new QItemEditorFactory;
//   QItemEditorCreatorBase *creator = new QStandardItemEditorCreator<QCombobox>();
//   editorFactory->registerEditor(QVariant::String, creator);

  sv->setWidget(channelWidget);
  sv->setWidgetResizable(true);

  //  channelandsliderlayout->addWidget(channelWidget);

  // sv->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
  channelWidget->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));

  commWidget = new QWidget(centralWidget());
  QSizePolicy sp2(QSizePolicy::Preferred, QSizePolicy::Minimum);
  sp2.setHorizontalStretch(1);
  sp2.setVerticalStretch(1);
  commWidget ->setSizePolicy(sp2);

  layout->addWidget(commWidget);

  QWidget *horizonsliderwidget = new QWidget(channelandslider);
  channelandsliderlayout->addWidget(horizonsliderwidget);

  QWidget *datasliderwidget = nullptr;
  if(mode == "file") {
    datasliderwidget = new QWidget(channelandslider);
    channelandsliderlayout->addWidget(datasliderwidget);
  }

  channellayout = new QVBoxLayout(channelWidget);
  commlayout    = new QHBoxLayout(commWidget);


  QVBoxLayout *horizonsliderlayout = new QVBoxLayout(horizonsliderwidget);
  horizonslider = new QSlider(Qt::Vertical, horizonsliderwidget);
  horizonslidervalue = new QLabel(horizonsliderwidget);

  if(mode == "file") {
    horizonslider->setInvertedAppearance(true);
    horizonslider->setMaximum(linecount);
    horizonslider->setValue(filePlotHorizon);
    horizonsliderlayout->addWidget(new QLabel("Win", horizonsliderwidget));
    horizonslidervalue->setText(QString::number(filePlotHorizon, 10));
  } else {
    horizonslider->setMinimum(10);
    horizonslider->setMaximum(maxplottimer);   // MaxTimer for Plottimer
    horizonslider->setValue(startplottimer);       // actual Value for Plottimer
    horizonsliderlayout->addWidget(new QLabel("Interval", horizonsliderwidget));
    horizonslidervalue->setText(QString("%1\nms").arg(startplottimer));
  }
  horizonsliderlayout->addWidget(horizonslider);
  horizonsliderlayout->addWidget(horizonslidervalue);
  connect(horizonslider, SIGNAL(valueChanged(int )), this, SLOT(horizonSliderValueChanged(int )));
  connect(horizonslider, SIGNAL(sliderReleased())  , this, SLOT(horizonSliderReleased()));

  if(mode=="file"){
    QBoxLayout *datasliderlayout = new QVBoxLayout(datasliderwidget);
    dataslider = new QSlider(Qt::Vertical, datasliderwidget);
    dataslider->setInvertedAppearance(true);
    dataslidervalue = new QLabel(datasliderwidget);
    dataslider->setMinimum(0);
    dataslider->setMaximum(linecount);
    dataslidervalue->setText(QString::number(0, 10));
    datasliderlayout->addWidget(new QLabel("Data", datasliderwidget));
    datasliderlayout->addWidget(dataslider);
    datasliderlayout->addWidget(dataslidervalue);
    connect(dataslider, SIGNAL(valueChanged(int )), this, SLOT(dataSliderValueChanged(int )));
    connect(dataslider, SIGNAL(sliderReleased())  , this, SLOT(dataSliderReleased()));
  }

  parameterlistbox   = new QTextEdit(commWidget);
  parameterlistbox->setMaximumHeight(60);
  parameterlistbox->setLineWrapMode(QTextEdit::NoWrap);

  paramvaluelineedit = new QLineEdit(commWidget);
  sendbutton         = new QPushButton("Send Cmd",commWidget);
  connect(sendbutton, SIGNAL(released())  , this, SLOT(sendButtonPressed()));

  //  parameterlistbox->setTextFormat(Qt::LogText);

  sendlayout = new QVBoxLayout();
  commlayout->addLayout(sendlayout);
  sendlayout->addWidget(paramvaluelineedit);
  sendlayout->addWidget(sendbutton);

  commlayout->addWidget(parameterlistbox);

  filemenu = new QMenu("&Menu", this);
  menuBar()->addMenu(filemenu);
  filemenu->addAction("&Save Config", this, SLOT(save()));
  filemenu->addAction("&Edit Config", this, SLOT(editconfig()));
  filemenu->addAction("&Load Config", this, SLOT(load()));
  filemenu->addAction("&Exit", this, SLOT(doQuit()));


  if(mode == "file") {
    resize( 520, 600 );
    // updateSliderPlot(); // Don't update plot yet, gnuplot not ready
    // Schedule Gnuplot initialization after event loop starts
    QTimer::singleShot(0, this, SLOT(initializeGnuplotWindows())); 
  } else {
    // For pipe/serial mode, start timer and schedule gnuplot init
    plottimer = new QTimer( this);
    resize( 520, 600 );
    connect(plottimer, SIGNAL(timeout()), SLOT(plotUpdate()));
    plottimer->setSingleShot(false);
    // Start timer only AFTER gnuplot windows are initialized
    // plottimer->start(startplottimer); 
    QTimer::singleShot(0, this, SLOT(initializeGnuplotWindows())); 
    // qDebug() << "GuiLogger::GuiLogger() - Plot timer ready, interval:" << startplottimer;
  }
  // qDebug() << "GuiLogger::GuiLogger() - Constructor END (Gnuplot init deferred)";

}


GuiLogger::~GuiLogger() {
  // qDebug() << "GuiLogger::~GuiLogger() - Destructor called.";
  // Ensure PlotInfo objects are deleted
  qDeleteAll(plotInfos);
  plotInfos.clear(); // Clear the vector after deleting contents

  // Delete Gnuplot instances directly
  // qDebug() << "GuiLogger::~GuiLogger() - Deleting" << plotWindows.size() << "Gnuplot instances.";
  qDeleteAll(plotWindows);
  plotWindows.clear();
  // qDebug() << "GuiLogger::~GuiLogger() - Gnuplot instances deleted.";

}

void GuiLogger::horizonSliderReleased() {
  // qDebug() << "GuiLogger::horizonSliderReleased()";
  updateSliderPlot();
  if(mode != "file") // in this case this slider shows the time
    plottimer->setInterval(filePlotHorizon);  // change Plotintervall
}

void GuiLogger::dataSliderValueChanged(int value) {
  dataslidervalue->setText(QString::number(value, 10));
}

void GuiLogger::horizonSliderValueChanged(int value) {
  if(mode=="file"){
    horizonslidervalue->setText(QString::number(value, 10));
  } else {
    horizonslidervalue->setText(QString("%1\nms").arg(value));
  }
  filePlotHorizon = value;
}



void GuiLogger::sendButtonPressed() {
  qDebug() << "GuiLogger::sendButtonPressed()";
  QString cmd = paramvaluelineedit->text();
  for(int i=0; i<plotWindows.size(); ++i) {
      if (plotWindows.at(i)) { // Check if pointer is valid
          plotWindows.at(i)->command(cmd);
      }
  }
}

void GuiLogger::dataSliderReleased() {
  // qDebug() << "GuiLogger::dataSliderReleased()";
  updateSliderPlot();
}


void GuiLogger::updateSliderPlot() {
  // qDebug() << "GuiLogger::updateSliderPlot()";
  if(mode!="file") return;
  int start = dataslider->value();

  parameterlistbox->clear();
  parameterlistbox->append("set style data lines");
  parameterlistbox->append("set zeroaxis");


  for(int i=0; i<plotWindows.size(); ++i) {
    if (!plotWindows.at(i)) continue;
    QString cmd = plotWindows.at(i)->plotCmd(filename,start,start+filePlotHorizon);
    if(!cmd.isEmpty()){
      plotWindows.at(i)->command(cmd);
      parameterlistbox->append(cmd);
    }
  }
}


/// analyzes the file, send channels and return number of lines with data
int GuiLogger::analyzeFile() {
  // qDebug() << "GuiLogger::analyzeFile() START";
  char *s=NULL;
  int buffersize=0;
  char c;
  int size=1, i=1;
  int linecount=0;

  FILE *instream;

  instream = fopen(filename.toLatin1(), "r");
  if(instream == NULL)
    {   qWarning() << "Cannot open input file:" << filename;
      return 0;
    }
  bool channelline=false;
  while(!channelline){
    size=1;
    c=0;
    while(c!= 10 && c != 13)
      {
        i = fread(&c, 1, 1, instream);
        if(i==1) {
          if(c== 10 || c == 13) break;
          size++;
          if(size>=buffersize){
            buffersize=buffersize*2+1;
            s=(char*)realloc(s, buffersize);
          }
          s[size-2] = c;
        } else{
          channelline=true;
          break;
        }
      }
    if(size>1){
      s[size-1]='\0';
      channelData.receiveRawData(QString(s).trimmed());
      if (s[0] == '#' && s[1] == 'C') channelline=true;
    }
  }

  do
    {   i = fread(&c, 1, 1, instream);
      if(i!=1) break;
      if(c == 10) linecount++;  // count only lines with data
    } while(i==1);

  fclose(instream);

  if(s != NULL) free(s);
  qDebug() << "GuiLogger::analyzeFile() END, lines:" << linecount;
  return linecount;
}


void GuiLogger::save(){
  qDebug() << "GuiLogger::save()";
  save(false);
}

/** saves the channel configuration to file
    if blank is used then a basic file is written without the window stuff
*/
void GuiLogger::save(bool blank){
  QString nr;
  IniSection *section;

  cfgFile.setFilename("guilogger.cfg");

  // delete all "window" sections, because they will be rewritten in the next "for loop".
  int i = 0;
  while(i < cfgFile.sections.size()) {
    section = cfgFile.sections.at(i);
    if(section->getName() == "Window") {
      delete section;
      cfgFile.sections.removeAt(i);  // remove current item
    } else if(blank && (section->getName() == "General" || section->getName() == "GNUPlot" ||
                   section->getName() == "Misc")) {
      delete section;
      cfgFile.sections.removeAt(i);  // remove current item
    } else {
      i++;
    }
  }

  // If we don't have a General section then add it.
  IniSection sec;
  if(!cfgFile.getSection(sec,"General",false)){
    cfgFile.setComment("# Please restart guilogger to apply changes you make in the file");
    section=cfgFile.addSection("General");
    section->addValue("Version",VERSIONSTRING, " # do not change!");
    section->addValue("PlotWindows","5");
    section->addValue("CalcPositions","yes"," # If yes then the gnuplot windows will be placed according to the layout");
    section->addValue("WindowLayout","blh"," # From where to start: t: top, b: bottom, l: left, r: right, h: horizontal, v: vertical");
    section->addValue("WindowsPerRowColumn","3");

    section->addValue("UpdateInterval","2000"," # time between plotting updates in ms");
    section->addValue("MinData4Replot","1"," # number of input events before updating plots");
    section->addValue("BufferSize","250", " # Size of history");

#if defined(WIN32) || defined(_WIN32) || defined (__WIN32) || defined(__WIN32__) \
      || defined (_WIN64) || defined(__CYGWIN__) || defined(__MINGW32__)
    section->addValue("Gnuplot","C:/msys/1.0/gnuplot/binary/gnuplot");
#else
    section->addValue("Gnuplot","gnuplot");
#endif

  }
  // If we don't have a GNUPlot section then add it.
  if(!cfgFile.getSection(sec,"GNUPlot",false)){
    section=cfgFile.addSection("GNUPlot");
#if defined(WIN32) || defined(_WIN32) || defined (__WIN32) || defined(__WIN32__) \
      || defined (_WIN64) || defined(__CYGWIN__) || defined(__MINGW32__)
    section->addValue("Command","set terminal wxt");
#else
    section->addValue("Command","set terminal x11");
#endif
    section->addValue("Command","set style data lines");
    section->addValue("Command","set zeroaxis");
  }

  if(1){ // !blank
    for(int i=0; i<plotInfos.size(); i++){
      nr = QString::number(i, 10);
      IniSection *sec = cfgFile.addSection("Window");
      sec->addComment("# you can also set the size and the position of a window. The channels can also contain wildcards like x* or C[0]*");
      sec->addValue("Number", nr);
      if(!plotInfos[i]->getIsVisible()){
        sec->addValue("Disabled", "yes");
      }
      QString ref = channelData.getChannelName(plotInfos[i]->getReference1());
      if(!ref.isEmpty()){
        sec->addValue("Reference1", ref);
      }
      ref = channelData.getChannelName(plotInfos[i]->getReference2());
      if(!ref.isEmpty()){
        sec->addValue("Reference2", ref);
      }
      if(windowsize.contains(i)){
        QSize s = windowsize[i];
        sec->addValue("Size",QString("%1x%2").arg(s.width()).arg(s.height()));
      } else
        sec->addValue("Size", "400x300");

      if(windowposition.contains(i)){
        QSize pos = windowposition[i];
        sec->addValue("Position",QString("%1 %2").arg(pos.width()).arg(pos.height())," # set to \"-1 -1\" to make it automatically set by your windowmanager. If calcPositions is used then this is ignored");
      }else{
        sec->addValue("Position", "-1 -1"," # set to any coordinate to place a window by hand, (-1 -1) means automatic. If calcPositions then this is ignored");
      }
      FOREACHC(std::list<int>, plotInfos[i]->getVisibleChannels(), c){
        sec->addValue("Channel", channelData.getChannelName(*c));
        // todo maybe add style as well
      }
    }
  }
  cfgFile.Save();
}


/// loads the channel configuration from file
void GuiLogger::load() {
  qDebug() << "GuiLogger::load() - Starting config load.";
  int pwin;
  QString qv;

  pwin = -1;

  cfgFile.setFilename("guilogger.cfg");
  if(!cfgFile.Load()){
    qWarning() << "Guilogger: Configuration file guilogger.cfg does not exist, creating default.";
    save(true); // this automatically creates the config (on disk and in memory)
  } else {
    qDebug() << "Guilogger::load() - Config file loaded.";
  }

  // load general settings
  if(cfgFile.getValueDef("General","Version","").trimmed() != VERSIONSTRING) {
    qWarning() << "Guilogger: Configuration file has incorrect version, creating new one.";
    save(true);// this automatically creates the config (on disk and in memory)
  }
  plotwindows = cfgFile.getValueDef("General","PlotWindows","5").toInt();
  startplottimer = cfgFile.getValueDef("General","UpdateInterval","2000").toInt();
  datadelayrate = cfgFile.getValueDef("General","MinData4Replot","1").toInt();

  gnuplotcmd    = cfgFile.getValueDef("General","Gnuplot","gnuplot");
  qDebug() << "GuiLogger::load() - General settings loaded. PlotWindows:" << plotwindows << "Gnuplot cmd:" << gnuplotcmd;

  // Resize PlotInfo vector
  if (plotInfos.size() < plotwindows) {
    int oldSize = plotInfos.size();
    plotInfos.resize(plotwindows);
    for (int i = oldSize; i < plotwindows; ++i) {
      plotInfos[i] = new PlotInfo(channelData);
      connect(&channelData, &ChannelData::channelsChanged, plotInfos[i], &PlotInfo::channelsChanged);
    }
  } else if (plotInfos.size() > plotwindows) {
    qDebug() << "GuiLogger::load() - Resizing plotInfos down from" << plotInfos.size() << "to" << plotwindows;
    for (int i = plotwindows; i < plotInfos.size(); ++i) {
      delete plotInfos[i];
    }
    plotInfos.resize(plotwindows);
  }

  // qDebug() << "GuiLogger::load() - Loading window settings...";
  // load window settings
  for(const auto* section : cfgFile.sections) {
    if(section->getName() == "Window"){
      pwin = -1;
      int current_pwin = -1;
      for(const auto* var : section->vars) {
        qv = var->getValue().trimmed();
        if(var->getName() == "Number") {
          bool ok;
          current_pwin = qv.toInt(&ok);
          if (!ok || current_pwin < 0 || current_pwin >= plotwindows) {
            qWarning() << "Guilogger: Invalid or out-of-bounds window number" << qv << "in config. Skipping section.";
            current_pwin = -1;
            break;
          }
          pwin = current_pwin;
        }
        else if (pwin != -1) {
          if(var->getName() == "Disabled") {
            plotInfos[pwin]->setIsVisible(qv != "yes");
          } else if(var->getName() == "Reference1") {
            plotInfos[pwin]->setReference1(qv);
          } else if(var->getName() == "Reference2") {
            plotInfos[pwin]->setReference2(qv);
          } else if(var->getName() == "Channel") {
            plotInfos[pwin]->setChannelShow(qv,true);
          } else if(var->getName() == "Size") {
            int x,y;
            QRegularExpression sizeRegex("(\\d+)x(\\d+)");
            QRegularExpressionMatch match = sizeRegex.match(qv);
            if(match.hasMatch()) {
              x = match.captured(1).toInt();
              y = match.captured(2).toInt();
              windowsize.insert(pwin, QSize(x,y));
            } else {
              qWarning() << "Guilogger: Invalid size format" << qv << "for window" << pwin;
            }
          } else if(var->getName() == "Position") {
            int w,h;
            QRegularExpression posRegex("(-?\\d+)\\s+(-?\\d+)");
            QRegularExpressionMatch match = posRegex.match(qv);
            if(match.hasMatch()){
              w = match.captured(1).toInt();
              h = match.captured(2).toInt();
              windowposition.insert(pwin, QSize(w,h));
            } else {
              qWarning() << "Guilogger: Invalid position format" << qv << "for window" << pwin;
            }
          }
        }
      }
    }
  }
  // qDebug() << "GuiLogger::load() - Window settings loaded.";

  // load and calculate positioning
  QString calcPositions = cfgFile.getValueDef("General","CalcPositions","yes");
  QString windowLayout = cfgFile.getValueDef("General","WindowLayout","blh");
  int windowsPerRowColumn = cfgFile.getValueDef("General","WindowsPerRowColumn","3").toInt();
  if(calcPositions.contains("yes")){
    // qDebug() << "GuiLogger::load() - Calculating window positions...";
    // arrange Gnuplot windows
    int xstart = windowLayout.contains("l") ? 0 : screenSize.width();
    int xinc   = windowLayout.contains("l") ? 1 : -1;
    int ystart = windowLayout.contains("t") ? 0 : screenSize.height();
    int yinc   = windowLayout.contains("t") ? 1 : -1;
    bool hor   = windowLayout.contains("h");
    int xpos   = xstart;
    int ypos   = ystart;
    for(int k=0; k<plotwindows; k++) {
      QSize s = (windowsize.contains(k) ? windowsize[k] : QSize(400,300)) + QSize(10,50);
      if(hor){
        xpos = xstart+(k%windowsPerRowColumn)*xinc*s.width();
        ypos = ystart+(k/windowsPerRowColumn)*yinc*s.height();
      }else{
        xpos = xstart+(k/windowsPerRowColumn)*xinc*s.width();
        ypos = ystart+(k%windowsPerRowColumn)*yinc*s.height();
      }
      windowposition.insert(k,QSize(xinc > 0 ? xpos : xpos-s.width(),yinc > 0 ? ypos : ypos-s.height()));
    }
    // qDebug() << "GuiLogger::load() - Window positions calculated.";
  }


  // qDebug() << "GuiLogger::load() - Setting up Gnuplot instances (NO INIT YET)...";
  // --- Direct Gnuplot Instance Setup --- 
  qDebug() << "GuiLogger::load() - Deleting existing Gnuplot instances (if any). Size:" << plotWindows.size();
  qDeleteAll(plotWindows); // Delete existing instances
  plotWindows.clear();

  plotWindows.resize(plotwindows);

  for (int i = 0; i < plotwindows; ++i) {
      // qDebug() << "GuiLogger::load() - Creating Gnuplot instance pointer for window" << i;
      plotWindows[i] = new Gnuplot(plotInfos[i]); // Create directly
  }
  // ------------------------------------
  // qDebug() << "GuiLogger::load() - Gnuplot instance pointers setup complete.";

  // --- REMOVED gnuplot init and command sending from here --- 

  channelData.setBufferSize(cfgFile.getValueDef("General","BufferSize","250").toInt());
  qDebug() << "Guilogger: Config file loading finished (Gnuplot instances created but not initialized).";
}


void GuiLogger::editconfig() {
  qDebug() << "GuiLogger::editconfig()";
  if(system("$EDITOR ./guilogger.cfg") == -1) {
    qWarning() << "Failed to launch editor for config file";
  }
}


void GuiLogger::doQuit(){
  qDebug() << "GuiLogger::doQuit()";
  emit quit();
}

void GuiLogger::updateRootName(QString name) {
  setWindowTitle("GUI Logger - " + name);
}


void GuiLogger::plotChannelsChanged(int window){
  // qDebug() << "GuiLogger::plotChannelsChanged() for window:" << window;
  if(mode=="file")
    updateSliderPlot();
  else
    plotUpdate(false, window);
}

// updates every n milliseconds the GNUPlot windows
void GuiLogger::plotUpdate(){
  // This might be too verbose if the timer interval is short
  // qDebug() << "GuiLogger::plotUpdate() - Timer tick";
  plotUpdate(true);
}

void GuiLogger::plotUpdate(bool waitfordata, int window)
{
  // qDebug() << "GuiLogger::plotUpdate() - wait:" << waitfordata << "window:" << window << "time diff:" << (channelData.getTime() - lastPlotTime);
  if(mode=="file") return;

  if(!waitfordata || channelData.getTime() - lastPlotTime > datadelayrate){
    // qDebug() << "GuiLogger::plotUpdate() - Updating plot windows. Window:" << window;
    if(window==-1){
      // serial version
      for(int i=0; i<plotWindows.size(); i++) {
          if (plotWindows.at(i)) {
            //  qDebug() << "GuiLogger::plotUpdate() - Calling plot() for window" << i;
             // Pass filePlotHorizon as the history limit
             plotWindows.at(i)->plot(filePlotHorizon); 
          }
      }
    } else {
      if(window >=0 && window < plotWindows.size() && plotWindows.at(window)) {
        //  qDebug() << "GuiLogger::plotUpdate() - Calling plot() for window" << window;
         // Pass filePlotHorizon as the history limit
        plotWindows.at(window)->plot(filePlotHorizon);
      }
    }
    if(waitfordata) lastPlotTime=channelData.getTime();
  }
}

// --- NEW SLOT --- 
void GuiLogger::initializeGnuplotWindows() {
    // qDebug() << "GuiLogger::initializeGnuplotWindows() - START";

    bool anyWindowInitialized = false;
    int initializedCount = 0;

    //open and position plotwindows
    for(int k=0; k<plotWindows.size(); ++k) {
        if (!plotWindows.at(k)) {
            qWarning() << "GuiLogger::initializeGnuplotWindows() - Skipping init for null Gnuplot instance at index" << k;
            continue;
        }
        QSize s = windowsize.contains(k) ? windowsize[k] : QSize(400,300);
        int x = -1, y = -1;
        if(windowposition.contains(k)){
          QSize pos = windowposition[k];
          x = pos.width();
          y = pos.height();
        }
        // Call init directly
        // qDebug() << "GuiLogger::initializeGnuplotWindows() - Calling init for window" << k << "Size:" << s << "Pos:" << x << y;
        if (plotWindows.at(k)->init(gnuplotcmd, s.width(), s.height(), x, y)) {
            anyWindowInitialized = true;
            initializedCount++;
        } else {
            qWarning() << "GuiLogger::initializeGnuplotWindows() - Failed to initialize window" << k;
        }
    }
    
    if (initializedCount > 0) {
        qDebug() << "GuiLogger::initializeGnuplotWindows() - Gnuplot window init complete. Successfully initialized:" 
                 << initializedCount << "of" << plotWindows.size() << "windows.";
    } else {
        qWarning() << "GuiLogger::initializeGnuplotWindows() - Failed to initialize ANY gnuplot windows!";
        
        // Try with fallback options
        qDebug() << "GuiLogger::initializeGnuplotWindows() - Trying fallback initialization...";
        for(int k=0; k<plotWindows.size(); ++k) {
            if (!plotWindows.at(k)) continue;
            
            // Try with minimal options
            if (plotWindows.at(k)->init("gnuplot", 400, 300, -1, -1)) {
                anyWindowInitialized = true;
                initializedCount++;
                qDebug() << "GuiLogger::initializeGnuplotWindows() - Fallback init succeeded for window" << k;
            }
        }
        
        if (initializedCount == 0) {
            qCritical() << "GuiLogger::initializeGnuplotWindows() - All gnuplot initialization attempts failed!";
            // Continue anyway - the plotting functions will check if processes are running
        }
    }

    // qDebug() << "GuiLogger::initializeGnuplotWindows() - Sending initial Gnuplot commands...";
    // send gnuplot commands (after init)
    IniSection GNUplotsection;
    if(cfgFile.getSection(GNUplotsection,"GNUPlot",false)){
        QString qv;
        for(const auto* var : GNUplotsection.vars) {
          qv = var->getValue();
          if(var->getName() == "Command") {
            qDebug() << "GuiLogger::initializeGnuplotWindows() - Found initial command:" << qv;
            for(int k=0; k<plotWindows.size(); ++k) {
                if (!plotWindows.at(k)) continue; // Check instance exists
                // Call command directly
                // qDebug() << "GuiLogger::initializeGnuplotWindows() - Sending initial command for window" << k << "Cmd:" << qv;
                plotWindows.at(k)->command(qv);
            }
          }
        }
    }
    // qDebug() << "GuiLogger::initializeGnuplotWindows() - Initial Gnuplot commands sent.";

    // If in file mode, trigger initial plot update now
    if (mode == "file") {
        qDebug() << "GuiLogger::initializeGnuplotWindows() - Triggering initial updateSliderPlot for file mode.";
        updateSliderPlot();
    } else {
        // If using timer, start it now
        if (plottimer) {
            qDebug() << "GuiLogger::initializeGnuplotWindows() - Starting plot timer.";
            plottimer->start(startplottimer);
        } else {
            qWarning() << "GuiLogger::initializeGnuplotWindows() - Plot timer is null!";
        }
    }
    // qDebug() << "GuiLogger::initializeGnuplotWindows() - END";
}
