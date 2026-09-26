#include "farmpch.h"
#include "farm/Network.h"

NwSocket::NwSocket(SOCKET Socket, WSAEVENT Event) :
		m_Socket(Socket),
		m_Event(Event)
{
}

NwSocket::~NwSocket()
{
	Close();
}

// The connect function establishes a connection to a specified socket.
bool NwSocket::Connect(const NwSockAddr& SockAddr)
{
	Close();

	if ((m_Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
		return false;

	if (connect(m_Socket, SockAddr, SockAddr.GetLength()) == SOCKET_ERROR)
	{
		Close();
		return false;
	}

	if ((m_Event = WSACreateEvent()) == INVALID_HANDLE_VALUE)
	{
		Close();
		return false;
	}

	if (WSAEventSelect(m_Socket, m_Event, FD_WRITE | FD_READ | FD_CLOSE) == SOCKET_ERROR)
	{
		Close();
		return false;
	}

	return true;
}

// The listen function places a socket in a state in which it is listening for an incoming connection.
bool NwSocket::Listen(const NwSockAddr& SockAddr)
{
	Close();

	if ((m_Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
		return false;

	if (bind(m_Socket, SockAddr, SockAddr.GetLength()) == SOCKET_ERROR)
	{
		Close();
		return false;
	}

	if (listen(m_Socket, 5) == SOCKET_ERROR)
	{
		Close();
		return false;
	}

	if ((m_Event = WSACreateEvent()) == INVALID_HANDLE_VALUE)
	{
		Close();
		return false;
	}

	if (WSAEventSelect(m_Socket, m_Event, FD_ACCEPT | FD_CLOSE) == SOCKET_ERROR)
	{
		Close();
		return false;
	}

	return true;
}

// The accept function permits an incoming connection attempt on a socket.
bool NwSocket::Accept(NwSocket& Socket)
{
	if (m_Socket == INVALID_SOCKET)
		return false;

	SOCKET RawSocket(accept(m_Socket, 0, 0));

	if (RawSocket == INVALID_SOCKET)
		return false;

	if (Socket.IsOpen())
		Socket.Close();

	Socket.m_Socket = RawSocket;

	if ((Socket.m_Event = WSACreateEvent()) == INVALID_HANDLE_VALUE)
	{
		Socket.Close();
		return false;
	}

	if (WSAEventSelect(Socket.m_Socket, Socket.m_Event, FD_WRITE | FD_READ | FD_CLOSE) == SOCKET_ERROR)
	{
		Socket.Close();
		return false;
	}

	return true;
}

bool NwSocket::Broadcast(int Port)
{
	Close();

	if ((m_Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
		return false;

	BOOL True(TRUE);

	if (setsockopt(m_Socket, SOL_SOCKET, SO_BROADCAST, (char*)&True, sizeof(True)) == SOCKET_ERROR)
	{
		Close();
		return false;
	}

	if (setsockopt(m_Socket, SOL_SOCKET, SO_REUSEADDR, (char*)&True, sizeof(True)) == SOCKET_ERROR)
	{
		Close();
		return false;
	}

	NwSockAddr SockAddr(INADDR_ANY, Port);

	if (bind(m_Socket, SockAddr, SockAddr.GetLength()) == SOCKET_ERROR)
	{
		Close();
		return false;
	}

	if ((m_Event = WSACreateEvent()) == INVALID_HANDLE_VALUE)
	{
		Close();
		return false;
	}

	if (WSAEventSelect(m_Socket, m_Event, FD_READ | FD_CLOSE) == SOCKET_ERROR)
	{
		Close();
		return false;
	}

	return true;
}

// The closesocket function closes an existing socket.
void NwSocket::Close()
{
	if (m_Socket != INVALID_SOCKET)
	{
		closesocket(m_Socket);
		m_Socket = INVALID_SOCKET;
	}

	if (m_Event != INVALID_HANDLE_VALUE)
	{
		WSACloseEvent(m_Event);
		m_Event = INVALID_HANDLE_VALUE;
	}
}

bool NwSocket::IsOpen() const
{
	return m_Socket != INVALID_SOCKET;
}

SOCKET NwSocket::GetSocket()
{
	return m_Socket;
}

WSAEVENT NwSocket::GetEvent()
{
	return m_Event;
}

SOCKET NwSocket::DetachSocket()
{
	return PushAssign(m_Socket, INVALID_SOCKET);
}

WSAEVENT NwSocket::DetachEvent()
{
	return PushAssign(m_Event, INVALID_HANDLE_VALUE);
}

// The recv function receives data from a connected or bound socket.
int NwSocket::Receive(void* Data, int MaxLength)
{
	if (m_Socket == INVALID_SOCKET)
		return false;

	int RecvLength(recv(m_Socket, (char*)Data, MaxLength, 0));

	if (RecvLength == 0)
		return -1;

	if (RecvLength < 0)
	{
		switch(WSAGetLastError())
		{
			case WSAEWOULDBLOCK:	return 0;
			default:				return -1;
		}
	}

	return RecvLength;
}

// The send function sends data on a connected socket.
int NwSocket::Send(const void* Data, int Length)
{
	if (m_Socket == INVALID_SOCKET)
		return false;

	int SendLength(send(m_Socket, (const char*)Data, Length, 0));

	if (SendLength == 0)
		return -1;

	if (SendLength < 0)
	{
		switch(WSAGetLastError())
		{
			case WSAEWOULDBLOCK:	return 0;
			default:				return -1;
		}
	}

	return SendLength;
}

// The recvfrom function receives a datagram and stores the source address.
int NwSocket::ReceiveFrom(void* Data, int MaxLength, NwSockAddr& Address)
{
	if (m_Socket == INVALID_SOCKET)
		return false;

	int AddrLength(Address.GetLength());
	int RecvLength(recvfrom(m_Socket, (char*)Data, MaxLength, 0, Address, &AddrLength));

	if (RecvLength == 0)
		return -1;

	if (RecvLength < 0)
	{
		switch(WSAGetLastError())
		{
			case WSAEWOULDBLOCK:	return 0;
			default:				return -1;
		}
	}

	return RecvLength;
}

// The sendto function sends data to a specific destination.
int NwSocket::SendTo(const void* Data, int Length, const NwSockAddr& Address)
{
	if (m_Socket == INVALID_SOCKET)
		return false;

	int SendLength(sendto(m_Socket, (const char*)Data, Length, 0, Address, Address.GetLength()));

	if (SendLength == 0)
		return -1;

	if (SendLength < 0)
	{
		switch(WSAGetLastError())
		{
			case WSAEWOULDBLOCK:	return 0;
			default:				return -1;
		}
	}

	return SendLength;
}

// The WSAEnumNetworkEvents function discovers occurrences of network events for the indicated socket,
// clear internal network event records, and reset event objects (optional).
bool NwSocket::EnumNetworkEvents(WSANETWORKEVENTS& WsaNe)
{
	return WSAEnumNetworkEvents(m_Socket, m_Event, &WsaNe) != SOCKET_ERROR;
}