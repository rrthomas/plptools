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

#include <qpainter.h>

#include "pie3dwidget.h"


Pie3DWidget::Pie3DWidget(QWidget *parent, const char *name) : QWidget(parent, name),
                         				      _totalsize(0)
{
  _piecelist.setAutoDelete(true);
  _piecelist.clear();
}


void  Pie3DWidget::addPiece(int size, const QColor& color)
{
  _totalsize += size;

  _piecelist.append(new Pie3DPiece(size, color));

  repaint();
}


int  Pie3DWidget::heightForWidth(int w) const
{
  return (int) (w*0.6);
}


QSize  Pie3DWidget::minimumSizeHint() const
{
  return QSize(60, 40);
}


QSize  Pie3DWidget::sizeHint() const
{
  return QSize(width(), (int)(width()*0.6));
}


/*
 * Protected methods
 ********************/

void  Pie3DWidget::paintEvent(QPaintEvent *ev)
{
  QPainter  p;
  QColor    widgetbg = palette().normal().background();
  QColor    black    = QColor(black);
  int       w        = width();
  int       h        = height();
  int       pieh     = h/4;
  int       halfrot  = 180*16;
  int       fullrot  = 360*16;
  int       bowpos   = 0;
  int       i, bowlen, bowcut;

  if (_piecelist.isEmpty()) return;

  p.begin(this);
  p.setClipRegion(ev->region());

  for (Pie3DPiece *piece = _piecelist.first(); piece; piece = _piecelist.next())
  {
    QPalette  piecepal(piece->color(), widgetbg);

    bowlen = (int) (((double) piece->size())/_totalsize*fullrot);

    p.setPen((_piecelist.count() > 1) ? black : _piecelist.first()->color());
    p.setBrush(piecepal.normal().button());
    p.drawPie(0, 0, w, h-pieh, bowpos, bowlen);

    if (bowpos+bowlen >= halfrot)	// Part of the footer is visible
    {
      bowcut  = (bowpos < halfrot) ? halfrot-bowpos : 0;
      bowpos += bowcut;
      bowlen -= bowcut;

      p.setPen(piecepal.normal().mid());

      for (i = 0; i < pieh; i++) p.drawArc(0, i, w, h-pieh, bowpos, bowlen);
    }

    bowpos += bowlen;
  }

  p.setPen(black);

  p.drawArc(0,    0,          w,   h-pieh, 0,  fullrot);
  p.drawArc(0,    pieh-1,     w,   h-pieh, 0, -halfrot);

  p.drawLine(0,   (h-pieh)/2, 0,   (h+pieh)/2-1);
  p.drawLine(w-1, (h-pieh)/2, w-1, (h+pieh)/2-1);

  p.end();
}

