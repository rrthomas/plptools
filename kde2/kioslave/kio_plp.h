/*  This file is part of the KDE project
    Copyright (C) 2000 Alexander Neundorf <neundorf@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#ifndef KIO_PLP_H
#define KIO_PLP_H

#include <kio/slavebase.h>
#include <kio/global.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qmap.h>
#include <rfsv.h>
#include <ppsocket.h>

typedef QMap<PlpUID,QString> UidMap;

class PLPProtocol : public KIO::SlaveBase
{
public:
	PLPProtocol (const QCString &pool, const QCString &app);
	virtual ~PLPProtocol();
	
	virtual void openConnection();
	virtual void closeConnection();

	virtual void setHost(const QString& host, int port, const QString&, const QString&);

	virtual void put(const KURL& url, int _mode,bool _overwrite, bool _resume);
	virtual void get(const KURL& url);
	virtual void listDir(const KURL& url);
	virtual void stat(const KURL & url);
	virtual void mimetype(const KURL & url);
	virtual void mkdir(const KURL& url, int permissions);
	virtual void del(const KURL& url, bool isfile);
	virtual void chmod(const KURL& url, int permissions);
	virtual void rename(const KURL &src, const KURL &dest, bool overwrite);
	virtual void copy(const KURL& src, const KURL &dest, int mode, bool overwrite );

	void calcprogress(long total);
private:
	bool checkConnection();

	char driveChar(const QString& path);

	void createVirtualDirEntry(KIO::UDSEntry & entry, bool rdonly);
	void completeUDSEntry(KIO::UDSEntry& entry, PlpDirent &e, bool rom);
	bool checkForError(Enum<rfsv::errs> res);
	bool isRomDrive(const QString& path);
	bool isDrive(const QString& path);
	bool isRoot(const QString& path);
	void convertName(QString &path);
	bool emitTotalSize(QString &name);
	QString uid2mime(PlpDirent &e);

	rfsv *plpRfsv;
	ppsocket *plpRfsvSocket;
	QStringList drives;
	QMap<QString,char> drivechars;
	UidMap puids; 
	QString currentHost;
	int    currentPort;
	time_t t_last;
	time_t t_start;
};

#endif
