//
//  PLP - An implementation of the PSION link protocol
//
//  Copyright (C) 1999  Philip Proudman
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//  e-mail philip.proudman@btinternet.com


#include <stdio.h>

#include "bufferstore.h"
#include "bufferarray.h"

bufferArray::bufferArray() {
  len = 0;
  lenAllocd = 5;
  buff = new bufferStore [lenAllocd];
}

bufferArray::bufferArray(const bufferArray &a) {
  len = a.len;
  lenAllocd = a.lenAllocd;
  buff = new bufferStore [lenAllocd];
  for (int i=0; i < len; i++) buff[i] = a.buff[i];
}

bufferArray::~bufferArray() {
  delete [] buff;
}

bufferStore bufferArray::popBuffer() {
  bufferStore ret;
  if (len > 0) {
    ret = buff[0];
    len--;
    for (long i=0; i<len; i++) {
      buff[i] = buff[i+1];
    }
  }
  return ret;
}

void bufferArray::pushBuffer(const bufferStore &b) {
  if (len == lenAllocd) {
    lenAllocd += 5;
    bufferStore* nb = new bufferStore [lenAllocd];
    for (long i=0; i<len; i++) {
      nb[i] = buff[i];
    }
    delete [] buff;
    buff = nb;
  }
  buff[len++] = b;
}
