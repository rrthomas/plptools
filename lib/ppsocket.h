#if !defined(AFX_ppsocket_H__5611BC0C_3E39_11D1_8E4B_00805F2AB205__INCLUDED_)
#define AFX_ppsocket_H__5611BC0C_3E39_11D1_8E4B_00805F2AB205__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#ifndef TRUE
#define  TRUE		-1
#define  FALSE		0
#endif
#define  DWORD		unsigned int
#define  SOCKET		int

#ifndef INADDR_NONE
#define  INADDR_NONE (in_addr_t)-1
#endif
#define  INVALID_SOCKET	-1
#define  SOCKET_ERROR	-1
#define  INFINITE	0

extern int errno;

class bufferStore;

class ppsocket
{

public:
  ppsocket();
  virtual ~ppsocket();

  virtual bool connect(char* Peer, int PeerPort, char* Host = NULL, int HostPort = 0);
  virtual bool reconnect();
  virtual void printPeer();
  virtual bool listen(char* Host, int Port);
  ppsocket* accept(char* Peer, int MaxLen);
  bool dataToGet() const;
  int getBufferStore(bufferStore &a, bool wait = true);
  bool sendBufferStore(const bufferStore &a);
  int printf(const char* Format, ...);
  int   readEx(unsigned char *Data, int cTerm, int MaxLen);
  bool  puts(const char* Data);
  char sgetc(void);
  bool sputc(char c);
  int read(void* Data, size_t Size, size_t NumObj);
  virtual int write(const void* Data, size_t Size, size_t NumObj);
  int recv(char* buf, int len, int flags);
  int send(const char* buf, int len, int flags);
  int readTimeout(void * buf, int len, int flags);
  int writeTimeout(const char *buf, int len, int flags);
  inline void timeout(DWORD t) { m_Timeout = t; }
  inline DWORD timeout(void) { return(m_Timeout); }
  bool closeSocket(void);
  bool bindSocket(char* Host, int Port);
  bool bindInRange(char* Host, int Low, int High, int Retries);
  bool linger(bool LingerOn, int LingerTime = 0);
  virtual bool createSocket(void);
  bool setPeer(char *Peer, int Port);
  bool getPeer(char *Peer, int MaxLen, int* Port);
  bool setHost(char *Host, int Port);
  bool getHost(char *Host, int MaxLen, int* Port);
  DWORD getLastError(void) { return(m_LastError); }
  inline SOCKET socket(void) const { return(m_Socket); }
  DWORD  lastErrorCode();

  //set the current socket to invalid
  //useful when deleting if the socket has been 
  //copied to another and should not be closed
  //when this instance is destructed
  void setSocketInvalid();

protected:
  ppsocket(const ppsocket&);
  struct sockaddr* getPeerAddrStruct() { return &m_PeerAddr; }
  struct sockaddr* getHostAddrStruct() { return &m_HostAddr; }
  void setHostAddrStruct(struct sockaddr* pHostStruct) { m_HostAddr = *pHostStruct; }
  void setSocket(SOCKET& sock) { m_Socket = sock; }
  void setBound() { m_Bound = true;}
  void updateLastError() {m_LastError = lastErrorCode(); }

private:
  SOCKET m_Socket;
  struct sockaddr m_HostAddr, m_PeerAddr;
  int m_Port;
  bool   m_Bound;
#ifdef WIN32
  OVERLAPPED m_ReadOverlapped, m_WriteOverlapped;
#endif
  DWORD m_Timeout;
  DWORD m_LastError;
};

#endif
