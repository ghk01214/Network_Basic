//#include <iostream>
//#include <chrono>
//#include <string>
//#include "conn.h"
//
//enum State
//{
//	Empty, Filled, Next
//};
//
//volatile int						sendState{ Empty };
//std::string							sendMessage;
//const std::chrono::microseconds		CLOCK{ 1000 };
//
//// num = 비트 수, value = 저장된 데이터
//bool SetSignal(std::chrono::high_resolution_clock::time_point& tp, int iNum, unsigned int uiValue)
//{
//	bool bCollision{ false };
//
//	for (int i{ 0 }; i < iNum; ++i)
//	{
//		bool bit = uiValue & true;
//		g_conn.set(bit);
//		uiValue >>= 1;
//
//		while (std::chrono::high_resolution_clock::now() < tp + CLOCK)
//		{
//			if (g_conn.get() != bit)
//			{
//				bCollision = true;
//			}
//		}
//
//		tp += CLOCK;
//	}
//
//	return bCollision;
//}
//
//char GetNodeID(std::chrono::high_resolution_clock::time_point& tp)
//{
//	char iValue{};
//
//	for (int i{ 0 }; i < 8; ++i)
//	{
//		while (std::chrono::high_resolution_clock::now() < tp + CLOCK);
//
//		if (std::chrono::high_resolution_clock::now() > tp + CLOCK)
//		{
//			iValue |= g_conn.get() << i;
//		}
//
//		tp += CLOCK;
//	}
//
//	return iValue;
//}
//
//int GetSignal(std::chrono::high_resolution_clock::time_point& tp)
//{
//	int iValue{};
//
//	for (int i{ 0 }; i < 8; ++i)
//	{
//		while (std::chrono::high_resolution_clock::now() < tp + CLOCK);
//
//		if (std::chrono::high_resolution_clock::now() > tp + CLOCK)
//		{
//			iValue |= g_conn.get() << i;
//		}
//
//		tp += CLOCK;
//	}
//
//	return iValue;
//}
//
//void do_node(char node_id)
//{
//	std::cout << "Hello World, I am node " << node_id << ".\n";
//
//	while (true)
//	{
//		std::cout << std::endl << "Enter destination node with a message to send : ";
//		std::getline(std::cin, sendMessage);
//		std::cout << "You entered [" << sendMessage << "]" << std::endl;
//		
//		sendState = Filled;
//		while (sendState != Next);
//		sendState = Empty;
//
//		//sendMessage.clear();
//	}
//}
//
//void do_node_NIC(char node_id)
//{
//	while (true)
//	{
//		auto currentClock{ std::chrono::high_resolution_clock::now() };
//
//		switch (sendState)
//		{
//		case Empty:
//		{
//			char nodeID{ GetNodeID(currentClock) };
//
//			if (nodeID == node_id)
//			{
//				char			sentNodeID{ GetNodeID(currentClock) };
//				int				iSize{ GetSignal(currentClock) };
//				std::string		getMessage{};
//				
//				for (int i{ 0 }; i < iSize; ++i)
//				{
//					getMessage.push_back(GetSignal(currentClock));
//				}
//
//				std::cout << "\nNode " << sentNodeID << " sent [" << getMessage << "]" << std::endl;
//			}
//
//			break;
//		}
//		case Filled:
//		{
//			SetSignal(currentClock, 1, 1);
//			SetSignal(currentClock, 8, sendMessage.at(0));				// 수신 노드 ID 전송
//			SetSignal(currentClock, 1, 0);
//
//			SetSignal(currentClock, 1, 1);
//			SetSignal(currentClock, 8, node_id);						// 발신 노드 ID 전송
//			SetSignal(currentClock, 1, 0);
//
//			SetSignal(currentClock, 1, 1);
//			SetSignal(currentClock, 8, sendMessage.size());			// 메세지의 길이 전송
//			SetSignal(currentClock, 1, 0);
//
//			for (int i{ 1 }; i < sendMessage.size(); ++i)
//			{
//				SetSignal(currentClock, 1, 1);
//				SetSignal(currentClock, 8, sendMessage.at(i));
//				SetSignal(currentClock, 1, 0);
//			}
//
//			sendState = Next;
//
//			break;
//		}
//		default:
//			std::cout << "Error! " << std::endl;
//			break;
//		}
//	}
//}

#include <iostream>
#include <chrono>
#include <string>
#include "conn.h"

using namespace std;

// send_state	
// 0: 아무런 데이터가 없다
// 1 : 데이터를 넣었으니 NIC는 받아 가라
// 2 : NIC가 다 받아서 처리했으니 다음 작업을 해라)
volatile int send_state = 0;
bool working = false;
string send_mess;
string receive_mess;

const chrono::microseconds CLOCK{ 1000 };
//num개의 비트를 CLOCK마다 g_conn에 실어주는 함수. 비트는 value에서 가져 온다. 
//다른 노드와 충돌해서 신호가 변경되었는가를 검사해서 리턴 한다.
//전송이 끝나면 tp도 전송이 끝난 시간으로 업데이트 된다.
bool set_signal(chrono::high_resolution_clock::time_point& tp, int num, unsigned int value) {
	bool collision = false;
	for (int i = 0; i < num; ++i) {
		bool bit = (value & (1 << (num - i - 1))) != 0;
		g_conn.set(bit);
		while (chrono::high_resolution_clock::now() < tp + CLOCK)
			if (g_conn.get() != bit)collision = true;
		//if (collision)
			//cout << "Collision!\n";
		tp += CLOCK;
	}
	return collision;
}
//num개의 비트를 CLOCK마다 받아오는 함수.
//code 0: 마지막 비트 받는전용
//code 1: 문자를 받는다
void get_signal(chrono::high_resolution_clock::time_point& tp, int num, int code) {
	char c = 0;
	for (int i = 0; i < num; ++i) {
		while (chrono::high_resolution_clock::now() < tp + CLOCK);
		if (chrono::high_resolution_clock::now() > tp + CLOCK) {
			switch (code)
			{
			case 1:
				if (g_conn.get()) {
					c |= 1 << (7 - i);
					//cout << "1 "; 
				}
				else {
					//cout << "0 ";
				}
				break;
			default:
				break;
			}
			tp += CLOCK;
		}
	}
	if (code == 1)
		receive_mess.push_back(c);
	//cout << receive_mess.back() << " ";
	//cout << "read " << num << "bit end\n";
}

void wait_signal(chrono::high_resolution_clock::time_point& tp) {
	while (chrono::high_resolution_clock::now() > tp + (chrono::milliseconds)100); // 100
}

void do_node(char node_id)
{
	int a;
	bool end_node = false;

	cout << "Enter: ";
	while (true) {
		getline(cin, send_mess);
		while (working);
		if (!working)
			send_state = 1;
		while (2 != send_state);
		send_state = 0;
	}
}

void check_conn(bool& my_conn)
{
	if (my_conn != g_conn.get()) {
		cout << "\n\ng_conn changed to";
		if (true == my_conn) cout << " False\n";
		else cout << " True\n";
		my_conn = !my_conn;
		cout << "Enter: ";
	}
}

void do_node_NIC(char node_id)
{
	bool read = false;
	bool finish = false;
	while (true) {
		auto curr = chrono::high_resolution_clock::now();
		bool my_conn = g_conn.get();
		//while (1 != send_state);
		if (send_state == 1) {
			cout << send_mess;

			// 송신자 전송
			set_signal(curr, 1, 1);
			set_signal(curr, 8, node_id);
			set_signal(curr, 1, 0);

			// 문자열 전송
			for (int i = 0; i < send_mess.length(); ++i) {
				set_signal(curr, 1, 1);
				set_signal(curr, 8, send_mess[i]);
				set_signal(curr, 1, 0);
			}
			// 문자열 끝났다는 신호 전송
			set_signal(curr, 1, 1);
			set_signal(curr, 8, '\0');
			set_signal(curr, 1, 0);

			send_mess.clear();

			send_state = 2;
			cout << "\nEnter: ";
		}
		else if (send_state == 0) {
			if (!read) read = g_conn.get();
			else {
				// 문자열 수신
				working = true;
				get_signal(curr, 8, 1);
				get_signal(curr, 1, 0);
				read = false;
				if (receive_mess.back() == '\0') {
					if (receive_mess[1] == node_id) {
						finish = true;
					}
					else {
						receive_mess.clear();
						//set_signal(curr, 1, 0);
						working = false;
					}
				}
			}
			if (finish) {
				cout << "node_" << receive_mess[0] << " send: ";
				receive_mess.erase(0, 2);
				cout << receive_mess << endl;
				finish = false;
				receive_mess.clear();
				working = false;
				//set_signal(curr, 1, 0);
			}
		}
	}
}