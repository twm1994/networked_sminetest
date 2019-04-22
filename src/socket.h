#ifndef SOCKET_HEADER
#define SOCKET_HEADER

#ifdef _WIN32
    #ifndef _WIN32_WINNT
        #define _WIN32_WINNT 0x0501
    #endif
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
//  #include <winsock2.h>
    #include <ws2tcpip.h>
//  #pragma comment (lib, "Ws2_32.lib")
typedef SOCKET socket_t;
typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <fcntl.h>
    #include <netdb.h>
    #include <unistd.h>
typedef int socket_t;
#endif

#include "exceptions.h"

// Define for simulating the quirks of sending through internet
// WARNING: This disables unit testing of socket and connection
#define INTERNET_SIMULATOR 0

class SocketException : public BaseException
{
public:
	SocketException(const char *s):
		BaseException(s)
	{
	}
};

class ResolveError : public BaseException
{
public:
	ResolveError(const char *s):
		BaseException(s)
	{
	}
};

class SendFailedException : public BaseException
{
public:
	SendFailedException(const char *s):
		BaseException(s)
	{
	}
};

void sockets_init();
void sockets_cleanup();

class Address
{
public:
	Address();
	Address(unsigned int address, unsigned short port);
	Address(unsigned int a, unsigned int b,
			unsigned int c, unsigned int d,
			unsigned short port);
	bool operator==(Address &address);
	void Resolve(const char *name);
	unsigned int getAddress() const;
	unsigned short getPort() const;
	void setAddress(unsigned int address);
	void setPort(unsigned short port);
	void print() const;
private:
	unsigned int m_address;
	unsigned short m_port;
};

class UDPSocket
{
public:
	UDPSocket();
	~UDPSocket();
	void Bind(unsigned short port);
	//void Close();
	//bool IsOpen();
	void Send(const Address & destination, const void * data, int size);
	// Returns -1 if there is no data
	int Receive(Address & sender, void * data, int size);
	int GetHandle(); // For debugging purposes only
	void setTimeoutMs(int timeout_ms);
	// Returns true if there is data, false if timeout occurred
	bool WaitData(int timeout_ms);
private:
	int m_handle;
	int m_timeout_ms;
};

#endif

