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

#include <stream.h>
#include <iomanip.h>
#include <string.h>

#include "bufferstore.h"

bufferStore::bufferStore() {
  lenAllocd = 0;
  buff = NULL;
  len = 0;
  start = 0;
}

bufferStore::bufferStore(const bufferStore &a) {
  lenAllocd = (a.getLen() > MIN_LEN) ? a.getLen() : MIN_LEN;
  buff = new unsigned char [lenAllocd];
  len = a.getLen();
  for (long i=0; i<len; i++) buff[i] = a.getByte(i);
  start = 0;
}

bufferStore::bufferStore(const unsigned char*_buff, long _len) {
  lenAllocd = (_len > MIN_LEN) ? _len : MIN_LEN;
  buff = new unsigned char [lenAllocd];
  len = _len;
  for (long i=0; i<len; i++) buff[i] = _buff[i];
  start = 0;
}

void bufferStore::operator =(const bufferStore &a) {
  checkAllocd(a.getLen());
  len = a.getLen();
  for (long i=0; i<len; i++) buff[i] = a.getByte(i);
  start = 0;
}

void bufferStore::init() {
  start = 0;
  len = 0;
}

void bufferStore::init(const unsigned char*_buff, long _len) {
  checkAllocd(_len);
  start = 0;
  len = _len;
  for (long i=0; i<len; i++) buff[i] = _buff[i];
}

bufferStore::~bufferStore() {
  delete [] buff;
}

unsigned long bufferStore::getLen() const {
  if (start > len) return 0;
  return len - start;
}

unsigned char bufferStore::getByte(long pos) const {
  return buff[pos+start];
}

unsigned int bufferStore::getWord(long pos) const {
  return buff[pos+start] + (buff[pos+start+1] << 8);
}

unsigned int bufferStore::getDWord(long pos) const {
  return buff[pos+start] +
    (buff[pos+start+1] << 8) +
    (buff[pos+start+2] << 16) +
    (buff[pos+start+3] << 24);
}

const char* bufferStore::getString(long pos) const {
  return (const char*)buff+pos+start;
}

ostream &operator<<(ostream &s, const bufferStore &m) {
  {
    for (int i = m.start; i < m.len; i++)
      s << hex << setw(2) << setfill('0') << (int)m.buff[i] << " ";
  }
  s << "(";
  {
    for (int i = m.start; i < m.len; i++) {
      unsigned char c = m.buff[i];
      if (c>=' ' && c <= 'z') s << c;
    }
  }
  s<< ")";
  return s;
}

void bufferStore::discardFirstBytes(int n) {
  start += n;
  if (start > len) start = len;
}

void bufferStore::checkAllocd(long newLen) {
  if (newLen >= lenAllocd) {
    do {
      if (lenAllocd < MIN_LEN)
	lenAllocd = MIN_LEN;
      else
	lenAllocd *= 2;
    } while (newLen >= lenAllocd);
    unsigned char* newBuff = new unsigned char [lenAllocd];
    for (int i=start; i<len; i++) newBuff[i] = buff[i];
    delete [] buff;
    buff = newBuff;
  }
}

void bufferStore::addByte(unsigned char cc) {
  checkAllocd(len + 1);
  buff[len++] = cc;
}

void bufferStore::addString(const char* s) {
  checkAllocd(len + strlen(s));
  for (int i=0; s[i]; i++) buff[len++] = s[i];
}

void bufferStore::addStringT(const char* s) {
  addString(s);
  addByte(0);
}

void bufferStore::addBuff(const bufferStore &s, long maxLen) {
  checkAllocd(len + s.getLen());
  for (unsigned long i=0; i < s.getLen() && (maxLen < 0 || i < (unsigned long)maxLen); i++) buff[len++] = s.getByte(i);
}

void bufferStore::addWord(int a) {
  checkAllocd(len + 2);
  buff[len++] = a & 0xff;
  buff[len++] = (a>>8) & 0xff;
}

void bufferStore::addDWord(long a) {
  checkAllocd(len + 4);
  buff[len++] = a & 0xff;
  buff[len++] = (a>>8) & 0xff;
  buff[len++] = (a>>16) & 0xff;
  buff[len++] = (a>>24) & 0xff;  
}
