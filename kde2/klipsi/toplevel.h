/*-*-c++-*-
 * $Id$
 *
 * This file is part of plptools.
 *
 *  Copyright (C) 1999  Philip Proudman <philip.proudman@btinternet.com>
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
#ifndef _TOPLEVEL_H_
#define _TOPLEVEL_H_

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <kapp.h>
#include <kmainwindow.h>
#include <kpopupmenu.h>
#include <qpixmap.h>
#include <qtimer.h>

#include <rfsv.h>
#include <rfsvfactory.h>
#include <rclip.h>
#include <ppsocket.h>

class QClipboard;

class TopLevel : public KMainWindow
{
    Q_OBJECT

public:
    TopLevel();
    ~TopLevel();
    bool isNotSupported();

protected:
    void paintEvent(QPaintEvent *);
    void mousePressEvent(QMouseEvent *);

protected slots:
    void showPopupMenu( QPopupMenu * );

private slots:
    void slotClipboardChanged();
    void slotTimer();
    void slotMenuSelected(int);

private:
    enum {
	ENABLED = 0,
	DISABLED = 1,
    } states;

    enum {
	DISCONNECTED = 0,
	CONNECTED = 1,
    } constates;

    void psiText2ascii(char *, int);
    void ascii2PsiText(char *, int);
    void putClipText(char *);
    void putClipImage(QImage &);
    void getClipData();
    void closeConnection();
    bool checkConnection();
    QImage *decode_image(const unsigned char *);

    QClipboard  *clip;
    KPopupMenu  *menu;
    QTimer      *timer;

    ppsocket    *rfsvSocket;
    ppsocket    *rclipSocket;
    rfsv        *rf;
    rclip       *rc;
    rfsvfactory *rff;

    QString     lastClipData;
    QPixmap     *icon;
    QPixmap     icons[2][2];
    bool        inSend;
    bool        inSetting;
    bool        mustListen;
    int         state;
    int         constate;
    int         sockNum;
};

#endif

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
