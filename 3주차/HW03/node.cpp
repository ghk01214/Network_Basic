#include <iostream>
#include <string>
#include <chrono>
#include <queue>
#include "conn.h"

using namespace std;

volatile int g_send_state = 0;			// 0 : nothing to do,   1: node sent cmd to NIC,  2: NIC sent reply to node
constexpr int NIC_STATE_IDLE = 0;
constexpr int NIC_STATE_HAVE_CMD = 1;
constexpr int NIC_STATE_DONE = 2;

volatile bool g_value_from_NIC;

volatile int g_cmd_to_NIC;
constexpr int CMD_SEND_MESSAGE = 0;
constexpr int CMD_RECIEVE_MESSAGE = 1;

string sendStr{};
string recieveStr{};
bool work{ false };
const chrono::microseconds CLOCK{ 10000 };

void send_command_to_NIC()
{
	while (work);
	if (!work)
	{
		g_send_state = NIC_STATE_HAVE_CMD;
	}

	while (g_send_state != NIC_STATE_DONE);

	g_send_state = NIC_STATE_IDLE;
}

void do_node(char node_id)
{
	cout << "Hello World, I am node " << node_id << "." << endl << endl;

	while (true)
	{
		// 전송할 노드와 메세지를 입력
		cout << "Enter destination node with a message to send : ";
		getline(cin, sendStr);

		send_command_to_NIC();
	}
}

void check_conn(const char* NIC_NAME, bool& my_conn, CONN& g_conn)
{
	if (my_conn != g_conn.get())
	{
		cout << "\n\n" << NIC_NAME << " changed to";
		if (true == my_conn)
			cout << " False\n";
		else
			cout << " True\n";
		my_conn = !my_conn;
		cout << "Enter CMD => ";
	}
}

bool send_message(chrono::high_resolution_clock::time_point& tp, int num, unsigned int c)
{
	bool collision{ false };

	for (int i{ 0 }; i < num; ++i)
	{
		bool bit{ (c & (1 << i)) != 0 };
		g_conn.set(bit);

		while (chrono::high_resolution_clock::now() < tp + CLOCK);

		if (g_conn.get() != bit)
			collision = true;

		tp += CLOCK;
	}

	return collision;
}

char recieve_message(chrono::high_resolution_clock::time_point& tp, int num)
{
	char c{};

	for (int i{ 0 }; i < num; ++i)
	{
		while (chrono::high_resolution_clock::now() < tp + CLOCK);

		if (chrono::high_resolution_clock::now() > tp + CLOCK)
		{
			if (g_conn.get())
				c |= 1 << i;

			tp += CLOCK;
		}
	}

	if (num != 1)
		recieveStr.push_back(c);

	return c;
}

void do_node_NIC(char node_id)
{
	bool read{ false };
	bool finish{ false };

	while (true)
	{
		auto time = chrono::high_resolution_clock::now();

		if (g_send_state == NIC_STATE_HAVE_CMD)
		{
			cout << "You entered [" << sendStr << "]" << endl << endl;

			// 수신 노드 정보 전송
			send_message(time, 1, 1);
			send_message(time, 8, sendStr[0]);
			send_message(time, 1, 0);

			// 송신 노드 정보 전송
			send_message(time, 1, 1);
			send_message(time, 8, node_id);
			send_message(time, 1, 0);

			// 메세지 전송
			for (int i{ 1 }; i < sendStr.length(); ++i)
			{
				send_message(time, 1, 1);
				send_message(time, 8, sendStr[i]);
				send_message(time, 1, 0);
			}

			// 전송 종료 알림 전송
			send_message(time, 1, 1);
			send_message(time, 8, '\0');
			send_message(time, 1, 0);

			sendStr.clear();

			g_send_state = NIC_STATE_DONE;
		}
		else if (g_send_state == NIC_STATE_IDLE)
		{
			if (!read)
			{
				read = g_conn.get();
			}
			else
			{
				work = true;
				// 메세지 수신
				recieve_message(time, 8);
				recieve_message(time, 1);

				read = false;

				if (recieveStr.back() == '\0')
				{
					if (recieveStr[0] == node_id)
					{
						finish = true;
					}
					else
					{
						recieveStr.clear();

						work = false;
						g_send_state = NIC_STATE_DONE;
					}
				}
			}

			if (finish)
			{
				cout << endl << "Node " << recieveStr[1] << " sent [";
				recieveStr.erase(0, 2);
				cout << recieveStr << "]" << endl;

				finish = false;
				recieveStr.clear();
				work = false;
				g_send_state = NIC_STATE_DONE;
				//cout << endl << "Enter destination node with a message to send : ";
			}
		}
	}
}