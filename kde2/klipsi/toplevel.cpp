/*-*-c++-*-
 * $Id$
 *
 * This file is part of plptools.
 *
 *  Copyright (C) 1999-2001 Fritz Elfert <felfert@to.com>
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include "toplevel.h"

#include <qclipboard.h>
#include <qmenudata.h>
#include <qpainter.h>
#include <qtimer.h>

#include <kaction.h>
#include <kapp.h>
#include <klocale.h>
#include <kwin.h>
#include <kiconloader.h>
#include <knotifyclient.h>
#include <kdebug.h>


#define QUIT_ITEM    50
#define CLIPFILE "C:/System/Data/Clpboard.cbd"

TopLevel::TopLevel()
  : KMainWindow(0)
{
    KNotifyClient::startDaemon();

    clip = kapp->clipboard();
    menu = new KPopupMenu(0, "main_menu");
    timer = new QTimer();

    rfsvSocket = 0;
    rclipSocket = 0;
    rf = 0;
    rc = 0;
    rff = 0;
    inSend = false;
    inSetting = false;
    mustListen = true;
    lastClipData = "";
    state = ENABLED;
    constate = DISCONNECTED;

    menu->insertTitle(kapp->miniIcon(), i18n("Klipsi - Psion Clipboard"));
    menu->insertSeparator();
    menu->insertItem(SmallIcon("exit"), i18n("&Quit"), QUIT_ITEM);

    connect(menu, SIGNAL(activated(int)), this, SLOT(slotMenuSelected(int)));
    connect(clip, SIGNAL(dataChanged()), this, SLOT(slotClipboardChanged()));
    connect(timer, SIGNAL(timeout()), this, SLOT(slotTimer()));

    icons[ENABLED][DISCONNECTED] =
	KGlobal::iconLoader()->loadIcon("klipsi", KIcon::Toolbar);
    icons[ENABLED][CONNECTED] =
	KGlobal::iconLoader()->loadIcon("klipsi_c", KIcon::Toolbar);
    icons[DISABLED][DISCONNECTED] =
	KGlobal::iconLoader()->loadIcon("klipsi_d", KIcon::Toolbar);
    icons[DISABLED][CONNECTED] =
	KGlobal::iconLoader()->loadIcon("klipsi_cd", KIcon::Toolbar);

    icon = &icons[state][constate];
    resize(icon->size());

    setBackgroundMode(X11ParentRelative);

    int interval = checkConnection() ? 500 : 5000;
    timer->start(interval, true);


}

TopLevel::~TopLevel()
{
    closeConnection();
    delete timer;
    delete menu;
}

void TopLevel::
closeConnection() {
    if (rf)
	delete(rf);
    if (rc)
	delete(rc);
    if (rff)
	delete rff;
    rfsvSocket = 0;
    rclipSocket = 0;
    rf = 0;
    rc = 0;
    rff = 0;
    mustListen = true;
    constate = DISCONNECTED;
    repaint();
}

void TopLevel::
mousePressEvent(QMouseEvent *e)
{
    if (e->button() == RightButton)
        showPopupMenu(menu);
    if (e->button() == LeftButton) {
	state = (state == ENABLED) ? DISABLED : ENABLED;
	repaint();
    }
}

void TopLevel::
paintEvent(QPaintEvent *)
{
    QPainter p(this);
    icon = &icons[state][constate];

    int x = (width() - icon->width()) / 2;
    int y = (height() - icon->height()) / 2;
    if ( x < 0 ) x = 0;
    if ( y < 0 ) y = 0;
    p.drawPixmap(x , y, *icon);
    p.end();
}

void TopLevel::
slotTimer()
{
    Enum<rfsv::errs> res;

    if (inSend) {
	timer->start(500, true);
	return;
    }

    if (!checkConnection()) {
	timer->start(5000, true);
	return;
    }

    if (state == DISABLED) {
	timer->start(500, true);
	return;
    }

    if (mustListen) {
	res = rc->sendListen();
	if (res != rfsv::E_PSI_GEN_NONE) {
	    closeConnection();
	    timer->start(5000, true);
	    return;
	} else
	    mustListen = false;
    }

    if ((res = rc->checkNotify()) != rfsv::E_PSI_GEN_NONE) {
	if (res != rfsv::E_PSI_FILE_EOF) {
	    closeConnection();
	    timer->start(5000, true);
	    return;
	}
    } else {
	getClipData();
	mustListen = true;
    }
    timer->start(500, true);
}

void TopLevel::
slotClipboardChanged()
{
    if (mustListen || inSetting || (state == DISABLED))
	return;

    Enum<rfsv::errs> res;

    QString clipData = clip->text();
    if (clipData.isEmpty() || (clipData == lastClipData))
        return;

    if (!checkConnection())
	return;

    lastClipData = clipData;

    inSend = true;
    mustListen = true;
    char *p = strdup(clipData.latin1());
    ascii2PsiText(p, clipData.length());
    putClipData(p);
    free(p);
    res = rc->notify();
    inSend = false;

    if (res != rfsv::E_PSI_GEN_NONE)
	closeConnection();
}

void TopLevel::
slotMenuSelected(int id)
{
    switch (id) {
	case QUIT_ITEM:
	    kapp->quit();
	    break;
    }
}


void TopLevel::
showPopupMenu(QPopupMenu *menu)
{
    ASSERT( menu != 0L );

    // Update menu geometry
    menu->move(-1000,-1000);
    menu->show();
    menu->hide();

    QPoint g = QCursor::pos();
    if ( menu->height() < g.y() )
	menu->popup(QPoint( g.x(), g.y() - menu->height()));
    else
	menu->popup(QPoint(g.x(), g.y()));
}

void TopLevel::
psiText2ascii(char *buf, int len) {
    char *p;

    for (p = buf; len; len--, p++)
	switch (*p) {
	    case 6:
	    case 7:
		*p = '\n';
		break;
	    case 8:
		*p = '\f';
		break;
	    case 10:
		*p = '\t';
		break;
	    case 11:
	    case 12:
		*p = '-';
		break;
	    case 15:
	    case 16:
		*p = ' ';
		break;
	}
}

void TopLevel::
ascii2PsiText(char *buf, int len) {
    char *p;

    for (p = buf; len; len--, p++)
	switch (*p) {
	    case '\n':
		*p = 6;
		break;
	    case '\f':
		*p = 8;
		break;
	    case '-':
		*p = 11;
		break;
	}
}

void TopLevel::
putClipData(char *data) {
    Enum<rfsv::errs> res;
    u_int32_t fh;
    u_int32_t l;
    const unsigned char *p;
    bufferStore b;

    res = rf->freplacefile(0x200, CLIPFILE, fh);
    if (res == rfsv::E_PSI_GEN_NONE) {
	while ((res = rc->checkNotify()) != rfsv::E_PSI_GEN_NONE) {
	    if (res != rfsv::E_PSI_FILE_EOF) {
		rf->fclose(fh);
		closeConnection();
		return;
	    }
	}

	// Base Header
	b.addDWord(0x10000037);   // @00 UID 0
	b.addDWord(0x1000003b);   // @04 UID 1
	b.addDWord(0);            // @08 UID 3
	b.addDWord(0x4739d53b);   // @0c Checksum the above

	// Section Table
	b.addDWord(0x00000014);   // @10 Offset of Section Table
	b.addByte(2);             // @14 Section Table, length in DWords
	b.addDWord(0x10000033);   // @15 Section Type (ASCII)
	b.addDWord(0x0000001d);   // @19 Section Offset

	// Data
	b.addDWord(strlen(data)); // @1e Section (String) length
	b.addStringT(data);       // @22 Data

	p = (const unsigned char *)b.getString(0);
	rf->fwrite(fh, p, 0x22 + strlen(data), l);
	rf->fclose(fh);
	rf->fsetattr(CLIPFILE, 0x20, 0x07);
    } else
	closeConnection();
}

#define splitByte(v)                                \
do {                                                \
    int j;                                          \
                                                    \
    if (x < bytesPerLine)                           \
	for (j = 0; j < pixelsPerByte; j++) {       \
	    if (j && ((oidx % xPixels) == 0))       \
		break;                              \
	    else                                    \
              if (oidx >= picsize)                  \
		return 0;                           \
	    else {                                  \
		out.addByte((v & mask) * grayVal);  \
                v >>= bitsPerPixel;                 \
		oidx++;                             \
	    }                                       \
	}                                           \
    if (++x >= linelen)                             \
	x = 0;                                      \
} while (0)

QImage *TopLevel::
decode_image(unsigned char *p)
{
    bufferStore out;
    u_int32_t totlen = *((u_int32_t*)p); p += 4;
    u_int32_t hdrlen = *((u_int32_t*)p); p += 4;
    u_int32_t datlen = totlen - hdrlen;
    u_int32_t xPixels = *((u_int32_t*)p); p += 4;
    u_int32_t yPixels = *((u_int32_t*)p); p += 4;
    u_int32_t xTwips = *((u_int32_t*)p); p += 4;
    u_int32_t yTwips = *((u_int32_t*)p); p += 4;
    u_int32_t bitsPerPixel = *((u_int32_t*)p); p += 4;
    u_int32_t unknown1 = *((u_int32_t*)p); p += 4;
    u_int32_t unknown2 = *((u_int32_t*)p); p += 4;
    u_int32_t RLEflag = *((u_int32_t*)p); p += 4;

    QString hdr = QString("P5\n%1 %2\n255\n").arg(xPixels).arg(yPixels);
    out.addString(hdr.latin1());

    u_int32_t picsize = xPixels * yPixels;
    u_int32_t linelen;
    int pixelsPerByte = (8 / bitsPerPixel);
    int nColors = 1 << bitsPerPixel;
    int grayVal = 255 / (nColors - 1);
    int bytesPerLine = (xPixels + pixelsPerByte - 1) / pixelsPerByte;
    int mask = (bitsPerPixel << 1) - 1;

    int oidx = 0;
    int x = 0;
    int y = 0;
    int offset = 0;

    if (RLEflag) {
	int i = 0;

	while (offset < datlen) {
	    unsigned char b = *(p + offset);
	    if (b >= 0x80) {
		offset += 0x100 - b + 1;
		i += 0x100 - b;
	    } else {
		offset += 2;
		i += b + 1;
	    }
	}
	linelen = i / yPixels;
	offset = 0;
	while (offset < datlen) {
	    unsigned char b = *(p + offset++);
	    if (b >= 0x80) {
		for (i = 0; i < 0x100 - b; i++, offset++) {
		    if (offset >= datlen)
			return 0; // data corrupted
		    unsigned char b2 = *(p + offset);
		    splitByte(b2);
		}
	    } else {
		if (offset >= datlen)
		    return 0;
		else {
		    unsigned char b2 = *(p + offset);
		    unsigned char bs = b2;
		    for (i = 0; i <= b; i++) {
			splitByte(b2);
			b2 = bs;
		    }
		}
		offset++;
	    }
	}
    } else {
	linelen = datlen / yPixels;
	while (offset < datlen) {
	    unsigned char b = *(p + offset++);
	    splitByte(b);
	}
    }
    QImage *img = new QImage(xPixels, yPixels, 8);
    if (!img->loadFromData((const uchar *)out.getString(0), out.getLen())) {
	delete img;
	img = 0L;
    }
    return img;
}

void TopLevel::
getClipData() {
    Enum<rfsv::errs> res;
    PlpDirent de;
    u_int32_t fh;
    QString clipText;
    QImage *clipImg = 0L;

    res = rf->fgeteattr(CLIPFILE, de);
    if (res == rfsv::E_PSI_GEN_NONE) {
	if (de.getAttr() & rfsv::PSI_A_ARCHIVE) {
	    u_int32_t len = de.getSize();
	    char *buf = (char *)malloc(len);

	    if (!buf) {
		cerr << "Out of memory in getClipData" << endl;
		return;
	    }
	    res = rf->fopen(rf->opMode(rfsv::PSI_O_RDONLY | rfsv::PSI_O_SHARE),
			   CLIPFILE, fh);
	    if (res == rfsv::E_PSI_GEN_NONE) {
		u_int32_t tmp;
		res = rf->fread(fh, (unsigned char *)buf, len, tmp);
		rf->fclose(fh);

		if ((res == rfsv::E_PSI_GEN_NONE) && (tmp == len)) {
		    char *p = buf;
		    int lcount;
		    u_int32_t     *ti = (u_int32_t*)buf;

		    // Check base header
		    if (*ti++ != 0x10000037) {
			free(buf);
			return;
		    }
		    if (*ti++ != 0x1000003b) {
			free(buf);
			return;
		    }
		    if (*ti++ != 0) {
			free(buf);
			return;
		    }
		    if (*ti++ != 0x4739d53b) {
			free(buf);
			return;
		    }

		    // Start of section table
		    p = buf + *ti;
		    // Length of section table (in DWords)
		    lcount = *p++;

		    u_int32_t *td = (u_int32_t*)p;
		    while (lcount > 0) {
			u_int32_t sType = *td++;
			if (sType == 0x10000033) {
			    // An ASCII section
			    p = buf + *td;
			    len = *((u_int32_t*)p);
			    p += 4;
			    psiText2ascii(p, len);
			    clipText += (char *)p;
			}
			if (sType == 0x1000003d) {
			    // A paint data section
			    p = buf + *td;
			    if (clipImg)
				delete clipImg;
			    clipImg = decode_image((unsigned char *)p);
			}
			td++;
			lcount -= 2;
		    }
		}

	    } else
		closeConnection();
	    free(buf);
	}
    } else
	closeConnection();

    if (!clipText.isEmpty()) {
	inSetting = true;
	clip->setText(clipText);
	inSetting = false;
	KNotifyClient::event("data received");
    } else if (clipImg) {
	inSetting = true;
	clip->setImage(*clipImg);
	inSetting = false;
	KNotifyClient::event("data received");
    }
}

bool TopLevel::
checkConnection() {
    if (rf && rc)
	return true;

    Enum<rfsv::errs> res;
    int sockNum = DPORT;

    struct servent *se = getservbyname("psion", "tcp");
    endservent();
    if (se != 0L)
        sockNum = ntohs(se->s_port);

    if (!rfsvSocket) {
	rfsvSocket = new ppsocket();
	if (!rfsvSocket->connect(NULL, sockNum)) {
	    delete rfsvSocket;
	    rfsvSocket = 0;
	    return false;
	}
    }

    if (!rclipSocket) {
	rclipSocket = new ppsocket();
	if (!rclipSocket->connect(NULL, sockNum)) {
	    delete rclipSocket;
	    rclipSocket = 0;
	    return false;
	}
    }

    if (!rff)
	rff = new rfsvfactory(rfsvSocket);

    if (!rf)
	rf = rff->create(true);

    if (rf) {
	if (!rc) {
	    rc = new rclip(rclipSocket);
	    if (rc->initClipbd() == rfsv::E_PSI_GEN_NONE) {
		KNotifyClient::event("connected");
		constate = CONNECTED;
		repaint();
		return true;
	    } else
		closeConnection();
	}
    }
    return false;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
