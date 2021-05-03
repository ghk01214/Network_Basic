#include <iostream>
#include <string>
#include <chrono>
#include <unordered_map>
#include "conn.h"

using namespace std;

volatile bool g_end_node = false;
volatile int g_send_state = 0;			// 0 : nothing to do,   1: node sent cmd to NIC,  2: NIC sent reply to node
volatile int NIC_STATE_IDLE = 0;
volatile int NIC_STATE_HAVE_CMD = 1;
volatile int NIC_STATE_DONE = 2;

bool active{ false };
string sendMessage;
const chrono::microseconds CLOCK{ 100000 };
chrono::high_resolution_clock::time_point next_start_time;

void send_message(string sMessage)
{
	while (g_send_state != 0);

	sendMessage = sMessage;

	g_send_state = 1;

	while (g_send_state != 2);

	g_send_state = 0;
}

int get_signal(chrono::high_resolution_clock::time_point& tp, CONN& g_conn, int num)
{
	int iBit = 0;

	for (int i = 0; i < num; ++i)
	{
		iBit = iBit << 1;

		if (g_conn.get())
			++iBit;

		while (chrono::high_resolution_clock::now() < tp + CLOCK * (i + 1));
	}

	tp += CLOCK * num;

	return iBit;
}

void do_node(char node_id)
{
	string message;

	cout << "Hello World, I am node " << node_id << "." << endl << endl;

	while (true)
	{
		// 전송할 노드와 메세지를 입력
		cout << "Enter destination node with a message to send : ";
		getline(cin, message);

		if (message == "q")
		{
			g_end_node = true;

			break;
		}

		cout << "You entered [" << message << "]" << endl << endl;
		send_message(message);
	}

	cout << "End Program" << endl;
}


bool set_signal(chrono::high_resolution_clock::time_point& tp, CONN& g_conn, int num, unsigned int iBit)
{
	bool bCollision = false;

	for (int i = 0; i < num; ++i)
	{
		bool bBit = (iBit & (1 << (num - i - 1))) != 0;

		g_conn.set(iBit);

		while (chrono::high_resolution_clock::now() < tp + CLOCK)
		{
			if (g_conn.get() != iBit)
				bCollision = true;
		}

		tp += CLOCK;
	}

	return bCollision;
}

bool transmit_message(CONN& g_conn, int mac_addr)
{
	auto start_time = chrono::high_resolution_clock::now();
	bool bCollision = set_signal(start_time, g_conn, 1, 1);

	if (bCollision)
	{
		set_signal(start_time, g_conn, 1, 1);
		set_signal(start_time, g_conn, 1, 0);

		next_start_time = start_time + CLOCK * (1 + (rand() % 10));
	}

	bCollision = set_signal(start_time, g_conn, 4, mac_addr);

	if (bCollision)
	{
		set_signal(start_time, g_conn, 1, 1);
		set_signal(start_time, g_conn, 1, 0);

		next_start_time = start_time + CLOCK * (1 + (rand() % 10));

		return false;
	}

	bCollision = set_signal(start_time, g_conn, 1, 0);

	if (bCollision)
	{
		set_signal(start_time, g_conn, 1, 1);
		set_signal(start_time, g_conn, 1, 0);

		next_start_time = start_time + CLOCK * (1 + (rand() % 10));

		return false;
	}

	int iDestination = 1 << (sendMessage[0] - 'A');

	set_signal(start_time, g_conn, 4, iDestination);
	set_signal(start_time, g_conn, 8, sendMessage.length());

	for (int i = 1; i < sendMessage.length(); ++i)
	{
		set_signal(start_time, g_conn, 8, sendMessage[i]);
	}

	//for (auto i = sendMessage.begin() + 1; i != sendMessage.end(); ++i)
	//{
	//	set_signal(start_time, g_conn, 8, *i);
	//}

	set_signal(start_time, g_conn, 1, 0);
	g_send_state = 2;

	return true;
}

void do_node_NIC(char node_id, CONN& g_conn)
{
	next_start_time = chrono::high_resolution_clock::now();
	int mac_addr = 0;

	switch (node_id)
	{
	case 'A':
	{
		mac_addr = 1;
		
		break;
	}
	case 'B':
	{
		mac_addr = 2;

		break;
	}
	case 'C':
	{
		mac_addr = 4;

		break;
	}
	case 'D':
	{
		mac_addr = 8;

		break;
	}
	}

	unordered_map<int, int> mac2node = { {1, 'A'}, {2, 'B'}, {4, 'C'}, {8, 'D'} };

	do
	{
		while (!g_conn.get())
		{
			if ((g_send_state == 1) && (chrono::high_resolution_clock::now() > next_start_time))
			{
				transmit_message(g_conn, mac_addr);
				
				continue;
			}
		}

		auto start_time = chrono::high_resolution_clock::now();

		while (chrono::high_resolution_clock::now() < start_time + CLOCK + CLOCK / 2);

		start_time += (CLOCK + CLOCK / 2);

		char cSource = get_signal(start_time, g_conn, 4);
		int iCd = get_signal(start_time, g_conn, 1);

		if (iCd == 1)
		{
			while (chrono::high_resolution_clock::now() < start_time + CLOCK * 2);

			continue;
		}

		int iDestination = get_signal(start_time, g_conn, 4);
		int iLength = get_signal(start_time, g_conn, 8);

		string sMessage;

		for (int i = 0; i < iLength; ++i)
		{
			sMessage[i] = get_signal(start_time, g_conn, 8);
		}

		sMessage[iLength] = 0;

		if (iDestination == mac_addr)
		{
			char sentNode = mac2node[cSource];

			cout << "Node " << sentNode << " sent [" << sMessage << "]" << endl;
			cout << "Enter destination node with a message to send : ";
		}
	} while (!g_end_node);
}