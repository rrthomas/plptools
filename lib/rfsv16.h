#ifndef _rfsv16_h_
#define _rfsv16_h_

#include "rfsv.h"

class rfsv16 : public rfsv {
	public:
	rfsv16(ppsocket *);
	~rfsv16();
	void reset(); // these 2 added
	void reconnect();

	// these are the original publics
	long dir(const char *, bufferArray *);
	/*long read(const char* psionName, const char* localName);
	long write(const char* localName, const char* psionName);*/
	long mkdir(const char *);

	// these are FS' promotions
	long dircount(const char *, long *);
	long copyFromPsion(const char *, const char *, cpCallback_t);
	long copyToPsion(const char *, const char *, cpCallback_t);
	long rmdir(const char *);
	long remove(const char *); // MJG: was this del? FE: yes
	long rename(const char *, const char *);
	long mktemp(long *, char *);
	long fgeteattr(const char *, long *, long *, long *);
	long fgetattr(const char *, long *);
	long fsetattr(const char *, long seta, long unseta);
	long fgetmtime(const char *, long *);
	long fsetmtime(const char *, long);
	long fopendir(long, const char *, long &);
	long fopen(long, const char *, long &);
	long fcreatefile(long, const char *, long &);
	long freplacefile(long, const char *, long &);
	long fseek(long, long, long);
	long fread(long, unsigned char *, long);
	long fwrite(long, unsigned char *, long);
	long fsetsize(long, long);
	long fclose(long);

	long devlist(long *);
	char *devinfo(int, long *, long *, long *, long *);
	long getStatus();
	char *opErr(long);
	char *opAttr(long);
	long opMode(long);

	long attr2std(long);
	long std2attr(long);

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
    P_FAWRITE = 0x0001, /* can the file be written to? */    
    P_FAHIDDEN = 0x0002, /* set if file is hidden */    
    P_FASYSTEM = 0x0004, /* set if file is a system file */
    P_FAVOLUME = 0x0008, /* set if the name is a volume name */    
    P_FADIR = 0x0010, /* set if file is a directory file */    
    P_FAMOD = 0x0020, /* has the file been modified? */    
    P_FAREAD = 0x0100, /* can the file be read? */    
    P_FAEXEC = 0x0200, /* is the file executable? */
    P_FASTREAM = 0x0400, /* is the file a byte stream file? */    
    P_FATEXT = 0x0800 /* is it a text file? */
  };
  
  const char *getConnectName();

  // File handlers
  //long fopen(fopen_attrib a, const char* file, int &handle); // returns status 0=OK
  //long fclose(int fileHandle);

	// Miscellaneous
	int convertName(const char*, char *);

	// Communication
	bool sendCommand(enum commands, bufferStore &);
	long getResponse(bufferStore &);
  
	// Vars
	ppsocket *skt;
	// MJG: not sure what these are yet
	int serNum;
	long status;	// current connection status
	int tDiff;	// don't think this is used anywhere
};

#endif
