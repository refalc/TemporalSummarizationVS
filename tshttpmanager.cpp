#include "tshttpmanager.h"
#include <iostream>
#include <regex>
#include <array>
#include "TemporalSummarization\utils.h"

TSHttpManager::WSAStateHolder::WSAStateHolder() : m_bInited(false)
{

}

TSHttpManager::WSAStateHolder::~WSAStateHolder()
{
	Release();
}

TSHttpManager::CSimpleSocket::CSimpleSocket() :
	m_pWSAStateHolder(nullptr),
	m_bConnected(false),
	m_bIsCreated(false)
{

}

TSHttpManager::CSimpleSocket::CSimpleSocket(WSAStateHolder *wsa_state_holder) :
    m_pWSAStateHolder(wsa_state_holder),
    m_bConnected(false),
    m_bIsCreated(false)
{
}

TSHttpManager::CSimpleSocket::~CSimpleSocket()
{
    if( m_bIsCreated )
        closesocket(m_iSocketID);
}

void TSHttpManager::CSimpleSocket::InitWSA(WSAStateHolder *wsa_state_holder)
{
	m_pWSAStateHolder = wsa_state_holder;
}

bool TSHttpManager::CSimpleSocket::CreateSocket(int af, int type, int protocol)
{
	if( !m_pWSAStateHolder || m_bIsCreated )
		return false;

    if( (m_iSocketID = (int)socket(af, type, protocol)) < 0 ) {
        m_pWSAStateHolder->SetLastMessage("socket() failed : ");
        return false;
    }

    m_bIsCreated = true;
    return true;
}

bool TSHttpManager::CSimpleSocket::Connect(sockaddr_in *server_addr)
{
	m_pLastServerAddr = server_addr;
    if( !m_bIsCreated || !m_pWSAStateHolder || m_bConnected )
        return false;

    if( connect(m_iSocketID, (sockaddr *)server_addr, sizeof(*server_addr)) < 0 ) {
        m_pWSAStateHolder->SetLastMessage("connect() failed : ");
        m_bConnected = false;
        return false;
    }

    m_bConnected = true;
    return true;
}

bool TSHttpManager::CSimpleSocket::CloseSocket()
{
	if( !m_bIsCreated )
		return false;

	if( closesocket(m_iSocketID) != 0 ) {
		m_pWSAStateHolder->SetLastMessage("closesocket failed : ");
		return false;
	}

	m_bIsCreated = false;
	m_bConnected = false;

	return true;

}
bool TSHttpManager::CSimpleSocket::Send(const std::string &request) const
{
	if( !m_pWSAStateHolder )
		return false;

    if( send((SOCKET)m_iSocketID, request.c_str(), request.length(), 0) != request.length() ) {
        if( !m_bConnected )
            m_pWSAStateHolder->SetLastMessage("Socket does not connected : ");
        else
            m_pWSAStateHolder->SetLastMessage("Error in Send : ");

        return false;
    }

    return true;
}

bool TSHttpManager::CSimpleSocket::Recv(std::string &reply, int max_wait_loop, int sleep_duration)
{
	constexpr int max_buffer_size = 1024 * 8;
    std::array<char, max_buffer_size> buffer;
	std::regex re_content_len("Content-Length: (.*?)[\r\n]");
	int recv_size = 0, count = -1, content_size = -1;
	bool find_header = false;
	std::string get_reply;

	while( ++count < max_wait_loop && (!find_header || get_reply.size() < content_size) ) {
		if (count > 0)
			Sleep(sleep_duration);

		while( (recv_size = recv(m_iSocketID, (char*)buffer.data(), max_buffer_size, 0)) > 0 ) {
			if( !find_header ) {
				std::string cur_recieved(buffer.begin(), buffer.begin() + recv_size);
				std::smatch content_match;
				if( std::regex_search(cur_recieved, content_match, re_content_len) ) {
					content_size = std::stoi(content_match[1]);
					get_reply.reserve(content_size + 300);
					find_header = true;
				}
			}

			get_reply.append(buffer.begin(), buffer.begin() + recv_size);
		}

		if( recv_size == -1 ) {
			m_pWSAStateHolder->SetLastMessage("Error when recv : ");
			return false;
		}
	}

	if( !find_header )
		return false;

	if( get_reply.size() < content_size )
		return false;

	if (get_reply.size() > content_size)
		get_reply = get_reply.substr(get_reply.size() - content_size, content_size);

	reply = std::move(get_reply);
	return true;
}

bool TSHttpManager::WSAStateHolder::Init()
{
    if( WSAStartup(MAKEWORD(2, 0), &m_WSAdata) != 0 ) {
        m_sLastMessage = "WSAStartup() failed : ";
        return false;
    }

    m_bInited = true;
    return true;
}

void TSHttpManager::WSAStateHolder::Release()
{
	if( !m_sLastMessage.empty() ) {
		std::cout << m_sLastMessage << std::endl;
		m_sLastMessage.clear();
	}

	if( m_bInited )
		WSACleanup();

	m_bInited = false;
}

void TSHttpManager::WSAStateHolder::SetLastMessage(const std::string &msg)
{
    m_sLastMessage = msg + std::to_string(WSAGetLastError());
}

TSHttpManager::TSHttpManager() 
{

}

bool TSHttpManager::Init(const std::string &ip, int port)
{
    memset(&m_ServerAddr, 0, sizeof(m_ServerAddr));
    m_ServerAddr.sin_family      = AF_INET;
    m_ServerAddr.sin_addr.s_addr = inet_addr(ip.c_str());
    m_ServerAddr.sin_port        = htons((unsigned short) port);

    m_sHostName = ip;

	if( !m_WSAStateHolder.Init() )
		return false;

	m_Socket.InitWSA(&m_WSAStateHolder);

    return true;
}

bool TSHttpManager::Close()
{
	if( !m_Socket.CloseSocket() )
		return false;

	m_WSAStateHolder.Release();
	return true;
}

std::string TSHttpManager::CreateGetRequest(const std::string &request) const
{
	return "GET /" + request + " HTTP/1.0\r\n" + "Host: " + m_sHostName + "\r\n\r\n";
}

bool TSHttpManager::Get(const std::string &request, std::string &reply)
{
	if( !m_Socket.CreateSocket(PF_INET, SOCK_STREAM, IPPROTO_TCP) )
		return false;

	if( !m_Socket.Connect(&m_ServerAddr) )
		return false;

	if( !m_Socket.Send(CreateGetRequest(request)) )
		return false;

	if( !m_Socket.Recv(reply) )
		return false;

	if( !m_Socket.CloseSocket() )
		return false;

	return true;
}
