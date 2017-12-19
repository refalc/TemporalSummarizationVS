#pragma once
#include <string>
#include <winsock.h>
#include <memory>

class TSHttpManager
{
public:
    TSHttpManager();
    bool Init(const std::string &ip, int port);
	bool Close();
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
		void Release();
        void SetLastMessage(const std::string &msg);

    private:
		WSADATA m_WSAdata;
        bool m_bInited;
        std::string m_sLastMessage;
    };

    class CSimpleSocket
    {
    public:
		CSimpleSocket();
        CSimpleSocket(WSAStateHolder *wsa_state_holder);
        ~CSimpleSocket();

		void InitWSA(WSAStateHolder *wsa_state_holder);
        bool CreateSocket(int af, int type, int protocol);
        bool Connect(sockaddr_in *server_addr);
		bool CloseSocket();
        bool Send(const std::string &request) const;
        bool Recv(std::string &reply, int max_wait_loop = 5, int sleep_duration = 100);

    private:
        WSAStateHolder *m_pWSAStateHolder;
        int m_iSocketID;
        bool m_bIsCreated;
        bool m_bConnected;
		sockaddr_in *m_pLastServerAddr;
    };

private:
	WSAStateHolder m_WSAStateHolder;
	CSimpleSocket m_Socket;
    sockaddr_in m_ServerAddr;
    std::string m_sHostName;

	//std::unique_ptr<happyhttp::Connection> m_spConnection;

};
