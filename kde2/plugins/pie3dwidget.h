/*
 *  This file is part of the KDE System Control Tool,
 *  Copyright (C)1999 Thorsten Westheider <twesthei@physik.uni-bielefeld.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Pie3DWidget was inspired by Christian Poulter's KDiskFree
 * 
 ****************************************************************************/

#ifndef __PIE3DWIDGET_H
#define __PIE3DWIDGET_H

#include <qwidget.h>
#include <qlist.h>

#include "pie3dpiece.h"


class Pie3DWidget : public QWidget
{
  public:
  
    Pie3DWidget(QWidget *parent = 0L, const char *name = 0L);
    ~Pie3DWidget() {}
    
            void   addPiece(int size, const QColor&);
            
    virtual int    heightForWidth(int w) const;
    virtual QSize  minimumSizeHint()     const;
    virtual QSize  sizeHint()            const;
  
  protected:
  
    virtual void   paintEvent(QPaintEvent *);
    
  private:
  
    int                _totalsize;
    QList<Pie3DPiece>  _piecelist;
};


#endif
