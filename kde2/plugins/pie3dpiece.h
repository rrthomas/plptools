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
 ****************************************************************************/

#ifndef __PIE3DPIECE_H
#define __PIE3DPIECE_H

#include <qcolor.h>


class Pie3DPiece
{
  public:
  
    Pie3DPiece(int size, const QColor&);
    Pie3DPiece() {}
    
          int      size()  const { return _size;  }
    const QColor&  color() const { return _color; }
    
  private:
  
    int     _size;
    QColor  _color;
};


#endif

