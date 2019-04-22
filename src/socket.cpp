#include "socket.h"
#include <stdio.h>
#include <iostream>
#include <stdlib.h>

// Debug print options
#define DP 0
//#define DPS "    "
#define DPS ""

bool g_sockets_initialized = false;

void sockets_init()
{
#ifdef _WIN32
	WSADATA WsaData;
	if(WSAStartup( MAKEWORD(2,2), &WsaData ) != NO_ERROR)
		throw SocketException("WSAStartup failed");
#else
#endif
	g_sockets_initialized = true;
}

void sockets_cleanup()
{
#ifdef _WIN32
	WSACleanup();
#endif
}

Address::Address()
{
}

Address::Address(unsigned int address, unsigned short port)
{
	m_address = address;
	m_port = port;
}

Address::Address(unsigned int a, unsigned int b,
		unsigned int c, unsigned int d,
		unsigned short port)
{
	m_address = (a<<24) | (b<<16) | ( c<<8) | d;
	m_port = port;
}

bool Address::operator==(Address &address)
{
	return (m_address == address.m_address
			&& m_port == address.m_port);
}

void Address::Resolve(const char *name)
{
	struct addrinfo *resolved;
	int e = getaddrinfo(name, NULL, NULL, &resolved);
	if(e != 0)
		throw ResolveError("");
	/*
		FIXME: This is an ugly hack; change the whole class
		to store the address as sockaddr
	*/
	struct sockaddr_in *t = (struct sockaddr_in*)resolved->ai_addr;
	m_address = ntohl(t->sin_addr.s_addr);
	freeaddrinfo(resolved);
}

unsigned int Address::getAddress() const
{
	return m_address;
}

unsigned short Address::getPort() const
{
	return m_port;
}

void Address::setAddress(unsigned int address)
{
	m_address = address;
}

void Address::setPort(unsigned short port)
{
	m_port = port;
}

void Address::print() const
{
	std::cout<<((m_address>>24)&0xff)<<"."
			<<((m_address>>16)&0xff)<<"."
			<<((m_address>>8)&0xff)<<"."
			<<((m_address>>0)&0xff)<<":"
			<<m_port;
}

UDPSocket::UDPSocket()
{
	if(g_sockets_initialized == false)
		throw SocketException("Sockets not initialized");
	
    m_handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	if(DP)
	std::cout<<DPS<<"UDPSocket("<<(int)m_handle<<")::UDPSocket()"<<std::endl;
	
    if(m_handle <= 0)
    {
		throw SocketException("Failed to create socket");
    }

/*#ifdef _WIN32
	DWORD nonblocking = 0;
	if(ioctlsocket(m_handle, FIONBIO, &nonblocking) != 0)
	{
		throw SocketException("Failed set non-blocking mode");
	}
#else
	int nonblocking = 0;
	if(fcntl(m_handle, F_SETFL, O_NONBLOCK, nonblocking) == -1)
	{
		throw SocketException("Failed set non-blocking mode");
	}
#endif*/

	setTimeoutMs(0);
}

UDPSocket::~UDPSocket()
{
	if(DP)
	std::cout<<DPS<<"UDPSocket("<<(int)m_handle<<")::~UDPSocket()"<<std::endl;

#ifdef _WIN32
	closesocket(m_handle);
#else
	close(m_handle);
#endif
}

void UDPSocket::Bind(unsigned short port)
{
	if(DP)
	std::cout<<DPS<<"UDPSocket("<<(int)m_handle
			<<")::Bind(): port="<<port<<std::endl;

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if(bind(m_handle, (const sockaddr*)&address, sizeof(sockaddr_in)) < 0)
    {
		throw SocketException("Failed to bind socket");
    }
}

void UDPSocket::Send(const Address & destination, const void * data, int size)
{
	bool dumping_packet = false;
	if(INTERNET_SIMULATOR)
		//dumping_packet = (rand()%10==0); //easy
		dumping_packet = (rand()%4==0); // hard

	if(DP){
		/*std::cout<<DPS<<"UDPSocket("<<(int)m_handle
				<<")::Send(): destination=";*/
		std::cout<<DPS;
		std::cout<<(int)m_handle<<" -> ";
		destination.print();
		std::cout<<", size="<<size<<", data=";
		for(int i=0; i<size && i<20; i++){
			if(i%2==0) printf(" ");
			printf("%.2X", ((int)((const char*)data)[i])&0xff);
		}
		if(size>20)
			std::cout<<"...";
		if(dumping_packet)
			std::cout<<" (DUMPED BY INTERNET_SIMULATOR)";
		std::cout<<std::endl;
	}
	else if(dumping_packet)
	{
		// Lol let's forget it
		std::cout<<"UDPSocket::Send(): "
				"INTERNET_SIMULATOR: dumping packet."
				<<std::endl;
	}

	if(dumping_packet)
		return;

	sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(destination.getAddress());
	address.sin_port = htons(destination.getPort());

	int sent = sendto(m_handle, (const char*)data, size,
		0, (sockaddr*)&address, sizeof(sockaddr_in));

    if(sent != size)
    {
		throw SendFailedException("Failed to send packet");
    }
}

int UDPSocket::Receive(Address & sender, void * data, int size)
{
	if(WaitData(m_timeout_ms) == false)
	{
		return -1;
	}

	sockaddr_in address;
	socklen_t address_len = sizeof(address);

	int received = recvfrom(m_handle, (char*)data,
			size, 0, (sockaddr*)&address, &address_len);

	if(received < 0)
		return -1;

	unsigned int address_ip = ntohl(address.sin_addr.s_addr);
	unsigned int address_port = ntohs(address.sin_port);

	sender = Address(address_ip, address_port);

	if(DP){
		//std::cout<<DPS<<"UDPSocket("<<(int)m_handle<<")::Receive(): sender=";
		std::cout<<DPS<<(int)m_handle<<" <- ";
		sender.print();
		//std::cout<<", received="<<received<<std::endl;
		std::cout<<", size="<<received<<", data=";
		for(int i=0; i<received && i<20; i++){
			if(i%2==0) printf(" ");
			printf("%.2X", ((int)((const char*)data)[i])&0xff);
		}
		if(received>20)
			std::cout<<"...";
		std::cout<<std::endl;
	}

	return received;
}

int UDPSocket::GetHandle()
{
	return m_handle;
}

void UDPSocket::setTimeoutMs(int timeout_ms)
{
	m_timeout_ms = timeout_ms;
}

bool UDPSocket::WaitData(int timeout_ms)
{
	fd_set readset;
	int result;

	// Initialize the set
	FD_ZERO(&readset);
	FD_SET(m_handle, &readset);

	// Initialize time out struct
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = timeout_ms * 1000;
	// select()
	result = select(m_handle+1, &readset, NULL, NULL, &tv);

	if(result == 0){
		// Timeout
		/*std::cout<<"Select timed out (timeout_ms="
				<<timeout_ms<<")"<<std::endl;*/
		return false;
	}
	else if(result < 0){
		// Error
		throw SocketException("Select failed");
	}
	else if(FD_ISSET(m_handle, &readset) == false){
		// No data
		//std::cout<<"Select reported no data in m_handle"<<std::endl;
		return false;
	}
	
	// There is data
	//std::cout<<"Select reported data in m_handle"<<std::endl;
	return true;
}


