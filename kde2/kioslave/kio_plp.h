/*-*-c++-*-
 * $Id$
 *
 * This file is part of plptools.
 *
 *  Copyright (C) 2001 Fritz Elfert <felfert@to.com>
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
#ifndef _KIO_PLP_H_
#define _KIO_PLP_H_

#include <kio/slavebase.h>
#include <kio/global.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qmap.h>
#include <rfsv.h>
#include <rpcs.h>
#include <ppsocket.h>

typedef QMap<PlpUID,QString> UidMap;

class PLPProtocol : public KIO::SlaveBase {
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
    virtual void special(const QByteArray &a);

    void calcprogress(long total);

private:
    bool checkConnection();

    char driveChar(const QString& path);

    void createVirtualDirEntry(KIO::UDSEntry & entry, bool rdonly, int type);
    void completeUDSEntry(KIO::UDSEntry& entry, PlpDirent &e, bool rom);
    bool checkForError(Enum<rfsv::errs> res, QString name1 = QString(0), QString name2 = QString(0));
    bool isRomDrive(const QString& path);
    bool isDrive(const QString& path);
    bool isRoot(const QString& path);
    void convertName(QString &path);
    bool emitTotalSize(QString &name);
    QString uid2mime(PlpDirent &e);
    int checkSpecial(const QString& path);
    void setOwner(KIO::UDSEntry & entry);

    rfsv *plpRfsv;
    rpcs *plpRpcs;
    ppsocket *plpRfsvSocket;
    ppsocket *plpRpcsSocket;
    QStringList drives;
    QMap<QString,char> drivechars;
    UidMap puids; 
    QString currentHost;
    int    currentPort;
    time_t t_last;
    time_t t_start;
    Enum<rpcs::machs> machType;
    rpcs::machineInfo machInfo;
};

#endif

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
