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
#include <qimage.h>
#include <qcursor.h>

#include <kaction.h>
#include <kapp.h>
#include <klocale.h>
#include <kwin.h>
#include <kiconloader.h>
#include <knotifyclient.h>
#include <kdebug.h>
#include <kmessagebox.h>

#include <psibitmap.h>

#include <iostream>

#define QUIT_ITEM    50
#define ABOUT_ITEM    51
#define CLIPFILE "C:/System/Data/Clpboard.cbd"

using namespace std;

TopLevel::TopLevel()
  : KMainWindow(0)
{
    KNotifyClient::startDaemon();

    clip = kapp->clipboard();
#if QT_VERSION > 300
    clip->setSelectionMode(true);
#endif
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
    sockNum = DPORT;

    struct servent *se = getservbyname("psion", "tcp");
    endservent();
    if (se != 0L)
        sockNum = ntohs(se->s_port);

    menu->insertTitle(kapp->miniIcon(), i18n("Klipsi - Psion Clipboard"));
    menu->insertSeparator();
    menu->insertItem(SmallIcon("help"), i18n("&About Klipsi"), ABOUT_ITEM);
    menu->insertItem(SmallIcon("exit"), i18n("&Quit"), QUIT_ITEM);

    about = new KAboutApplication(0L, 0L, false);
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
    if (timer)
	timer->start(interval, true);
}

TopLevel::~TopLevel()
{
    closeConnection();
    if (timer)
	delete timer;
    delete menu;
}

bool TopLevel::
isNotSupported() {
    return (timer == NULL);
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
	if (timer)
	    timer->start(5000, true);
	else
	    kapp->quit();
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

    if (!checkConnection())
	return;

    QImage clipImage = 0L;
    QString clipText = clip->text();

    if (clipText.isEmpty()) {
	clipImage = clip->image();
	if (clipImage.isNull())
	    return;
	inSend = true;
	mustListen = true;
	putClipImage(clipImage);
    } else {
	if (clipText == lastClipData)
	    return;
	lastClipData = clipText;
	inSend = true;
	mustListen = true;
	char *p = strdup(clipText.latin1());
	ascii2PsiText(p, clipText.length());
	putClipText(p);
	free(p);
    }

    res = rc->notify();
    inSend = false;

    if (res != rfsv::E_PSI_GEN_NONE)
	closeConnection();
}

void TopLevel::
slotMenuSelected(int id)
{
    switch (id) {
	case ABOUT_ITEM:
	    about->show();
	    break;
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
putClipText(char *data) {
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
	b.addStringT(data);       // @22 Data (Psion Word seems to need a
                                  //     terminating 0.

	p = (const unsigned char *)b.getString(0);
	rf->fwrite(fh, p, b.getLen(), l);
	rf->fclose(fh);
	rf->fsetattr(CLIPFILE, 0x20, 0x07);
    } else
	closeConnection();
}

static QImage *putImage;

static int
getGrayPixel(int x, int y)
{
    return qGray(putImage->pixel(x, y));
}

void TopLevel::
putClipImage(QImage &img) {
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
	b.addDWord(0x1000003d);   // @15 Section Type (Image)
	b.addDWord(0x0000001d);   // @19 Section Offset

	// Data
	bufferStore ib;
	putImage = &img;
	encodeBitmap(img.width(), img.height(), getGrayPixel, false, ib);
	b.addBuff(ib);

	p = (const unsigned char *)b.getString(0);
	rf->fwrite(fh, p, b.getLen(), l);
	rf->fclose(fh);
	rf->fsetattr(CLIPFILE, 0x20, 0x07);
    } else
	closeConnection();
}

QImage *TopLevel::
decode_image(const unsigned char *p)
{
    bufferStore out;
    bufferStore hout;
    QImage *img = 0L;
    int xPixels;
    int yPixels;

    if (!decodeBitmap(p, xPixels, yPixels, out))
	return img;

    QString hdr = QString("P5\n%1 %2\n255\n").arg(xPixels).arg(yPixels);
    hout.addString(hdr.latin1());
    hout.addBuff(out);

    img = new QImage(xPixels, yPixels, 8);
    if (!img->loadFromData((const uchar *)hout.getString(0), hout.getLen())) {
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
			    clipImg = decode_image((const unsigned char *)p);
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
	if (rf->getProtocolVersion() == 3) {
	    closeConnection();
	    delete timer;
	    timer = NULL;
	    KMessageBox::error(NULL, i18n(
	        "<QT>Your Psion is reported to be a <B>Series 3</B> "
		"machine. This type of machine does <B>not support</B> the "
		"remote clipboard protocol; Sorry.<BR/>"
		"<B>Klipsi</B> will now terminate.</QT>"),
			       i18n("Protocol not supported"));
	    return false;
	}
	if (!rc) {
	    rc = new rclip(rclipSocket);
	    Enum<rfsv::errs> ret;

	    if ((ret = rc->initClipbd()) == rfsv::E_PSI_GEN_NONE) {
		KNotifyClient::event("connected");
		constate = CONNECTED;
		repaint();
		return true;
	    } else {
		closeConnection();
		if (ret == rfsv::E_PSI_GEN_NSUP) {
		    KMessageBox::error(NULL, i18n(
			"<QT>Your Psion does not support the remote clipboard "
			"protocol.<BR/>The reason for that is usually a missing "
			"server library on your Psion.<BR/>Make shure, that "
			"<B>C:\\System\\Libs\\clipsvr.rsy</B> exists.<BR/>"
			"This file is part of PsiWin and usually gets copied "
			"to your Psion when you enable CopyAnywhere in PsiWin. "
			"You also get it from a PsiWin installation directory "
			"and copy it to your Psion manually.<BR/>"
			"<B>Klipsi</B> will now terminate.</QT>"),
				       i18n("Protocol not supported"));
		    delete timer;
		    timer = NULL;
		}
	    }
	}
    }
    return false;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
