/***************************************************************************
 *   Copyright (C) 2008-2011 LpzRobots development team                    *
 *    Antonia Siegert (original author)                                  *
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

#ifndef __ABSTRACTVISUALISATION_H_
#define __ABSTRACTVISUALISATION_H_

class AbstractPlotChannel;

#include "MatrixPlotChannel.h"
#include "VectorPlotChannel.h"
#include "ColorPalette.h"
#include <QGLWidget>

#include <iostream>


class AbstractVisualisation: public QGLWidget {

  Q_OBJECT

public:
  AbstractVisualisation(MatrixPlotChannel *channel, ColorPalette *colorPalette, QWidget *parent = 0);
  virtual ~AbstractVisualisation();

protected:

  MatrixPlotChannel *channel;
  ColorPalette *colorPalette;


  virtual void initializeGL() = 0;
  virtual void resizeGL(int w, int h) = 0;
  virtual void paintGL() = 0;

private:

};


#endif /* __ABSTRACTVISUALISATION_H_ */
