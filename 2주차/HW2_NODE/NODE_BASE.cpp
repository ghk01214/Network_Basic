#include <iostream>
#include <map>
#include <thread>
using namespace std;
#include <WS2tcpip.h>
#include <MSWSock.h>
#include "conn.h"
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")

void do_node(char node_type);

#define MAX_BUFFER        1024
#define SERVER_PORT       3500

enum OPERATION { OP_RECV, OP_SEND, OP_ACCEPT };

struct OVER_EX {
	WSAOVERLAPPED over;
	WSABUF	wsabuf[1];
	char	net_buf[MAX_BUFFER];
	OPERATION	op;
	SOCKET  accept_socket;
};

struct SOCKETINFO
{
	OVER_EX	recv_over;
	SOCKET	socket;
	int		id;
};

map <SOCKET, SOCKETINFO> clients;
HANDLE	g_iocp;
CONN g_conn;

void send_conn(SOCKET socket, const char* buf, int num_byte)
{
	OVER_EX* send_over = new OVER_EX;
	memset(send_over, 0x00, sizeof(OVER_EX));
	send_over->op = OP_SEND;
	memcpy(send_over->net_buf, buf, num_byte);
	send_over->wsabuf[0].buf = send_over->net_buf;
	send_over->wsabuf[0].len = num_byte;
	WSASend(socket, send_over->wsabuf, 1, 0, 0, &send_over->over, 0);
}

void broadcast_conn(const char* buf, int num_byte)
{
	for (auto& cl : clients) 
		send_conn(cl.first, buf, num_byte);
}

void send_value(bool value)
{
	char data;
	if (true == value) data = 1;
	else data = 0;
	broadcast_conn(&data, 1);
}

CONN::CONN() : stat(false)
{
}

void CONN::set(bool value)
{
	stat = value;
	SleepEx(0, TRUE);
	send_value(value);
}

void CONN::set_value(bool value)
{
	stat = value;
}

bool CONN::get()
{
	SleepEx(0, TRUE);
	return stat;
}

int main()
{
	std::wcout.imbue(std::locale("korean"));
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);
	g_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);

	char node_type;

	for (;;) {
		cout << "Enter NODE ID (A, B, C, D) : ";
		cin >> node_type;
		node_type = toupper(node_type);
		if ((node_type >= 'A') && (node_type <= 'D')) break;
		cout << "Wrong Node Type!\n\n";
	}

	SOCKET listenSocket = 0;
	SOCKET clientSocket1 = 0;
	SOCKET serverSocket = 0;
	OVER_EX* accept_over1 = new OVER_EX;

	if ('A' == node_type) {
		listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		SOCKADDR_IN serverAddr;
		memset(&serverAddr, 0, sizeof(SOCKADDR_IN));
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(SERVER_PORT);
		serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
		::bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(SOCKADDR_IN));
		listen(listenSocket, 5);
		SOCKADDR_IN clientAddr;
		int addrLen = sizeof(SOCKADDR_IN);
		memset(&clientAddr, 0, addrLen);
		CreateIoCompletionPort(reinterpret_cast<HANDLE>(listenSocket), g_iocp, listenSocket, 0);
		clientSocket1 = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		memset(accept_over1, 0, sizeof(OVER_EX));
		accept_over1->op = OP_ACCEPT;

		AcceptEx(listenSocket, clientSocket1, accept_over1->net_buf, NULL, sizeof(SOCKADDR_IN) + 16,
			sizeof(SOCKADDR_IN) + 16, NULL, &accept_over1->over);
		CreateIoCompletionPort(reinterpret_cast<HANDLE>(clientSocket1), g_iocp, clientSocket1, 0);
	}
	else {
		wcout << L"노드 " << node_type << L"를 시작합니다.\n";
		wcout << L"노드 A와 연결을 시도합니다.\n";

		serverSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

		struct sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(SERVER_PORT);
		inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);


		while (true) {
			int res = connect(serverSocket, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
			if (0 != res) {
				wcout << L"노드 A의 응답이 없습니다. 재시도 합니다." << endl;
			}
			else break;
		}

		CreateIoCompletionPort(reinterpret_cast<HANDLE>(serverSocket), g_iocp, serverSocket, 0);
		wcout << L"노드 A에 연결되었습니다.\n";

		clients[serverSocket] = SOCKETINFO{};
		memset(&clients[serverSocket], 0, sizeof(struct SOCKETINFO));
		clients[serverSocket].socket = serverSocket;
		clients[serverSocket].recv_over.wsabuf[0].len = MAX_BUFFER;
		clients[serverSocket].recv_over.wsabuf[0].buf = clients[serverSocket].recv_over.net_buf;
		clients[serverSocket].recv_over.op = OP_RECV;
		DWORD flags = 0;
		WSARecv(serverSocket, clients[serverSocket].recv_over.wsabuf, 1, NULL,
			&flags, &(clients[serverSocket].recv_over.over), NULL);

	}

	thread node_thread{ do_node, node_type };

	while (true) {
		DWORD num_byte;
		ULONG key;
		PULONG p_key = &key;
		WSAOVERLAPPED* p_over;

		GetQueuedCompletionStatus(g_iocp, &num_byte, p_key, &p_over, INFINITE);

		OVER_EX* over_ex = reinterpret_cast<OVER_EX*> (p_over);

		switch (over_ex->op) {
		case OP_ACCEPT: {
			SOCKET clientSocket = clientSocket1;

			clients[clientSocket] = SOCKETINFO{};
			memset(&clients[clientSocket], 0, sizeof(struct SOCKETINFO));
			clients[clientSocket].socket = clientSocket;
			clients[clientSocket].recv_over.wsabuf[0].len = MAX_BUFFER;
			clients[clientSocket].recv_over.wsabuf[0].buf = clients[clientSocket].recv_over.net_buf;
			clients[clientSocket].recv_over.op = OP_RECV;
			DWORD flags = 0;
			WSARecv(clientSocket, clients[clientSocket].recv_over.wsabuf, 1, NULL,
				&flags, &(clients[clientSocket].recv_over.over), NULL);
			bool value = g_conn.get();
			char data;
			if (true == value) data = 1;
			else data = 0;
			send_conn(clientSocket, &data, 1);

			clientSocket1 = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			memset(&accept_over1->over, 0, sizeof(WSAOVERLAPPED));
			AcceptEx(listenSocket, clientSocket1, accept_over1->net_buf, NULL, sizeof(SOCKADDR_IN) + 16,
				sizeof(SOCKADDR_IN) + 16, NULL, &accept_over1->over);
			CreateIoCompletionPort(reinterpret_cast<HANDLE>(clientSocket1), g_iocp, clientSocket1, 0);
		}
					  break;
		case OP_RECV: {
			SOCKET client_s = static_cast<SOCKET>(key);

			if (num_byte == 0) {
				closesocket(client_s);
				clients.erase(client_s);
				if ('A' != node_type) {
					cout << "\n\nConnection to the NODE A is closed. Exiting\n";
					exit(-1);
				}
				continue;
			}  // 클라이언트가 closesocket을 했을 경우

			for (DWORD i = 0; i < num_byte; ++i)
				g_conn.set_value(over_ex->net_buf[i] == 1);

			if ('A' == node_type)
				broadcast_conn(over_ex->wsabuf->buf, num_byte);

			DWORD flags = 0;
			memset(&over_ex->over, 0x00, sizeof(WSAOVERLAPPED));
			WSARecv(client_s, over_ex->wsabuf, 1, 0, &flags, &over_ex->over, 0);
		}
					break;
		case OP_SEND: {
			SOCKET client_s = static_cast<SOCKET>(key);

			if (num_byte == 0) {
				closesocket(client_s);
				clients.erase(client_s);
				delete p_over;
				if ('A' != node_type) {
					cout << "\n\nConnection to the NODE A is closed. Exiting\n";
					exit(-1);
				}
				continue;
			}  // 클라이언트가 closesocket을 했을 경우
			OVER_EX* over_ex = reinterpret_cast<OVER_EX*> (p_over);
			// cout << "Sent to Client[" << client_s << "] " << over_ex->net_buf << " (" << num_byte << " bytes)\n";
			delete p_over;
		}
					break;
		}
	}
	node_thread.join();
	closesocket(listenSocket);
	WSACleanup();
}