/*-*-c++-*-
 * $Id$
 *
 * This file is part of plptools.
 *
 *  Copyright (C) 1999  Philip Proudman <philip.proudman@btinternet.com>
 *  Copyright (C) 2000, 2001 Fritz Elfert <felfert@to.com>
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kio_plp.h"

#include <iomanip>
#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>

#include <qfile.h>

#include <kinstance.h>
#include <kdebug.h>
#include <klocale.h>
#include <kconfig.h>

#include <rfsvfactory.h>
#include <rpcsfactory.h>
#include <bufferarray.h>

#include <string>

using namespace KIO;

static int PLP_DEBUGAREA = 7999;
// until we get an offical assignment
#define kdDebug(PLP_DEBUGAREA) cout

extern "C" {
	int kdemain(int argc, char **argv);
}

#define PLP_FTYPE_DRIVE   1
#define PLP_FTYPE_OWNER   2
#define PLP_FTYPE_MACHINE 3
#define PLP_FTYPE_SETUP   4
#define PLP_FTYPE_BACKUP  5
#define PLP_FTYPE_RESTORE 6
#define PLP_FTYPE_ROOT    7

int
kdemain( int argc, char **argv ) {
    KInstance instance( "kio_nfs" );

    if (argc != 4) {
	fprintf(stderr, "Usage: kio_plp protocol domain-socket1 domain-socket2\n");
	exit(-1);
    }
    kdDebug(PLP_DEBUGAREA) << "PLP: kdemain: starting" << endl;

    PLPProtocol slave(argv[2], argv[3]);
    slave.dispatchLoop();
    return 0;
}

static void
stripTrailingSlash(QString& path) {
    if (path=="/")
	path="";
    else
	if (path[path.length()-1]=='/')
	    path.truncate(path.length()-1);
}

static QString
baseName(const QString& path) {
    return path.mid(path.findRev("/") + 1);
}

static QString
removeFirstPart(const QString& path, QString &removed) {
    QString result("");
    if (path.isEmpty()) {
	removed = "";
	return result;
    }
    result = path.mid(1);
    int slashPos = result.find("/");
    if (slashPos == -1) {
	removed = result;
	result = "";
    } else {
	removed = result.left(slashPos);
	result = result.mid(slashPos);
    }
    return result;
}

PLPProtocol::PLPProtocol (const QCString &pool, const QCString &app)
    :SlaveBase("psion", pool, app), plpRfsv(0), plpRfsvSocket(0) {

    kdDebug(PLP_DEBUGAREA) << "PLPProtocol::PLPProtocol(" << pool << ","
			   << app << ")" << endl;

    currentHost = "";
    struct servent *se = getservbyname("psion", "tcp");
    endservent();
    if (se != 0L)
	currentPort = ntohs(se->s_port);
    else
	currentPort = DPORT;

    typedef QMap<QString,QString> UIDMap;
    KConfig *cfg = new KConfig("kioslaverc");

    UIDMap uids = cfg->entryMap("Psion/UIDmapping");
    if (uids.isEmpty()) {
	cfg->setGroup("Psion/UIDmapping");
	// Builtin application types.
	cfg->writeEntry("uid-10000037-1000006D-1000007F",
			"application/x-psion-word");
	cfg->writeEntry("uid-10000037-1000006D-10000088",
			"application/x-psion-sheet");
	cfg->writeEntry("uid-10000037-1000006D-1000006d",
			"application/x-psion-record");
	cfg->writeEntry("uid-10000037-1000006D-1000007d",
			"application/x-psion-sketch");
	cfg->writeEntry("uid-10000037-1000006D-10000085",
			"application/x-psion-opl");
	cfg->writeEntry("uid-10000050-1000006D-10000084",
			"application/x-psion-agenda");
	cfg->writeEntry("uid-10000050-1000006D-10000086",
			"application/x-psion-data");
	cfg->sync();
	uids = cfg->entryMap("Psion/UIDmapping");
    }
    for (UIDMap::Iterator uit = uids.begin(); uit != uids.end(); ++uit) {
	long u1, u2, u3;

	sscanf(uit.key().data(), "uid-%08X-%08X-%08X", &u1, &u2, &u3);
	puids.insert(PlpUID(u1, u2, u3), uit.data());
    }
}

PLPProtocol::~PLPProtocol() {
    closeConnection();
}

void PLPProtocol::
closeConnection() {
    if (plpRfsv)
	delete(plpRfsv);
    if (plpRfsvSocket)
	delete(plpRfsvSocket);
    plpRfsv = 0;
    plpRfsvSocket = 0;
}

bool PLPProtocol::
isRoot(const QString& path) {
    return (path.isEmpty() || (path=="/"));
}

bool PLPProtocol::
isDrive(const QString& path) {
    QString tmp = path;
    stripTrailingSlash(tmp);
    for (QStringList::Iterator it = drives.begin(); it != drives.end(); it++) {
	QString cmp = "/" + *it;
	if (cmp == tmp)
	    return true;
    }
    return false;
}

bool PLPProtocol::
isRomDrive(const QString& path) {
    return (driveChar(path) == 'Z');
}

char PLPProtocol::
driveChar(const QString& path) {
    QString vname;
    QString dummy = removeFirstPart(path, vname);
    if (drivechars.find(vname) != drivechars.end())
	return drivechars[vname];
    return '\0';
}

int PLPProtocol::
checkSpecial(const QString& path) {
    QString tmp = path.mid(1);
    if (tmp == i18n("Owner"))
	return PLP_FTYPE_OWNER;
    if (tmp == i18n("Machine"))
	return PLP_FTYPE_MACHINE;
    if (tmp == i18n("Settings"))
	return PLP_FTYPE_SETUP;
    if (tmp == i18n("Backup"))
	return PLP_FTYPE_BACKUP;
    if (tmp == i18n("Restore"))
	return PLP_FTYPE_RESTORE;
    return 0;
}

void PLPProtocol::
convertName(QString &path) {
    kdDebug(PLP_DEBUGAREA) << "convert: in='" << path << "' out='";
    QString dummy;
    QString drive;

    drive.sprintf("%c:", driveChar(path));
    path = drive + removeFirstPart(path, dummy);
    path.replace(QRegExp("/"), "\\");
    kdDebug(PLP_DEBUGAREA) << path << "'" << endl;
}

void PLPProtocol::
openConnection() {
    kdDebug(PLP_DEBUGAREA) << "PLP::openConnection" << endl;
    closeConnection();

    plpRfsvSocket = new ppsocket();
    QString estr = i18n("Could not connect to ncpd at %1:%2").arg(currentHost).arg(currentPort);
    if (!plpRfsvSocket->connect((char *)(currentHost.data()), currentPort)) {
	error(ERR_COULD_NOT_CONNECT, estr);
	return;
    }
    rfsvfactory factory(plpRfsvSocket);
    plpRfsv = factory.create(false);
    if (plpRfsv == 0L) {
	error(ERR_COULD_NOT_CONNECT, estr);
	return;
    }

    plpRpcsSocket = new ppsocket();
    if (!plpRpcsSocket->connect((char *)(currentHost.data()), currentPort)) {
	error(ERR_COULD_NOT_CONNECT, estr);
	return;
    }
    rpcsfactory factory2(plpRpcsSocket);
    plpRpcs = factory2.create(false);
    if (plpRpcs == 0L) {
	error(ERR_COULD_NOT_CONNECT, estr);
	return;
    }

    /* If we have a S5, get the Psion's Owner- and Mach- info.
     * This implicitely sets the Timezone info of the Psion also.
     */
    bufferArray b;
    Enum <rfsv::errs> res;
    if ((res = plpRpcs->getOwnerInfo(b)) == rfsv::E_PSI_GEN_NONE) {
	plpRpcs->getMachineType(machType);
	if (machType == rpcs::PSI_MACH_S5)
	    plpRpcs->getMachineInfo(machInfo);
    }

    u_int32_t devbits;

    if ((res = plpRfsv->devlist(devbits)) == rfsv::E_PSI_GEN_NONE) {
	for (int i = 0; i < 26; i++) {
	    if ((devbits & 1) != 0) {
		PlpDrive drive;
		if (plpRfsv->devinfo(i + 'A', drive) == rfsv::E_PSI_GEN_NONE) {
		    string vname = drive.getName();
		    QString name;

		    if (!vname.empty())
			name = QString(vname.c_str());
		    else
			name.sprintf("%c", 'A' + i);
		    drives.append(name);
		    drivechars.insert(name, 'A' + i);
		}
	    }
	    devbits >>= 1;
	}
    } else {
	error(ERR_COULD_NOT_CONNECT, i18n("Could not get drive list"));
	return;
    }
    connected();
    kdDebug(PLP_DEBUGAREA) << "openConnection succeeded" << endl;
}

bool PLPProtocol::
checkConnection() {
    kdDebug(PLP_DEBUGAREA) << "PLP::checkConnection" << endl;
    if (plpRfsv == 0)
	openConnection();
    return (plpRfsv == 0);
}

QString PLPProtocol::
uid2mime(PlpDirent &e) {
    QString tmp;
    PlpUID u = e.getUID();
    UidMap::Iterator it = puids.find(u);

    if (it != puids.end())
	tmp = it.data();
    else
	tmp.sprintf("application/x-psion-uid-%08X-%08X-%08X", u[0], u[1], u[2]);
    return tmp;
}

void PLPProtocol::
listDir(const KURL& _url) {
    KURL url(_url);
    QString path(QFile::encodeName(url.path(-1)));

    if (path.isEmpty()) {
	url.setPath("/");
	redirection(url);
	finished();
	return;
    }

    if (checkConnection())
	return;

    if (isRoot(path)) {
	kdDebug(PLP_DEBUGAREA) << "listing root " << drives.count() << endl;
	totalSize(drives.count());
	//in this case we don't need to do a real listdir
	UDSEntry entry;
	UDSAtom atom;
	for (QStringList::Iterator it = drives.begin(); it != drives.end(); it++) {
	    entry.clear();
	    atom.m_uds = KIO::UDS_NAME;
	    atom.m_str = (*it);
	    kdDebug(PLP_DEBUGAREA) << "listing " << (*it) << endl;
	    entry.append(atom);
	    createVirtualDirEntry(entry, drivechars[*it] == 'Z', PLP_FTYPE_DRIVE);
	    listEntry(entry, false);
	}
	listEntry(entry, true);
	finished();
	return;
    }

    kdDebug(PLP_DEBUGAREA) << "getting subdir -" << path << "-" << endl;
    bool rom = isRomDrive(path);
    convertName(path);
    path += "\\";

    PlpDir files;
    Enum<rfsv::errs> res = plpRfsv->dir(path.latin1(), files);
    if (checkForError(res, url.path()))
	return;
    totalSize(files.size());
    UDSEntry entry;
    for (int i = 0; i < files.size(); i++) {
	UDSAtom atom;

	PlpDirent e = files[i];
	long attr = e.getAttr();

	entry.clear();
	atom.m_uds = KIO::UDS_NAME;
	atom.m_str = e.getName();
	entry.append(atom);

	if ((attr & rfsv::PSI_A_DIR) == 0) {
	    atom.m_uds = KIO::UDS_MIME_TYPE;
	    atom.m_str = uid2mime(e);
	    entry.append(atom);
	}

	completeUDSEntry(entry, e, rom);
	listEntry(entry, false);
    }
    listEntry(entry, true); // ready
    finished();
}

void PLPProtocol::
setOwner(UDSEntry & entry) {
    UDSAtom atom;
    struct passwd *pw = getpwuid(getuid());
    struct group *gr = getgrgid(getgid());

    atom.m_uds = KIO::UDS_USER;
    atom.m_str = pw ? pw->pw_name : "root";
    entry.append(atom);

    atom.m_uds = KIO::UDS_GROUP;
    atom.m_str = gr ? gr->gr_name : "root";
    entry.append(atom);

    endgrent();
    endpwent();
}

void PLPProtocol::
createVirtualDirEntry(UDSEntry & entry, bool rdonly, int type) {
    UDSAtom atom;

    atom.m_uds = KIO::UDS_ACCESS;
    atom.m_long = S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
    if (!rdonly)
	atom.m_long |= (S_IWUSR | S_IWGRP | S_IWOTH);
    entry.append(atom);

    atom.m_uds = KIO::UDS_SIZE;
    atom.m_long = 0;
    entry.append(atom);

    setOwner(entry);

    switch (type) {
	case PLP_FTYPE_ROOT:
	    atom.m_uds = KIO::UDS_FILE_TYPE;
	    atom.m_long = S_IFDIR;
	    entry.append(atom);
	    atom.m_uds = KIO::UDS_GUESSED_MIME_TYPE;
	    atom.m_uds = KIO::UDS_MIME_TYPE;
	    atom.m_str = QString("inode/x-psion-drive");
	    entry.append(atom);
	    break;
	case PLP_FTYPE_DRIVE:
	    atom.m_uds = KIO::UDS_FILE_TYPE;
	    atom.m_long = S_IFDIR;
	    entry.append(atom);
	    atom.m_uds = KIO::UDS_GUESSED_MIME_TYPE;
	    atom.m_str = QString("inode/x-psion-drive");
	    atom.m_uds = KIO::UDS_MIME_TYPE;
	    atom.m_str = QString("inode/x-psion-drive");
	    entry.append(atom);
	    break;
	case PLP_FTYPE_OWNER:
	    atom.m_uds = KIO::UDS_FILE_TYPE;
	    atom.m_long = S_IFREG;
	    entry.append(atom);
	    atom.m_uds = KIO::UDS_MIME_TYPE;
	    atom.m_str = QString("application/x-psion-owner");
	    entry.append(atom);
	    break;
	case PLP_FTYPE_MACHINE:
	    atom.m_uds = KIO::UDS_FILE_TYPE;
	    atom.m_long = S_IFREG;
	    entry.append(atom);
	    atom.m_uds = KIO::UDS_MIME_TYPE;
	    atom.m_str = QString("application/x-psion-machine");
	    entry.append(atom);
	    break;
	case PLP_FTYPE_SETUP:
	    atom.m_uds = KIO::UDS_FILE_TYPE;
	    atom.m_long = S_IFREG;
	    entry.append(atom);
	    atom.m_uds = KIO::UDS_MIME_TYPE;
	    atom.m_str = QString("application/x-psion-setup");
	    entry.append(atom);
	    break;
	case PLP_FTYPE_BACKUP:
	    atom.m_uds = KIO::UDS_FILE_TYPE;
	    atom.m_long = S_IFREG;
	    entry.append(atom);
	    atom.m_uds = KIO::UDS_MIME_TYPE;
	    atom.m_str = QString("application/x-psion-backup");
	    entry.append(atom);
	    break;
	case PLP_FTYPE_RESTORE:
	    atom.m_uds = KIO::UDS_FILE_TYPE;
	    atom.m_long = S_IFREG;
	    entry.append(atom);
	    atom.m_uds = KIO::UDS_MIME_TYPE;
	    atom.m_str = QString("application/x-psion-restore");
	    entry.append(atom);
	    break;
    }
}

bool PLPProtocol::
emitTotalSize(QString &name) {
    PlpDirent e;

    Enum<rfsv::errs> res = plpRfsv->fgeteattr(name.latin1(), e);
    if (checkForError(res, name))
	return true;
    totalSize(e.getSize());
    return false;
}

void PLPProtocol::
stat(const KURL & url) {
    QString path(QFile::encodeName(url.path(-1)));
    UDSEntry entry;
    UDSAtom atom;

    if (checkConnection())
	return;

    kdDebug(PLP_DEBUGAREA) << "stat(" << path << ")" << endl;
    stripTrailingSlash(path);

    if (isRoot(path) || isDrive(path)) {
	atom.m_uds = KIO::UDS_NAME;
	atom.m_str = path;
	entry.append(atom);
	if (isRoot(path))
	    createVirtualDirEntry(entry, true, PLP_FTYPE_ROOT);
	else
	    createVirtualDirEntry(entry, isRomDrive(path), PLP_FTYPE_DRIVE);
	statEntry(entry);
	finished();
	kdDebug(PLP_DEBUGAREA) << "succeeded" << endl;
	return;
    }
    int ftype = checkSpecial(path);
    if (ftype != 0) {
	atom.m_uds = KIO::UDS_NAME;
	atom.m_str = path;
	entry.append(atom);
	createVirtualDirEntry(entry, false, ftype);
	statEntry(entry);
	finished();
	kdDebug(PLP_DEBUGAREA) << "succeeded" << endl;
	return;
    }

    bool rom = isRomDrive(path);
    QString fileName = baseName(path);
    convertName(path);

    if (path.isEmpty()) {
	error(ERR_DOES_NOT_EXIST, url.path());
	return;
    }

    PlpDirent e;

    Enum<rfsv::errs> res = plpRfsv->fgeteattr(path.latin1(), e);
    if (checkForError(res, url.path()))
	return;

    atom.m_uds = KIO::UDS_NAME;
    atom.m_str = fileName;
    entry.append(atom);
    completeUDSEntry(entry, e, rom);
    statEntry(entry);

    finished();
}

void PLPProtocol::
mimetype(const KURL & url) {
    QString path(QFile::encodeName(url.path(-1)));
    UDSEntry entry;
    UDSAtom atom;

    kdDebug(PLP_DEBUGAREA) << "mimetype(" << path << ")" << endl;
    stripTrailingSlash(path);

    if (isRoot(path) || isDrive(path)) {
	mimeType("inode/directory");
	finished();
	return;
    }
    convertName(path);

    if (path.isEmpty()) {
	error(ERR_DOES_NOT_EXIST, url.path());
	return;
    }

    PlpDirent e;
    Enum<rfsv::errs> res = plpRfsv->fgeteattr(path.latin1(), e);
    if (checkForError(res, url.path()))
	return;
    mimeType(uid2mime(e));
    finished();
}

void PLPProtocol::
completeUDSEntry(UDSEntry& entry, PlpDirent &e, bool rom) {
    UDSAtom atom;
    long attr = e.getAttr();

    if (rom)
	attr |= rfsv::PSI_A_RDONLY;

    atom.m_uds = KIO::UDS_SIZE;
    atom.m_long = e.getSize();
    entry.append(atom);

    if (attr & rfsv::PSI_A_DIR)
	atom.m_uds = KIO::UDS_CREATION_TIME;
    else
	atom.m_uds = KIO::UDS_MODIFICATION_TIME;
    atom.m_long = e.getPsiTime().getTime();
    entry.append(atom);

    atom.m_uds = KIO::UDS_ACCESS;
    atom.m_long = S_IRUSR | S_IRGRP | S_IROTH;
    if (attr & rfsv::PSI_A_DIR)
	atom.m_long |= S_IXUSR | S_IXGRP | S_IXOTH;
    if (!(attr & rfsv::PSI_A_RDONLY))
	atom.m_long |= S_IWUSR | S_IWGRP | S_IWOTH;
    entry.append(atom);

    atom.m_uds = KIO::UDS_FILE_TYPE;
    atom.m_long = (attr & rfsv::PSI_A_DIR) ? S_IFDIR : S_IFREG;
    entry.append(atom);

    setOwner(entry);

#if 1
    KIO::UDSEntry::ConstIterator it = entry.begin();
    for( ; it != entry.end(); it++ ) {
	switch ((*it).m_uds) {
	    case KIO::UDS_FILE_TYPE:
		kdDebug(PLP_DEBUGAREA) << "File Type : " <<
		    (mode_t)((*it).m_long) << endl;
		break;
	    case KIO::UDS_SIZE:
		kdDebug(PLP_DEBUGAREA) << "File Size : " <<
		    (long)((*it).m_long) << endl;
		break;
	    case KIO::UDS_ACCESS:
		kdDebug(PLP_DEBUGAREA) << "Access permissions : " <<
		    (mode_t)((*it).m_long) << endl;
		break;
	    case KIO::UDS_USER:
		kdDebug(PLP_DEBUGAREA) << "User : " <<
		    ((*it).m_str.ascii() ) << endl;
		break;
	    case KIO::UDS_GROUP:
		kdDebug(PLP_DEBUGAREA) << "Group : " <<
		    ((*it).m_str.ascii() ) << endl;
		break;
	    case KIO::UDS_NAME:
		kdDebug(PLP_DEBUGAREA) << "Name : " <<
		    ((*it).m_str.ascii() ) << endl;
		//m_strText = decodeFileName( (*it).m_str );
		break;
	    case KIO::UDS_URL:
		kdDebug(PLP_DEBUGAREA) << "URL : " <<
		    ((*it).m_str.ascii() ) << endl;
		break;
	    case KIO::UDS_MIME_TYPE:
		kdDebug(PLP_DEBUGAREA) << "MimeType : " <<
		    ((*it).m_str.ascii() ) << endl;
		break;
	    case KIO::UDS_LINK_DEST:
		kdDebug(PLP_DEBUGAREA) << "LinkDest : " <<
		    ((*it).m_str.ascii() ) << endl;
		break;
	}
    }
#endif
}

void PLPProtocol::
setHost(const QString& host, int port, const QString&, const QString&) {
    kdDebug(PLP_DEBUGAREA) << "setHost(" << host << "," << port << ")" << endl;
    QString tmphost = host;
    if (host.isEmpty())
	tmphost = "localhost";
    if (port == 0) {
	struct servent *se = getservbyname("psion", "tcp");
	endservent();
	if (se != 0L)
	    port = ntohs(se->s_port);
	else
	    port = DPORT;
    }
    if ((tmphost == currentHost) && (port == currentPort))
	return;
    currentHost = tmphost;
    currentPort = port;
    closeConnection();
}

void PLPProtocol::
mkdir(const KURL& url, int) {
    kdDebug(PLP_DEBUGAREA) << "mkdir" << endl;
    QString name(QFile::encodeName(url.path(-1)));

    if (checkConnection())
	return;
    kdDebug(PLP_DEBUGAREA) << "mkdir(" << name << ")" << endl;
    if (isRomDrive(name)) {
	error(ERR_ACCESS_DENIED, i18n("%1: read only filesystem").arg(url.path()));
	return;
    }
    if (isRoot(name) || isDrive(name)) {
	error(ERR_ACCESS_DENIED, i18n("%1: virtual directory").arg(url.path()));
	return;
    }
    convertName(name);
    Enum<rfsv::errs> res = plpRfsv->mkdir(name.latin1());
    if (checkForError(res, url.path()))
	return;
    finished();
}

bool PLPProtocol::
checkForError(Enum<rfsv::errs> res, QString n1, QString n2) {
    if (res != rfsv::E_PSI_GEN_NONE) {
	kdDebug(PLP_DEBUGAREA) << "plp error: " << res << endl;
	QString reason(res);
	QString text;
	if (!!n1 && !!n2)
	    text = i18n("%1 or %2: %3").arg(n1).arg(n2).arg(reason);
	else {
	    if (!!n1 || !!n2)
		text = QString("%1: %2").arg(n1 ? n1 : n2).arg(reason);
	    else
		text = reason;
	}
	switch (res) {
	    case rfsv::E_PSI_FILE_ACCESS:
		error(ERR_ACCESS_DENIED, text);
		break;
	    case rfsv::E_PSI_FILE_EXIST:
		error(ERR_FILE_ALREADY_EXIST, text);
		break;
	    case rfsv::E_PSI_FILE_NXIST:
		error(ERR_DOES_NOT_EXIST, text);
		break;
	    case rfsv::E_PSI_FILE_DIR:
		error(ERR_IS_DIRECTORY, text);
		break;
	    default:
		error(ERR_UNKNOWN, text);
		break;
	}
	return true;
    }
    return false;
}

void PLPProtocol::
del( const KURL& url, bool isfile) {
    kdDebug(PLP_DEBUGAREA) << "del" << endl;
    QString name(QFile::encodeName(url.path(-1)));

    if (checkConnection())
	return;
    kdDebug(PLP_DEBUGAREA) << "del(" << name << ")" << endl;
    if (isRomDrive(name)) {
	error(ERR_ACCESS_DENIED, i18n("%1: read only filesystem").arg(url.path()));
	return;
    }
    if (isRoot(name) || isDrive(name)) {
	error(ERR_ACCESS_DENIED, i18n("%1: virtual directory").arg(url.path()));
	return;
    }
    convertName(name);

    Enum<rfsv::errs> res =
	(isfile) ? plpRfsv->remove(name.latin1()) : plpRfsv->rmdir(name.latin1());
    if (checkForError(res, url.path()))
	return;
    finished();
}

void PLPProtocol::
chmod( const KURL& url, int permissions ) {
    kdDebug(PLP_DEBUGAREA) << "del" << endl;
    QString name(QFile::encodeName(url.path(-1)));

    if (checkConnection())
	return;
    kdDebug(PLP_DEBUGAREA) << "chmod(" << name << ")" << endl;
    if (isRomDrive(name)) {
	error(ERR_ACCESS_DENIED, i18n("%1: read only filesystem").arg(url.path()));
	return;
    }
    if (isRoot(name) || isDrive(name)) {
	error(ERR_ACCESS_DENIED, i18n("%1: virtual directory").arg(url.path()));
	return;
    }
    convertName(name);
    long attr[2];
    attr[0] = attr[1] = 0;
    Enum <rfsv::errs> res = plpRfsv->fsetattr(name.latin1(), attr[0], attr[1]);
    if (checkForError(res, url.path()))
	return;
    finished();
}

void PLPProtocol::
get( const KURL& url ) {
    kdDebug(PLP_DEBUGAREA) << "get" << endl;
    QString name(QFile::encodeName(url.path(-1)));

    if (checkConnection())
	return;
    kdDebug(PLP_DEBUGAREA) << "get(" << name << ")" << endl;
    if (isRoot(name) || isDrive(name)) {
	error(ERR_ACCESS_DENIED, i18n("%1: virtual directory").arg(url.path()));
	return;
    }
    convertName(name);

    Enum<rfsv::errs> res;
    u_int32_t handle;
    u_int32_t len;
    u_int32_t size;
    u_int32_t total = 0;

    if (emitTotalSize(name))
	return;
    res = plpRfsv->fopen(plpRfsv->opMode(rfsv::PSI_O_RDONLY), name.latin1(), handle);
    if (checkForError(res, url.path()))
	return;

    QByteArray a(RFSV_SENDLEN);
    do {
	if ((res = plpRfsv->fread(handle, (unsigned char *)(a.data()),
				  RFSV_SENDLEN, len)) == rfsv::E_PSI_GEN_NONE) {
	    if (len < RFSV_SENDLEN)
		a.resize(len);
	    data(a);
	    total += len;
	    calcprogress(total);
	}
    } while ((len > 0) && (res == rfsv::E_PSI_GEN_NONE));
    plpRfsv->fclose(handle);
    if (checkForError(res, url.path()))
	return;
    data(QByteArray());

    finished();
}

//TODO the partial putting thing is not yet implemented
void PLPProtocol::
put( const KURL& url, int _mode, bool _overwrite, bool /*_resume*/ ) {
    kdDebug(PLP_DEBUGAREA) << "get" << endl;
    QString name(QFile::encodeName(url.path(-1)));

    if (checkConnection())
	return;
    kdDebug(PLP_DEBUGAREA) << "put(" << name << ")" << endl;
    if (isRomDrive(name)) {
	error(ERR_ACCESS_DENIED, i18n("%1: read only filesystem").arg(url.path()));
	return;
    }
    if (isRoot(name) || isDrive(name)) {
	error(ERR_ACCESS_DENIED, i18n("%1: virtual directory").arg(url.path()));
	return;
    }
    convertName(name);

    Enum<rfsv::errs> res;
    u_int32_t handle;
    int result;

    res = plpRfsv->fcreatefile(plpRfsv->opMode(rfsv::PSI_O_RDWR), name.latin1(), handle);
    if ((res == rfsv::E_PSI_FILE_EXIST) && _overwrite)
	res = plpRfsv->freplacefile(plpRfsv->opMode(rfsv::PSI_O_RDWR), name.latin1(), handle);
    if (checkForError(res, url.path()))
	return;

    do {
	QByteArray a;
	dataReq();
	result = readData(a);
	const unsigned char *data = (const unsigned char *)(a.data());
	long len = a.size();

	if (result > 0)
	    do {
		u_int32_t written;
		u_int32_t count = (len > RFSV_SENDLEN) ? RFSV_SENDLEN : len;
		res = plpRfsv->fwrite(handle, data, count, written);
		if (checkForError(res, url.path())) {
		    plpRfsv->fclose(handle);
		    return;
		}
		len -= written;
		data += written;
	    } while (len > 0);
    } while (result > 0);
    plpRfsv->fclose(handle);
    finished();
}

void PLPProtocol::
rename(const KURL &src, const KURL &dest, bool _overwrite) {
    QString from(QFile::encodeName(src.path(-1)));
    QString to(QFile::encodeName(dest.path(-1)));

    if (checkConnection())
	return;
    kdDebug(PLP_DEBUGAREA) << "rename(" << from << "," << to << ")" << endl;
    if ((driveChar(from) != driveChar(to)) && (driveChar(to) != '\0')) {
	error(ERR_ACCESS_DENIED, i18n("%1 or %2: virtual directory").arg(src.path()).arg(dest.path()));
	kdDebug(PLP_DEBUGAREA) << "from FS != to FS" << endl;
	return;
    }
    if (isRomDrive(from)) {
	error(ERR_ACCESS_DENIED, i18n("%1: read only filesystem").arg(src.path()));
	kdDebug(PLP_DEBUGAREA) << "from ROFS" << endl;
	return;
    }
    if (isRoot(from)) {
	error(ERR_ACCESS_DENIED, i18n("%1: virtual directory").arg(src.path()));
	kdDebug(PLP_DEBUGAREA) << "from VFS" << endl;
	return;
    }
    bool volRename = isDrive(from);
    if (isRomDrive(to)) {
	error(ERR_ACCESS_DENIED, i18n("%1: read only filesystem").arg(dest.path()));
	kdDebug(PLP_DEBUGAREA) << "to ROFS" << endl;
	return;
    }
    if (isRoot(to)) {
	error(ERR_ACCESS_DENIED, i18n("%1: virtual directory").arg(dest.path()));
	kdDebug(PLP_DEBUGAREA) << "to VFS" << endl;
	return;
    }

    Enum <rfsv::errs> res;
    kdDebug(PLP_DEBUGAREA) << "ren: from=" << from << " to=" << to << endl;
    if (volRename) {
	to = to.mid(1);
	res = plpRfsv->setVolumeName(driveChar(from), to);
	if (res == rfsv::E_PSI_GEN_NONE) {
	    char drvc = driveChar(from);
	    drivechars.remove(from);
	    drivechars.insert(to, drvc);
	}
    } else {
	convertName(from);
	convertName(to);
	if (!_overwrite) {
	    u_int32_t attr;
	    if ((res = plpRfsv->fgetattr(to.latin1(), attr)) == rfsv::E_PSI_GEN_NONE) {

		error(ERR_FILE_ALREADY_EXIST, to);
		return;
	    }
	}
	res = plpRfsv->rename(from.latin1(), to.latin1());
    }
    if (checkForError(res, src.path(), dest.path()))
	return;
    finished();
}

extern "C" {
    static int
    progresswrapper(void *ptr, u_int32_t total) {

	((PLPProtocol *)ptr)->calcprogress(total);
	return 1;
    }
}

void PLPProtocol::
calcprogress(long total) {
    time_t t = time(0);
    if (t - t_last) {
	processedSize(total);
	speed(total / (t - t_start));
	t_last = t;
    }
}

void PLPProtocol::
copy( const KURL &src, const KURL &dest, int _mode, bool _overwrite ) {
    QString from( QFile::encodeName(src.path(-1)));
    QString to( QFile::encodeName(dest.path(-1)));

    if (checkConnection())
	return;
    kdDebug(PLP_DEBUGAREA) << "copy(" << from << "," << to << ")" << endl;
    if (isRoot(from) || isDrive(from)) {
	error(ERR_ACCESS_DENIED, i18n("%1 or %2: virtual directory").arg(src.path()).arg(dest.path()));
	return;
    }
    convertName(from);
    if (isRomDrive(to)) {
	error(ERR_ACCESS_DENIED, i18n("%1: read only filesystem").arg(dest.path()));
	return;
    }
    if (isRoot(to) || isDrive(to)) {
	error(ERR_ACCESS_DENIED, i18n("%1: virtual directory").arg(dest.path()));
	return;
    }
    convertName(to);
    Enum <rfsv::errs> res;
    if (!_overwrite) {
	u_int32_t attr;
	if ((res = plpRfsv->fgetattr(to.latin1(), attr)) == rfsv::E_PSI_GEN_NONE) {
	    error(ERR_FILE_ALREADY_EXIST, to);
	    return;
	}
    }
    if (emitTotalSize(from))
	return;
    t_last = t_start = time(0);
    res = plpRfsv->copyOnPsion(from.latin1(), to.latin1(), (void *)this, progresswrapper);
    if (checkForError(res, src.path(), dest.path()))
	return;
    finished();
}

void PLPProtocol::
special(const QByteArray &a) {
    kdDebug(PLP_DEBUGAREA) << "special()" << endl;

    QDataStream stream(a, IO_ReadOnly);
    int cmd;
    UDSEntry entry;
    UDSAtom atom;

    if (checkConnection())
	return;

    stream >> cmd;

    switch (cmd) {
	case 1: {
	    kdDebug(PLP_DEBUGAREA) << "get DriveDetails" << endl;
	    QString param;
	    PlpDrive drive;

	    Enum<rfsv::errs> res;
	    int drv;

	    stream >> param;
	    cout << "p='" << param << "'" << endl;
	    if (param.isEmpty()) {
		error(ERR_MALFORMED_URL, i18n("(empty)"));
		return;
	    }
	    if (!isDrive(param)) {
		error(ERR_PROTOCOL_IS_NOT_A_FILESYSTEM, param);
		return;
	    }
	    param = param.mid(1);
	    drv = drivechars[param];
	    res = plpRfsv->devinfo(drv, drive);
	    if (res != rfsv::E_PSI_GEN_NONE) {
		error(ERR_COULD_NOT_STAT, param);
		return;
	    }

	    string mtype;
	    drive.getMediaType(mtype);

	    // DriveLetter
	    atom.m_uds = KIO::UDS_USER;
	    atom.m_str = QString("%1").arg(drivechars[param]);
	    entry.append(atom);
	    // TypeName
	    atom.m_uds = KIO::UDS_NAME;
	    atom.m_str = QString(mtype.c_str());
	    entry.append(atom);
	    // Total capacity
	    atom.m_uds = KIO::UDS_SIZE;
	    atom.m_long = drive.getSize();
	    entry.append(atom);
	    // Free capacity
	    atom.m_uds = KIO::UDS_MODIFICATION_TIME;
	    atom.m_long = drive.getSpace();
	    entry.append(atom);
	    // UID
	    atom.m_uds = KIO::UDS_CREATION_TIME;
	    atom.m_long = drive.getUID();
	    entry.append(atom);

	    statEntry(entry);
	}
	    break;
	case 2: {
	    kdDebug(PLP_DEBUGAREA) << "get Ownerinfo" << endl;
	    bufferArray b;

	    Enum<rfsv::errs> res = plpRpcs->getOwnerInfo(b);
	    if (res != rfsv::E_PSI_GEN_NONE) {
		error(ERR_COULD_NOT_STAT, "Owner");
		return;
	    }
	    QString param = "";
	    while (!b.empty()) {
		param += b.pop().getString();
		param += "\n";
	    }
	    // Owner-String
	    atom.m_uds = KIO::UDS_NAME;
	    atom.m_str = param;
	    entry.append(atom);
	    statEntry(entry);
	}
	    break;
	case 3: {
	    kdDebug(PLP_DEBUGAREA) << "get Fileattribs" << endl;

	    QString name;
	    PlpDirent e;
	    bool isRoFS;

	    stream >> name;

	    if (name.isEmpty()) {
		error(ERR_MALFORMED_URL, i18n("(empty)"));
		return;
	    }
	    if (isRoot(name) || isDrive(name)) {
		error(ERR_ACCESS_DENIED, i18n("%1: virtual directory").arg(name));
		return;
	    }
	    isRoFS = isRomDrive(name);
	    convertName(name);
	    Enum<rfsv::errs> res = plpRfsv->fgeteattr(name.latin1(), e);
	    if (checkForError(res, name))
		return;
	    // Attributes
	    atom.m_uds = KIO::UDS_SIZE;
	    atom.m_long = e.getAttr();
	    entry.append(atom);
	    // bool ReadonlyFS (for disabling checkboxes in attr dialog)
	    atom.m_uds = KIO::UDS_CREATION_TIME;
	    u_int32_t flags = (machType == rpcs::PSI_MACH_S5) ? 1 : 0;
	    flags |= (isRoFS) ? 2 : 0;
	    atom.m_long = flags;
	    entry.append(atom);
	    // Psion Path
	    atom.m_uds = KIO::UDS_NAME;
	    atom.m_str = name;
	    entry.append(atom);

	    statEntry(entry);
	    kdDebug(PLP_DEBUGAREA) << "get Fileattribs done OK" << endl;
	}
	    break;
	case 4: {
	    kdDebug(PLP_DEBUGAREA) << "set Fileattribs" << endl;

	    QString name;
	    u_int32_t seta, unseta;

	    stream >> seta >> unseta >> name;

	    if (name.isEmpty()) {
		error(ERR_MALFORMED_URL, i18n("(empty)"));
		return;
	    }
	    if (isRoot(name) || isDrive(name)) {
		error(ERR_ACCESS_DENIED, i18n("%1: virtual directory").arg(name));
		return;
	    }
	    if (isRomDrive(name)) {
		error(ERR_ACCESS_DENIED, i18n("%1: read only filesystem").arg(name));
		return;
	    }
	    convertName(name);
	    Enum<rfsv::errs> res = plpRfsv->fsetattr(name.latin1(), seta,
						     unseta);
	    if (checkForError(res, name))
		return;
	}
	    break;
	case 5: {
	    kdDebug(PLP_DEBUGAREA) << "get machineInfo" << endl;
	    QByteArray a(sizeof(machInfo));
	    a.duplicate((const char *)&machInfo, sizeof(machInfo));
	    data(a);
	    data(QByteArray());
	}
	    break;
	default:
	    error(ERR_UNSUPPORTED_PROTOCOL, QString(i18n("Code: %1")).arg(cmd));
	    return;
    }
    finished();
}

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
