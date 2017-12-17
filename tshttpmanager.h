#pragma once
#include <string>
#include <winsock.h>

class TSHttpManager
{
public:
    TSHttpManager();
    bool SetServer(const std::string &ip, int port, const std::string &host_name);
    bool Get(const std::string &request, std::string &reply);

private:
	std::string CreateGetRequest(const std::string &request) const;

private:
    class WSAStateHolder
    {
    public:
        WSAStateHolder();
        ~WSAStateHolder();

        bool Init();
        void SetLastMessage(const std::string &msg);

    private:
		WSADATA m_WSAdata;
        bool m_bInited;
        std::string m_sLastMessage;
    };

    class CSimpleSocket
    {
    public:
        CSimpleSocket(WSAStateHolder *wsa_state_holder);
        ~CSimpleSocket();

        bool CreateSocket(int af, int type, int protocol);
        bool Connect(sockaddr_in *server_addr);
        bool Send(const std::string &request) const;
        bool Recv(std::string &reply, int max_wait_loop = 5, int sleep_duration = 100) const;

    private:
        WSAStateHolder *m_pWSAStateHolder;
        int m_iSocketID;
        bool m_bIsCreated;
        bool m_bConnected;
    };

private:
	WSAStateHolder * m_pWSAStateHolder;
    sockaddr_in m_ServerAddr;
    std::string m_sHostName;
};
