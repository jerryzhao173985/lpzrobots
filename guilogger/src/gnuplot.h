// -*- C++ -*-
/* 
   \file gnuplot.h
   simple C++ interface to gnuplot
*/

#ifndef GNUPLOT_H
#define GNUPLOT_H

#include <QString>
#include <QProcess>

#include "plotinfo.h"

/** \defgroup baseclasses Base classes
 */
/// \ingroup baseclasses

/**
   Open a Gnuplot window and updates the last n values of channels defined
    in PlotInfo structure.
*/

class Gnuplot {
public: 
  Gnuplot() : plotInfo(0), process(nullptr), windowWidth(400), windowHeight(300), windowX(-1), windowY(-1) {}
  Gnuplot(const PlotInfo* plotinfo);
  
  ~Gnuplot();

  bool init(const QString& gnuplotcmd, int w=400,int h=300, int x=-1, int y=-1);
  
  bool open(const QString& gnuplotcmd, int w=400,int h=300, int x=-1, int y=-1);

  void close();
  

  /** send arbitrary command to gnuplot.
      like "set zeroaxis" or other stuff */
  void command(const QString& cmd);


  /** make gnuplot plot selected content of data buffers */
  void plot(int historyLimit);

  /** creates the plot command
      if file is empty then the stdin is assumed ('-') and no using are given
   */
  QString plotCmd(const QString& file=QString(), int start=-1, int end=-1);
    
private:
  /** Try different terminal types until one works */
  void tryTerminals(int w, int h, int x, int y);
  
  const PlotInfo* plotInfo;
  QProcess* process;
  int windowWidth;
  int windowHeight;
  int windowX;
  int windowY;
};

#endif

