#ifndef _rfsv16_h_
#define _rfsv16_h_

#include "rfsv.h"

class rfsv16 : public rfsv {
public:
	rfsv16(ppsocket *);

	Enum<rfsv::errs> fopen(const long, const char * const, long &);
	Enum<rfsv::errs> mktemp(long &, char * const);
	Enum<rfsv::errs> fcreatefile(const long, const char * const, long &);
	Enum<rfsv::errs> freplacefile(const long, const char * const, long &);
	Enum<rfsv::errs> fclose(const long);
	Enum<rfsv::errs> dir(const char * const, PlpDir &);
	Enum<rfsv::errs> fgetmtime(const char * const, PsiTime &);
	Enum<rfsv::errs> fsetmtime(const char * const, const PsiTime);
	Enum<rfsv::errs> fgetattr(const char * const, long &);
	Enum<rfsv::errs> fgeteattr(const char * const, long &, long &, PsiTime &);
	Enum<rfsv::errs> fsetattr(const char * const, const long seta, const long unseta);
	Enum<rfsv::errs> dircount(const char * const, long &);
	Enum<rfsv::errs> devlist(long &);
	Enum<rfsv::errs> devinfo(const int, long &, long &, long &, long &, char * const);
	Enum<rfsv::errs> fread(const long, unsigned char * const, const long, long &);
	Enum<rfsv::errs> fwrite(const long, const unsigned char * const, const long, long &);
	Enum<rfsv::errs> copyFromPsion(const char * const, const char * const, void *, cpCallback_t);
	Enum<rfsv::errs> copyToPsion(const char * const, const char * const, void *, cpCallback_t);
	Enum<rfsv::errs> copyOnPsion(const char *, const char *, void *, cpCallback_t);
	Enum<rfsv::errs> fsetsize(const long, const long);
	Enum<rfsv::errs> fseek(const long, const long, const long, long &);
	Enum<rfsv::errs> mkdir(const char * const);
	Enum<rfsv::errs> rmdir(const char * const);
	Enum<rfsv::errs> rename(const char * const, const char * const);
	Enum<rfsv::errs> remove(const char * const);
	Enum<rfsv::errs> opendir(const long, const char * const, rfsvDirhandle &);
	Enum<rfsv::errs> readdir(rfsvDirhandle &, PlpDirent &);
	Enum<rfsv::errs> closedir(rfsvDirhandle &);
	Enum<rfsv::errs> setVolumeName(const char, const char * const);

	long opMode(const long);

private:
	enum commands {
		FOPEN = 0, // File Open
		FCLOSE = 2, // File Close
		FREAD = 4, // File Read
		FDIRREAD = 6, // Read Directory entries
		FDEVICEREAD = 8, // Device Information
		FWRITE = 10, // File Write
		FSEEK = 12, // File Seek
		FFLUSH = 14, // Flush
		FSETEOF = 16,
		RENAME = 18,
		DELETE = 20,
		FINFO = 22,
		SFSTAT = 24,
		PARSE = 26,
		MKDIR = 28,
		OPENUNIQUE = 30,
		STATUSDEVICE = 32,
		PATHTEST = 34,
		STATUSSYSTEM = 36,
		CHANGEDIR = 38,
		SFDATE = 40,
		RESPONSE = 42
	};
  
	enum fopen_attrib {
		P_FOPEN = 0x0000, /* Open file */
		P_FCREATE = 0x0001, /* Create file */
		P_FREPLACE = 0x0002, /* Replace file */
		P_FAPPEND = 0x0003, /* Append records */
		P_FUNIQUE = 0x0004, /* Unique file open */
		P_FSTREAM = 0x0000, /* Stream access to a binary file */
		P_FSTREAM_TEXT = 0x0010, /* Stream access to a text file */
		P_FTEXT = 0x0020, /* Record access to a text file */
		P_FDIR = 0x0030, /* Record access to a directory file */
		P_FFORMAT = 0x0040, /* Format a device */
		P_FDEVICE = 0x0050, /* Record access to device name list */
		P_FNODE = 0x0060, /* Record access to node name list */
		P_FUPDATE = 0x0100, /* Read and write access */
		P_FRANDOM = 0x0200, /* Random access */
		P_FSHARE = 0x0400 /* File can be shared */
	};

	enum status_enum {
		P_FAWRITE  = 0x0001, /* can the file be written to? */    
		P_FAHIDDEN = 0x0002, /* set if file is hidden */    
		P_FASYSTEM = 0x0004, /* set if file is a system file */
		P_FAVOLUME = 0x0008, /* set if the name is a volume name */    
		P_FADIR    = 0x0010, /* set if file is a directory file */    
		P_FAMOD    = 0x0020, /* has the file been modified? */    
		P_FAREAD   = 0x0100, /* can the file be read? */    
		P_FAEXEC   = 0x0200, /* is the file executable? */
		P_FASTREAM = 0x0400, /* is the file a byte stream file? */    
		P_FATEXT   = 0x0800, /* is it a text file? */
		P_FAMASK   = 0x0f3f  /* All of the above */
	};
  

	// Miscellaneous
	Enum<rfsv::errs> convertName(const char*, char *);
	Enum<rfsv::errs> fopendir(const char * const, long &);
	long attr2std(const long);
	long std2attr(const long);

	// Communication
	bool sendCommand(enum commands, bufferStore &);
	Enum<rfsv::errs> getResponse(bufferStore &);
};

#endif
