/*  
    A KIOslave for KDE2

    Copyright (C) 2001 Fritz Elfert <felfert@to.com>

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kio_plp.h"

#include <iomanip>
#include <stdio.h>
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

int
kdemain( int argc, char **argv ) {
	KInstance instance( "kio_nfs" );

	if (argc != 4) {
		fprintf(stderr, "Usage: kio_nfs protocol domain-socket1 domain-socket2\n");
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
#if 0
	cout << "uids:" << endl;
	for (UidMap::Iterator it = puids.begin(); it != puids.end(); it++) {
		cout << "UID: " << hex << setw(8) << setfill('0') << it.key().uid[0]
		     << it.key().uid[1] << it.key().uid[2] << dec << "->" <<
			 it.data() << endl;
	}
#endif
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
	if (!plpRfsvSocket->connect((char *)(currentHost.data()), currentPort)) {
		error(ERR_COULD_NOT_CONNECT, i18n("Could not connect to ncpd"));
		return;
	}
	rfsvfactory factory(plpRfsvSocket);
	plpRfsv = factory.create(false);
	if (plpRfsv == 0) {
		error(ERR_COULD_NOT_CONNECT, i18n("Could not read version info"));
		return;
	}

	/* If we have a S5, get the Psion's Owner- and Mach- info.
	 * This implicitely sets the Timezone info of the Psion also.
	 */

	ppsocket rpcsSocket;
	if (rpcsSocket.connect((char *)(currentHost.data()), currentPort)) {
		rpcsfactory factory(&rpcsSocket);
		rpcs *Rpcs = factory.create(false);
		if (Rpcs != 0L) {
			bufferArray b;
			Enum <rfsv::errs> res;
			if ((res = Rpcs->getOwnerInfo(b)) == rfsv::E_PSI_GEN_NONE) {
				Rpcs->getMachineType(machType);
				if (machType == rpcs::PSI_MACH_S5)
					Rpcs->getMachineInfo(mi);
			}
		}
	}

	u_int32_t devbits;
	Enum<rfsv::errs> res;

	if ((res = plpRfsv->devlist(devbits)) == rfsv::E_PSI_GEN_NONE) {
		for (int i = 0; i < 26; i++) {
			string vname;
			u_int32_t vtotal, vfree, vattr, vuniqueid;

			if ((devbits & 1) != 0) {
				if (plpRfsv->devinfo(i, vfree, vtotal, vattr, vuniqueid,
						     vname) == rfsv::E_PSI_GEN_NONE) {
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
	QString path(QFile::encodeName(url.path()));

	if (path.isEmpty()) {
		url.setPath("/");
		redirection(url);
		finished();
		return;
	}

	if (checkConnection())
		return;

	if (isRoot(path)) {
		kdDebug(PLP_DEBUGAREA) << "listing root" << endl;
		totalSize(drives.count());
		//in this case we don't need to do a real listdir
		UDSEntry entry;
		for (QStringList::Iterator it = drives.begin(); it != drives.end(); it++) {
			UDSAtom atom;
			entry.clear();
			atom.m_uds = KIO::UDS_NAME;
			atom.m_str = (*it);
			kdDebug(PLP_DEBUGAREA) << "listing " << (*it) << endl;
			entry.append(atom);
			createVirtualDirEntry(entry, drivechars[*it] == 'Z');
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
	Enum<rfsv::errs> res = plpRfsv->dir(path, files);
	if (checkForError(res))
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
createVirtualDirEntry(UDSEntry & entry, bool rdonly) {
	UDSAtom atom;

	atom.m_uds = KIO::UDS_FILE_TYPE;
	atom.m_long = S_IFDIR;
	entry.append( atom );

	atom.m_uds = KIO::UDS_ACCESS;
	atom.m_long = S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
	if (!rdonly)
		atom.m_long |= (S_IWUSR | S_IWGRP | S_IWOTH);
	entry.append( atom );

	atom.m_uds = KIO::UDS_SIZE;
	atom.m_long = 0;
	entry.append( atom );
}

bool PLPProtocol::
emitTotalSize(QString &name) {
	PlpDirent e;

	Enum<rfsv::errs> res = plpRfsv->fgeteattr(name, e);
	if (checkForError(res))
		return true;
	totalSize(e.getSize());
	return false;
}

void PLPProtocol::
stat(const KURL & url) {
	QString path(QFile::encodeName(url.path()));
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
		createVirtualDirEntry(entry, isRoot(path) || isRomDrive(path));
		statEntry(entry);
		finished();
		kdDebug(PLP_DEBUGAREA) << "succeeded" << endl;
		return;
	}

	bool rom = isRomDrive(path);
	QString fileName = baseName(path);
	convertName(path);

	if (path.isEmpty()) {
		error(ERR_DOES_NOT_EXIST, fileName);
		return;
	}

	PlpDirent e;

	Enum<rfsv::errs> res = plpRfsv->fgeteattr(path, e);
	if (checkForError(res))
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
	QString path(QFile::encodeName(url.path()));
	UDSEntry entry;
	UDSAtom atom;

	if (checkConnection())
		return;

	kdDebug(PLP_DEBUGAREA) << "mimetype(" << path << ")" << endl;
	stripTrailingSlash(path);

	if (isRoot(path) || isDrive(path)) {
		mimeType("inode/directory");
		finished();
		return;
	}
	convertName(path);

	if (path.isEmpty()) {
		error(ERR_DOES_NOT_EXIST, path);
		return;
	}

	PlpDirent e;
	Enum<rfsv::errs> res = plpRfsv->fgeteattr(path, e);
	if (checkForError(res))
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
	QString name(QFile::encodeName(url.path()));

	if (checkConnection())
		return;
	kdDebug(PLP_DEBUGAREA) << "mkdir(" << name << ")" << endl;
	if (isRomDrive(name)) {
		error(ERR_ACCESS_DENIED, i18n("read only filesystem"));
		return;
	}
	if (isRoot(name) || isDrive(name)) {
		error(ERR_ACCESS_DENIED, i18n("Virtual directory"));
		return;
	}
	convertName(name);
	Enum<rfsv::errs> res = plpRfsv->mkdir(name);
	if (checkForError(res))
		return;
	finished();
}

bool PLPProtocol::
checkForError(Enum<rfsv::errs> res) {
	if (res != rfsv::E_PSI_GEN_NONE) {
		kdDebug(PLP_DEBUGAREA) << "plp error: " << res << endl;
		QString text(res.toString().data());
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
	QString name(QFile::encodeName(url.path()));

	if (checkConnection())
		return;
	kdDebug(PLP_DEBUGAREA) << "del(" << name << ")" << endl;
	if (isRomDrive(name)) {
		error(ERR_ACCESS_DENIED, i18n("read only filesystem"));
		return;
	}
	if (isRoot(name) || isDrive(name)) {
		error(ERR_ACCESS_DENIED, i18n("Virtual directory"));
		return;
	}
	convertName(name);

	Enum<rfsv::errs> res =
		(isfile) ? plpRfsv->remove(name) : plpRfsv->rmdir(name);
	if (checkForError(res))
		return;
	finished();
}

void PLPProtocol::
chmod( const KURL& url, int permissions ) {
	kdDebug(PLP_DEBUGAREA) << "del" << endl;
	QString name(QFile::encodeName(url.path()));

	if (checkConnection())
		return;
	kdDebug(PLP_DEBUGAREA) << "chmod(" << name << ")" << endl;
	if (isRomDrive(name)) {
		error(ERR_ACCESS_DENIED, i18n("read only filesystem"));
		return;
	}
	if (isRoot(name) || isDrive(name)) {
		error(ERR_ACCESS_DENIED, i18n("Virtual directory"));
		return;
	}
	convertName(name);
	long attr[2];
	attr[0] = attr[1] = 0;
	Enum <rfsv::errs> res = plpRfsv->fsetattr(name, attr[0], attr[1]);
	if (checkForError(res))
		return;
	finished();
}

void PLPProtocol::
get( const KURL& url ) {
	kdDebug(PLP_DEBUGAREA) << "get" << endl;
	QString name(QFile::encodeName(url.path()));

	if (checkConnection())
		return;
	kdDebug(PLP_DEBUGAREA) << "get(" << name << ")" << endl;
	if (isRoot(name) || isDrive(name)) {
		error(ERR_ACCESS_DENIED, i18n("Virtual directory"));
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
	res = plpRfsv->fopen(plpRfsv->opMode(rfsv::PSI_O_RDONLY), name, handle);
	if (checkForError(res))
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
	if (checkForError(res))
		return;
	data(QByteArray());

	finished();
}

//TODO the partial putting thing is not yet implemented
void PLPProtocol::
put( const KURL& url, int _mode, bool _overwrite, bool /*_resume*/ ) {
	kdDebug(PLP_DEBUGAREA) << "get" << endl;
	QString name(QFile::encodeName(url.path()));

	if (checkConnection())
		return;
	kdDebug(PLP_DEBUGAREA) << "put(" << name << ")" << endl;
	if (isRomDrive(name)) {
		error(ERR_ACCESS_DENIED, i18n("read only filesystem"));
		return;
	}
	if (isRoot(name) || isDrive(name)) {
		error(ERR_ACCESS_DENIED, i18n("Virtual directory"));
		return;
	}
	convertName(name);

	Enum<rfsv::errs> res;
	u_int32_t handle;
	int result;

	res = plpRfsv->fcreatefile(plpRfsv->opMode(rfsv::PSI_O_RDWR), name, handle);
	if ((res == rfsv::E_PSI_FILE_EXIST) && _overwrite)
		res = plpRfsv->freplacefile(plpRfsv->opMode(rfsv::PSI_O_RDWR), name, handle);
	if (checkForError(res))
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
				if (checkForError(res)) {
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
	QString from(QFile::encodeName(src.path()));
	QString to(QFile::encodeName(dest.path()));

	if (checkConnection())
		return;
	kdDebug(PLP_DEBUGAREA) << "rename(" << from << "," << to << ")" << endl;
	if ((driveChar(from) != driveChar(to)) && (driveChar(to) != '\0')) {
		error(ERR_ACCESS_DENIED, i18n("Virtual directory"));
		kdDebug(PLP_DEBUGAREA) << "from FS != to FS" << endl;
		return;
	}
	if (isRomDrive(from)) {
		error(ERR_ACCESS_DENIED, i18n("read only filesystem"));
		kdDebug(PLP_DEBUGAREA) << "from ROFS" << endl;
		return;
	}
	if (isRoot(from)) {
		error(ERR_ACCESS_DENIED, i18n("Virtual directory"));
		kdDebug(PLP_DEBUGAREA) << "from VFS" << endl;
		return;
	}
	bool volRename = isDrive(from);
	if (isRomDrive(to)) {
		error(ERR_ACCESS_DENIED, i18n("read only filesystem"));
		kdDebug(PLP_DEBUGAREA) << "to ROFS" << endl;
		return;
	}
	if (isRoot(to)) {
		error(ERR_ACCESS_DENIED, i18n("Virtual directory"));
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
			if ((res = plpRfsv->fgetattr(to, attr)) == rfsv::E_PSI_GEN_NONE) {

				error(ERR_FILE_ALREADY_EXIST, to);
				return;
			}
		}
		res = plpRfsv->rename(from, to);
	}
	if (checkForError(res))
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
	QString from( QFile::encodeName(src.path()));
	QString to( QFile::encodeName(dest.path()));

	if (checkConnection())
		return;
	kdDebug(PLP_DEBUGAREA) << "copy(" << from << "," << to << ")" << endl;
	if (isRoot(from) || isDrive(from)) {
		error(ERR_ACCESS_DENIED, i18n("Virtual directory"));
		return;
	}
	convertName(from);
	if (isRomDrive(to)) {
		error(ERR_ACCESS_DENIED, i18n("read only filesystem"));
		return;
	}
	if (isRoot(to) || isDrive(to)) {
		error(ERR_ACCESS_DENIED, i18n("Virtual directory"));
		return;
	}
	convertName(to);
	Enum <rfsv::errs> res;
	if (!_overwrite) {
		u_int32_t attr;
		if ((res = plpRfsv->fgetattr(to, attr)) == rfsv::E_PSI_GEN_NONE) {
			error(ERR_FILE_ALREADY_EXIST, to);
			return;
		}
	}
	if (emitTotalSize(from))
	    return;
	t_last = t_start = time(0);
	res = plpRfsv->copyOnPsion(from, to, (void *)this, progresswrapper);
	if (checkForError(res))
		return;
	finished();
}
