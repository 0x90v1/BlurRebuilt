#pragma once

// describe a network address for use with NwSocket class
class NwSockAddr
{
public:
	inline NwSockAddr()
	{
		ZeroMemory(&m_SockAddr, sizeof(m_SockAddr));

		m_SockAddr.sin_family = AF_INET;
	}

	inline NwSockAddr(IN_ADDR Addr, u16 Port = 0)
	{
		ZeroMemory(&m_SockAddr.sin_zero, sizeof(m_SockAddr.sin_zero));

		m_SockAddr.sin_family	= AF_INET;
		m_SockAddr.sin_port		= htons(Port);
		m_SockAddr.sin_addr		= Addr;
	}

	inline NwSockAddr(u32 Addr, u16 Port = 0)
	{
		ZeroMemory(&m_SockAddr.sin_zero, sizeof(m_SockAddr.sin_zero));

		m_SockAddr.sin_family			= AF_INET;
		m_SockAddr.sin_port				= htons(Port);
		m_SockAddr.sin_addr.S_un.S_addr	= Addr;
	}

public:
	inline operator SOCKADDR*() { return (SOCKADDR*)&m_SockAddr; }
	inline operator const SOCKADDR*() const { return (const SOCKADDR*)&m_SockAddr; }

public:
	inline bool operator==(const NwSockAddr& Rhs) const
	{
		return (m_SockAddr.sin_addr.S_un.S_addr == Rhs.m_SockAddr.sin_addr.S_un.S_addr) && (m_SockAddr.sin_port == Rhs.m_SockAddr.sin_port);
	}

public:
	inline u16		GetPort() const { return m_SockAddr.sin_port; }
	inline IN_ADDR	GetAddr() const { return m_SockAddr.sin_addr; }
	int				GetLength() const { return sizeof(m_SockAddr); }

private:
	SOCKADDR_IN m_SockAddr;
};

// winsock encapsulation class
class NwSocket
{
public:
	NwSocket(SOCKET Socket = INVALID_SOCKET, WSAEVENT Event = INVALID_HANDLE_VALUE);
	~NwSocket();

public:
	bool Connect(const NwSockAddr& SockAddr);
	bool Listen(const NwSockAddr& SockAddr);
	bool Accept(NwSocket& Socket);
	bool Broadcast(int Port);
	void Close();

public:
	bool IsOpen() const;

public:
	SOCKET GetSocket();
	WSAEVENT GetEvent();

public:
	SOCKET DetachSocket();
	WSAEVENT DetachEvent();

public:
	int Receive(void* Data, int MaxLength);
	int Send(const void* Data, int Length);

public:
	int ReceiveFrom(void* Data, int MaxLength, NwSockAddr& Address);
	int SendTo(const void* Data, int Length, const NwSockAddr& Address);

public:
	bool EnumNetworkEvents(WSANETWORKEVENTS& WsaNe);

private:
	SOCKET		m_Socket;
	WSAEVENT	m_Event;
};




