#include "StdAfx.h"
#include "Network.h"

NwSocket::NwSocket(ost::IPV4Address &addr, int iPort) :
		TCPSocket(addr, iPort)
{
}

NwSocket::~NwSocket()
{
}

bool NwSocket::IsOpen() const
{
	return isActive();
}

int NwSocket::Receive(void* Data, int MaxLength)
{
	int i = ::recv(getSocket(), (char *)Data, MaxLength, 0);
	if (i<0) i=0;

	return i;
}

int NwSocket::Send(const void* Data, int Length)
{
	int i = ::send(getSocket(), (char *)Data, Length, 0);
	if (i<0) i=0;

	return i;
}

bool NwSocket::EnumNetworkEvents(WSANETWORKEVENTS& WsaNe)
{
	// TODO: program this
	//return WSAEnumNetworkEvents(m_Socket, m_Event, &WsaNe) != SOCKET_ERROR;
	return false;
}
