#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <queue>
#include "conn.h"

constexpr int MAX_NODES = 16;
extern const unsigned int NUM_NODES;

using namespace std;

constexpr int NIC_STATE_IDLE = 0;
constexpr int NIC_STATE_HAVE_CMD = 1;
constexpr int NIC_STATE_DONE = 2;

struct FRAME
{
	char source;
	char destination;
	string message;
};

struct BUFFER
{
	volatile int state;
	FRAME buffer;
	queue<FRAME> messageQueue;
};

struct NIC {						// g_conn을 관리하기 위한 관리 객체
	string NICName;
	BUFFER toNIC;
	BUFFER fromNIC;
	chrono::high_resolution_clock::time_point nextStartTime;
	int padding[64];				// 멀티코어 프로그래밍 시간에 다룰 내용, 성능 향상을 위해서 존재.
};

string message[4]{};
const chrono::microseconds CLOCK{ 100000 };

volatile bool g_end_hub = false;
NIC g_nic[4];
queue<FRAME> g_msg_queue;

char mac2node(int macAddress)
{
	switch (macAddress)
	{
	case 1:
		return 'A';
	case 2:
		return 'B';
	case 4:
		return 'C';
	case 8:
		return 'D';
	default:
		return 'X';
	}
}

int mac2port(int macAddress)
{
	switch (macAddress)
	{
	case 1:
		return 0;
	case 2:
		return 1;
	case 4:
		return 2;
	case 8:
		return 3;
	}
}

void send_message(NIC& nic, FRAME& message)
{
	while (nic.toNIC.state != NIC_STATE_IDLE);

	nic.toNIC.buffer = message;
	nic.toNIC.state = NIC_STATE_HAVE_CMD;

	while (nic.toNIC.state != NIC_STATE_DONE);

	nic.toNIC.state = NIC_STATE_IDLE;
}

void do_hub()
{
	cout << "Hello Wolrd. I am the HUB." << endl;

	while (true)
	{
		for (unsigned int i = 0; i < NUM_NODES; ++i)
		{
			if (g_nic[i].fromNIC.state == NIC_STATE_HAVE_CMD)
			{
				g_nic[i].fromNIC.messageQueue.push(g_nic[i].fromNIC.buffer);
				g_nic[i].fromNIC.state = NIC_STATE_DONE;
			}
		}

		for (unsigned int i = 0; i < NUM_NODES; ++i)
		{
			if (!g_nic[i].fromNIC.messageQueue.empty())
			{
				FRAME fr = g_nic[i].fromNIC.messageQueue.front();

				g_nic[i].fromNIC.messageQueue.pop();

				int destinationAddress = fr.destination;
				int destinationNumber = mac2node(destinationAddress);

				if (destinationNumber == -1)
				{
					char cPort = 'A' + i;

					cout << "Error Frame from Port " << cPort << endl;

					continue;
				}

				send_message(g_nic[destinationNumber], fr);

				char cSource = 'A' + i;
				char cDestination = mac2node(fr.destination);

				cout << "Send Message from Port " << cSource << " to Port " << cDestination << " : " << fr.message;
			}
		}

		this_thread::yield();
	}

	cout << "End Program" << endl;
}

bool set_signal(chrono::high_resolution_clock::time_point& tp, CONN& g_conn, int num, unsigned int uiValue);
int get_signal(chrono::high_resolution_clock::time_point& tp, CONN& g_conn, int num);

bool transmit_message(CONN& g_conn, NIC& nic)
{
	FRAME fr = nic.toNIC.messageQueue.front();
	auto startTime = chrono::high_resolution_clock::now();
	bool bCollision = set_signal(startTime, g_conn, 1, 1);

	if (bCollision)
	{
		set_signal(startTime, g_conn, 1, 1);
		set_signal(startTime, g_conn, 1, 0);

		nic.nextStartTime = startTime + CLOCK * (1 + (rand() % 10));

		return false;
	}

	bCollision = set_signal(startTime, g_conn, 1, 0);

	if (bCollision)
	{
		set_signal(startTime, g_conn, 1, 1);
		set_signal(startTime, g_conn, 1, 0);

		nic.nextStartTime = startTime + CLOCK * (1 + (rand() % 10));

		return false;
	}

	set_signal(startTime, g_conn, 4, fr.destination);
	set_signal(startTime, g_conn, 8, fr.message.length());

	for (int i = 0; i < fr.message.length(); ++i)
	{
		set_signal(startTime, g_conn, 8, fr.message[i]);
	}

	//for (auto i = fr.message.begin() + 1; i != fr.message.end(); ++i)
	//{
	//	set_signal(start_time, g_conn, 8, *i);
	//}

	set_signal(startTime, g_conn, 1, 0);
	nic.toNIC.messageQueue.pop();

	return true;
}

void do_hub_NIC(int portId, CONN& g_conn)
{
	NIC& nic = g_nic[portId];
	nic.nextStartTime = chrono::high_resolution_clock::now();

	do
	{
		while (!g_conn.get())
		{
			if (nic.toNIC.state == NIC_STATE_HAVE_CMD)
			{
				nic.toNIC.messageQueue.push(nic.toNIC.buffer);
				nic.toNIC.state = NIC_STATE_DONE;
			}

			if (!nic.toNIC.messageQueue.empty() && (chrono::high_resolution_clock::now() > nic.nextStartTime))
			{
				transmit_message(g_conn, nic);

				continue;
			}
		}

		auto startTime = chrono::high_resolution_clock::now();

		while (chrono::high_resolution_clock::now() < startTime + CLOCK + CLOCK / 2);

		startTime += (CLOCK + CLOCK / 2);
		char cSource = get_signal(startTime, g_conn, 4);
		int iCd = get_signal(startTime, g_conn, 1);

		if (iCd == 1)
		{
			while (chrono::high_resolution_clock::now() < startTime + CLOCK * 2);
			continue;
		}

		int iDestination = get_signal(startTime, g_conn, 4);
		int iLength = get_signal(startTime, g_conn, 8);
		char sMessage[200];

		for (int i = 0; i < iLength; ++i)
		{
			sMessage[i] = get_signal(startTime, g_conn, 8);
		}

		sMessage[iLength] = 0;

		char sentNode = mac2node(cSource);
		char destinationNode = mac2node(iDestination);

		cout << "NODE " << sentNode << " sent [" << sMessage << "] to Node " << destinationNode << endl;

		FRAME fr;

		fr.source = cSource;
		fr.destination = iDestination;
		fr.message = sMessage;

		while (nic.fromNIC.state != NIC_STATE_IDLE);

		nic.fromNIC.buffer = fr;
		nic.fromNIC.state = NIC_STATE_HAVE_CMD;

		while (nic.fromNIC.state != NIC_STATE_DONE);

		nic.fromNIC.state = NIC_STATE_IDLE;
	} while (!g_end_hub);
}

void do_hub_NIC0(CONN& g_conn)
{
	do_hub_NIC(0, g_conn);
}

void do_hub_NIC1(CONN& g_conn)
{
	do_hub_NIC(1, g_conn);
}

void do_hub_NIC2(CONN& g_conn)
{
	do_hub_NIC(2, g_conn);
}

void do_hub_NIC3(CONN& g_conn)
{
	do_hub_NIC(3, g_conn);
}