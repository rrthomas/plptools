//
//  PLP - An implementation of the PSION link protocol
//
//  Copyright (C) 1999  Philip Proudman
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//  e-mail philip.proudman@btinternet.com


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <iostream.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "defs.h"
#include "bool.h"
#include "bufferstore.h"
#include "ppsocket.h"

//**********************************************************************
// For unix we need a few definitions
//**********************************************************************

#ifndef MAKEWORD
#define MAKEWORD(a, b) ((WORD)(((BYTE)(a)) | ((WORD)((BYTE)(b))) << 8))
#endif
#ifndef SHORT
#define SHORT int
#endif

//This constructor is useful when converting a socket
//to a derived class of socket
ppsocket::ppsocket(const ppsocket & another)
{
	m_Socket = another.m_Socket;
	m_HostAddr = another.m_HostAddr;
	m_PeerAddr = another.m_PeerAddr;
	m_Bound = another.m_Bound;
	m_Timeout = another.m_Timeout;
	m_LastError = another.m_LastError;

	m_Timeout = INFINITE;
}


ppsocket::ppsocket()
{
	m_Socket = INVALID_SOCKET;

	memset(&m_HostAddr, 0, sizeof(m_HostAddr));
	memset(&m_PeerAddr, 0, sizeof(m_PeerAddr));


	((struct sockaddr_in *) &m_HostAddr)->sin_family = AF_INET;
	((struct sockaddr_in *) &m_PeerAddr)->sin_family = AF_INET;

	m_Bound = false;
	m_LastError = 0;
	m_Timeout = INFINITE;
}

ppsocket::~ppsocket()
{
	if (m_Socket != INVALID_SOCKET) {
		shutdown(m_Socket, 3);
		close(m_Socket);
	}
}

bool ppsocket::
reconnect()
{
	if (m_Socket != INVALID_SOCKET) {
		shutdown(m_Socket, 3);
		close(m_Socket);
	}
	m_Socket = INVALID_SOCKET;
	if (!createSocket())
		return (false);
	m_LastError = 0;
	m_Bound = 0;
	if (::bind(m_Socket, &m_HostAddr, sizeof(m_HostAddr)) != 0) {
		m_LastError = lastErrorCode();
		return (false);
	}
	if (::connect(m_Socket, &m_PeerAddr, sizeof(m_PeerAddr)) != 0) {
		m_LastError = lastErrorCode();
		return (false);
	}
	return (true);
}

void ppsocket::
printPeer()
{
	char *pPeer = 0;
	int port;

	pPeer = inet_ntoa(((struct sockaddr_in *) &m_PeerAddr)->sin_addr);
	if (pPeer) {
		port = ntohs(((struct sockaddr_in *) &m_PeerAddr)->sin_port);
		cout << "Peer : " << pPeer << " Port : " << port << endl;
	} else
		cerr << "Error getting Peer details\n";
}

bool ppsocket::
connect(char *Peer, int PeerPort, char *Host, int HostPort)
{
	//****************************************************
	//* If we aren't already bound set the host and bind *
	//****************************************************

	if (!bindSocket(Host, HostPort)) {
		if (m_LastError != 0) {
			return (false);
		}
	}
	//****************
	//* Set the peer *
	//****************
	if (!setPeer(Peer, PeerPort)) {
		return (false);
	}
	//***********
	//* Connect *
	//***********
	if (::connect(m_Socket, &m_PeerAddr, sizeof(m_PeerAddr)) != 0) {
		m_LastError = lastErrorCode();
		return (false);
	}
	return (true);
}

bool ppsocket::
listen(char *Host, int Port)
{
	//****************************************************
	//* If we aren't already bound set the host and bind *
	//****************************************************

	if (!bindSocket(Host, Port)) {
		if (m_LastError != 0) {
			return (false);
		}
	}
	//**********************
	//* Listen on the port *
	//**********************

	if (::listen(m_Socket, 5) != 0) {
		m_LastError = lastErrorCode();
		return (false);
	}
	// Our accept member function relies on non-blocking accepts,
	// so set the flag here (rather than every time around the loop)
	fcntl(m_Socket, F_SETFL, O_NONBLOCK);
	return (true);
}

ppsocket *ppsocket::
accept(char *Peer, int MaxLen)
{
	socklen_t len;
	ppsocket *accepted;
	char *peer;

	//*****************************************************
	//* Allocate a new object to hold the accepted socket *
	//*****************************************************

	accepted = new ppsocket;

	if (!accepted) {
		m_LastError = lastErrorCode();
		return (NULL);
	}
	//***********************
	//* Accept a connection *
	//***********************

	len = sizeof(struct sockaddr);
	accepted->m_Socket =::accept(m_Socket, &accepted->m_PeerAddr, &len);

	if (accepted->m_Socket == INVALID_SOCKET) {
		m_LastError = lastErrorCode();
		delete accepted;
		return (NULL);
	}
	//****************************************************
	//* Got a connection so fill in the other attributes *
	//****************************************************

	// Make sure the new socket hasn't inherited O_NONBLOCK
	// from the accept socket
	int flags = fcntl( accepted->m_Socket, F_GETFL, 0 );
	if( flags >= 0 ) {
		flags &= ~O_NONBLOCK;
		fcntl( accepted->m_Socket, F_SETFL, flags);
	}

	accepted->m_HostAddr = m_HostAddr;
	accepted->m_Bound = true;

	//****************************************************
	//* If required get the name of the connected client *
	//****************************************************

	if (Peer) {
		peer = inet_ntoa(((struct sockaddr_in *) &accepted->m_PeerAddr)->sin_addr);
		if (peer) {
			strncpy(Peer, peer, MaxLen);
			Peer[MaxLen] = '\0';
		}
	} else {
		strcpy(Peer, "");
	}
	return (accepted);
}

int ppsocket::
printf(const char *Format,...)
{
	int i;
	va_list ap;
	char s[512];

	va_start(ap, Format);
	vsprintf(s, Format, ap);
	va_end(ap);

	i = writeTimeout(s, strlen(s), 0);

	return (i);
}

bool ppsocket::
dataToGet() const
{
	fd_set io;
	FD_ZERO(&io);
	FD_SET(m_Socket, &io);
	struct timeval t;
	t.tv_usec = 0;
	t.tv_sec = 0;
	return (select(m_Socket + 1, &io, NULL, NULL, &t) != 0) ? true : false;
}

int ppsocket::
getBufferStore(bufferStore & a, bool wait)
{
	/* Returns a 0 for for no message,
	 * 1 for message OK, and -1 for socket problem
	 */

	long l;
	long count = 0;
	unsigned char *buff;
	unsigned char *bp;
	if (!wait && !dataToGet())
		return 0;
	a.init();

	if (readTimeout((char *) &l, sizeof(l), 0) != sizeof(l))
		return -1;
	l = ntohl(l);
	bp = buff = new unsigned char[l];
	while (l > 0) {
		int j = readTimeout((char *) bp, l, 0);
		if (j == SOCKET_ERROR || j == 0) {
			delete[]buff;
			return -1;
		}
		count += j;
		l -= j;
		bp += j;
	};
	a.init(buff, count);
	delete[]buff;
	return (a.getLen() == 0) ? 0 : 1;
}

bool ppsocket::
sendBufferStore(const bufferStore & a)
{
	long l = a.getLen();
	long hl = htonl(l);
	long sent = 0;
	int retries = 0;
	int i;

	i = writeTimeout((char *) &hl, sizeof(hl), 0);
	if (i != sizeof(hl))
		return false;
	while (l > 0) {
		i = writeTimeout(a.getString(sent), l, 0);
		if (i == SOCKET_ERROR || i == 0)
			return (false);
		sent += i;
		l -= i;
		if (++retries > 5) {
			m_LastError = 0;
			return (false);
		}
	}
	return true;
}

int ppsocket::
readEx(char *Data, int cTerm, int MaxLen)
{
	int i, j;

	for (i = 0; i < MaxLen; i++) {
		j = readTimeout(Data + i, 1, 0);

		if (j == SOCKET_ERROR || j == 0) {
			Data[i] = '\0';
			return (i > 0 ? i : 0);
		}
		if (Data[i] == cTerm)
			break;
	}

	return (i + 1);
}

bool ppsocket::
puts(const char *Data)
{
	int tosend, sent, retries, i;

	tosend = strlen(Data);
	sent = retries = 0;

	while (tosend > 0) {
		i = writeTimeout(Data + sent, tosend, 0);

		if (i == SOCKET_ERROR || i == 0)
			return (sent > 0 ? true : false);

		sent += i;
		tosend -= i;

		if (++retries > 5) {
			m_LastError = 0;
			return (false);
		}
	}

	return (true);
}

char ppsocket::
sgetc(void)
{
	int i;
	char c;

	i = readTimeout(&c, 1, 0);
	if (i == SOCKET_ERROR || i == 0) {
		return (0);
	}
	return (c);
}

bool ppsocket::
sputc(char c)
{
	int i;

	cout << hex << (int) c << endl;
	i = writeTimeout(&c, 1, 0);
	if (i == SOCKET_ERROR || i == 0)
		return (false);
	return (true);
}

int ppsocket::
read(void *Data, size_t Size, size_t NumObj)
{
	int i = readTimeout((char *) Data, Size * NumObj, 0);

	return (i);
}

int ppsocket::
write(const void *Data, size_t Size, size_t NumObj)
{
	int i = writeTimeout((char *) Data, Size * NumObj, 0);

	return (i);
}

int ppsocket::
recv(char *buf, int len, int flags)
{
	int i =::recv(m_Socket, buf, len, flags);

	if (i < 0)
		m_LastError = lastErrorCode();

	return (i);
}

int ppsocket::
send(const char *buf, int len, int flags)
{
	int i =::send(m_Socket, buf, len, flags);

	if (i < 0)
		m_LastError = lastErrorCode();

	return (i);
}

int ppsocket::
readTimeout(char *buf, int len, int flags)
{
	int i;

	//*********************************************************
	//* If there is no timeout use the Berkeley recv function *
	//*********************************************************

	if (m_Timeout == INFINITE) {
		i =::recv(m_Socket, buf, len, flags);

		if (i == SOCKET_ERROR) {
			m_LastError = lastErrorCode();
		}
	}
	//********************************************
	//* If there is a timeout use overlapped i/o *
	//********************************************

	else {
		i = SOCKET_ERROR;
	}


	return (i);
}

int ppsocket::
writeTimeout(const char *buf, int len, int flags)
{
	int i;
	// If there is no timeout use the Berkeley send function

	if (m_Timeout == INFINITE) {
		i =::send(m_Socket, buf, len, flags);

		if (i == SOCKET_ERROR) {
			m_LastError = lastErrorCode();
		}
	} else {
		// If there is a timeout use overlapped i/o
		i = SOCKET_ERROR;
	}

	return (i);
}

bool ppsocket::
closeSocket(void)
{
	shutdown(m_Socket, 3);
	if (close(m_Socket) != 0) {
		m_LastError = lastErrorCode();
		return (false);
	}
	m_Socket = INVALID_SOCKET;

	return (true);
}

bool ppsocket::
bindSocket(char *Host, int Port)
{

	// If we are already bound return FALSE but with no last error

	m_LastError = 0;

	if (m_Bound) {
		return (false);
	}
	// If the socket hasn't been created create it now

	if (m_Socket == INVALID_SOCKET) {
		if (!createSocket()) {
			return (false);
		}
	}
	// Set SO_REUSEADDR
	int one = 1;
	if (setsockopt( m_Socket, SOL_SOCKET, SO_REUSEADDR, 
			(const char *) &one, sizeof(int)) < 0 )
	  	cerr << "Warning: Unable to set SO_REUSEADDR option\n";
	// If a host name was supplied then use it
	if (!setHost(Host, Port)) {
		return (false);
	}
	// Now bind the socket
	if (::bind(m_Socket, &m_HostAddr, sizeof(m_HostAddr)) != 0) {
		m_LastError = lastErrorCode();
		return (false);
	}
	m_Bound = true;

	return (true);
}

bool ppsocket::
bindInRange(char *Host, int Low, int High, int Retries)
{
	int port, i;

	// If we are already bound return FALSE but with no last error
	m_LastError = 0;
	if (m_Bound) {
		return (false);
	}
	// If the socket hasn't been created create it now
	if (m_Socket == INVALID_SOCKET) {
		if (!createSocket()) {
			return (false);
		}
	}
	// If the number of retries is greater than the range then work
	// through the range sequentially.
	if (Retries > High - Low) {
		for (port = Low; port <= High; port++) {
			if (!setHost(Host, port))
				return (false);

				if (::bind(m_Socket, &m_HostAddr, sizeof(m_HostAddr)) == 0)
				break;
		}
		if (port > High) {
			m_LastError = lastErrorCode();
			return (false);
		}
	} else {
		for (i = 0; i < Retries; i++) {
			port = Low + (rand() % (High - Low));
			if (!setHost(Host, port))
				return (false);
			if (::bind(m_Socket, &m_HostAddr, sizeof(m_HostAddr)) == 0)
				break;
		}
		if (i >= Retries) {
			m_LastError = lastErrorCode();
			return (false);
		}
	}
	m_Bound = true;
	return (true);
}

bool ppsocket::
linger(bool LingerOn, int LingerTime)
{
	int i;
	struct linger l;

	// If the socket hasn't been created create it now
	if (m_Socket == INVALID_SOCKET) {
		if (!createSocket()) {
			return (false);
		}
	}
	// Set the lingering
	if (LingerOn) {
		l.l_onoff = 1;
		l.l_linger = LingerTime;
	} else {
		l.l_onoff = 0;
		l.l_linger = 0;
	}
	i = setsockopt(m_Socket, SOL_SOCKET, SO_LINGER, (const char *) &l, sizeof(l));
	// Check for errors
	if (i != 0) {
		m_LastError = lastErrorCode();
		return (false);
	}
	// Return indicating success
	return (true);
}

bool ppsocket::
createSocket(void)
{
	// If the socket has already been created just return true
	if (m_Socket != INVALID_SOCKET)
		return (true);
	// Create the socket
	m_Socket =::socket(PF_INET, SOCK_STREAM, 0);
	if (m_Socket == INVALID_SOCKET) {
		m_LastError = lastErrorCode();
		return (false);
	}
	// By default set no lingering
	linger(FALSE, 0);
	// Return indicating success
	return (true);
}

bool ppsocket::
setPeer(char *Peer, int Port)
{
	struct hostent *he;

	// If a peer name was supplied then use it
	if (Peer) {
		he = gethostbyname(Peer);
		if (!he) {
			unsigned long ipaddress = inet_addr(Peer);
			if (ipaddress == INADDR_NONE) {
				m_LastError = lastErrorCode();
				return (false);
			}
			he = gethostbyaddr((const char *) &ipaddress, 4, PF_INET);
			if (!he) {
				m_LastError = lastErrorCode();
				return (false);
			}
		}
		memcpy((void *) &((struct sockaddr_in *) &m_PeerAddr)->sin_addr, (void *) he->h_addr_list[0], 4);
	}
	// If a port name was supplied use it
	if (Port > 0)
		((struct sockaddr_in *) &m_PeerAddr)->sin_port = htons((SHORT) Port);
	return (true);
}

bool ppsocket::
getPeer(char *Peer, int MaxLen, int *Port)
{
	char *peer;

	if (Peer) {
		peer = inet_ntoa(((struct sockaddr_in *) &m_PeerAddr)->sin_addr);
		if (!peer) {
			m_LastError = lastErrorCode();
			return (false);
		}
		strncpy(Peer, peer, MaxLen);
		Peer[MaxLen] = '\0';
	}
	if (Port)
		*Port = ntohs(((struct sockaddr_in *) &m_PeerAddr)->sin_port);
	return (false);
}

bool ppsocket::
setHost(char *Host, int Port)
{
	struct hostent *he;

	// If a host name was supplied then use it
	if (Host) {
		he = gethostbyname(Host);
		if (!he) {
			unsigned long ipaddress = inet_addr(Host);
			if (ipaddress == INADDR_NONE) {
				m_LastError = lastErrorCode();
				return (false);
			}
			he = gethostbyaddr((const char *) &ipaddress, 4, PF_INET);
			if (!he) {
				m_LastError = lastErrorCode();
				return (false);
			}
		}
		memcpy((void *) &((struct sockaddr_in *) &m_HostAddr)->sin_addr, (void *) he->h_addr_list[0], 4);
	}
	// If a port name was supplied use it

	if (Port > 0)
		((struct sockaddr_in *) &m_HostAddr)->sin_port = htons((SHORT) Port);
	return (true);
}

bool ppsocket::
getHost(char *Host, int MaxLen, int *Port)
{
	char *host;

	if (Host) {
		host = inet_ntoa(((struct sockaddr_in *) &m_HostAddr)->sin_addr);
		if (!host) {
			m_LastError = lastErrorCode();
			return (false);
		}
		strncpy(Host, host, MaxLen);
		Host[MaxLen] = '\0';
	}
	if (Port)
		*Port = ntohs(((struct sockaddr_in *) &m_HostAddr)->sin_port);
	return (false);
}

DWORD ppsocket::
lastErrorCode()
{
	return errno;
}

void ppsocket::
setSocketInvalid()
{
	m_Socket = INVALID_SOCKET;
}
