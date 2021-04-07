#include <iostream>
#include <string>

#include "conn.h"

constexpr int MAX_NODES = 16;
extern const unsigned int NUM_NODES;

using namespace std;

constexpr int NIC_STATE_IDLE = 0;
constexpr int NIC_STATE_HAVE_CMD = 1;
constexpr int NIC_STATE_DONE = 2;

constexpr int CMD_QUIT = 0;
constexpr int CMD_SET_CONN = 1;
constexpr int CMD_RESET_CONN = 2;
constexpr int CMD_GET_CONN = 3;

struct NIC {					// g_conn을 관리하기 위한 관리 객체
	string m_nic_name;
	volatile int g_send_state;	// NIC_STATE_IDLE : nothing to do,   
								// NIC_STATE_HAVE_CMD: node sent cmd to NIC,  
								// NIC_STATE_DONE: NIC sent reply to node
	volatile bool g_value_from_NIC;
	volatile char g_cmd_to_NIC;
	int padding[64];				// 멀티코어 프로그래밍 시간에 다룰 내용, 성능 향상을 위해서 존재.
};

NIC g_nic[4];



void check_conn(const char *nic_name, bool& my_conn, CONN& g_conn);

bool send_command_to_NIC(NIC &nic, int cmd)
{
	nic.g_cmd_to_NIC = cmd;
	nic.g_send_state = NIC_STATE_HAVE_CMD;
	while (NIC_STATE_DONE != nic.g_send_state);
	bool ret = nic.g_value_from_NIC;
	nic.g_send_state = 0;
	return ret;
}

void do_hub()
{
	wcout << L"\n허브로 동작합니다. Port들을 초기화 합니다\n";
	for (unsigned i = 0; i < NUM_NODES; ++i) {
		g_nic[i].m_nic_name = string{ "Port " };
		g_nic[i].m_nic_name += ('A' + i);
		send_command_to_NIC(g_nic[i], CMD_RESET_CONN);
	}

	string cmd;
	while (true) {
		cout << "Enter CMD => 0(quit), s(scan port's value), NODE+VALUE(set port ex:'A1') : ";
		cin >> cmd;
		if (to_string(CMD_QUIT) == cmd) break;
		else if (cmd == "s") {
			for (unsigned i = 0; i < NUM_NODES; ++i) {
				char port_name = 'A' + i;
				cout << "Port " << port_name << " = ";
				if (true == send_command_to_NIC(g_nic[i], CMD_GET_CONN))
					cout << "True";
				else
					cout << "False";
				cout << ",  ";
			}
			cout << endl;
		}
		else {
			unsigned int port_id = cmd[0] - 'A';
			unsigned int  port_cmd = cmd[1] - '0';
			if ((port_id < NUM_NODES) && (port_cmd < 2)) {
				if (0 == port_cmd)
					send_command_to_NIC(g_nic[port_id], CMD_RESET_CONN);
				else
					send_command_to_NIC(g_nic[port_id], CMD_SET_CONN);
			}
			else {
				cout << "Invalid CMD\n";
			}

		}
	
	}
}

void do_hub_NIC0(CONN& g_conn)
{
	constexpr int PORT_NUM = 0;
	NIC& nic = g_nic[PORT_NUM];
	bool my_conn = g_conn.get();
	for (;;) {
		while (NIC_STATE_HAVE_CMD != nic.g_send_state)
			check_conn(nic.m_nic_name.c_str(), my_conn, g_conn);
		auto cmd = nic.g_cmd_to_NIC;
		switch (cmd) {
		case CMD_SET_CONN: g_conn.set(true); my_conn = true; break;
		case CMD_RESET_CONN: g_conn.set(false); my_conn = false;  break;
		case CMD_GET_CONN: nic.g_value_from_NIC = g_conn.get(); break;
		}
		nic.g_send_state = NIC_STATE_DONE;
		if (CMD_QUIT == cmd) break;
	}
}


void do_hub_NIC1(CONN& g_conn)
{
	constexpr int PORT_NUM = 1;
	NIC& nic = g_nic[PORT_NUM];
	bool my_conn = g_conn.get();
	for (;;) {
		while (NIC_STATE_HAVE_CMD != nic.g_send_state)
			check_conn(nic.m_nic_name.c_str(), my_conn, g_conn);
		auto cmd = nic.g_cmd_to_NIC;
		switch (cmd) {
		case CMD_SET_CONN: g_conn.set(true); my_conn = true; break;
		case CMD_RESET_CONN: g_conn.set(false); my_conn = false;  break;
		case CMD_GET_CONN: nic.g_value_from_NIC = g_conn.get(); break;
		}
		nic.g_send_state = NIC_STATE_DONE;
		if (CMD_QUIT == cmd) break;
	}
}


void do_hub_NIC2(CONN& g_conn)
{
	constexpr int PORT_NUM = 2;
	NIC& nic = g_nic[PORT_NUM];
	bool my_conn = g_conn.get();
	for (;;) {
		while (NIC_STATE_HAVE_CMD != nic.g_send_state)
			check_conn(nic.m_nic_name.c_str(), my_conn, g_conn);
		auto cmd = nic.g_cmd_to_NIC;
		switch (cmd) {
		case CMD_SET_CONN: g_conn.set(true); my_conn = true; break;
		case CMD_RESET_CONN: g_conn.set(false); my_conn = false;  break;
		case CMD_GET_CONN: nic.g_value_from_NIC = g_conn.get(); break;
		}
		nic.g_send_state = NIC_STATE_DONE;
		if (CMD_QUIT == cmd) break;
	}
}


void do_hub_NIC3(CONN& g_conn)
{
	constexpr int PORT_NUM = 3;
	NIC& nic = g_nic[PORT_NUM];
	bool my_conn = g_conn.get();
	for (;;) {
		while (NIC_STATE_HAVE_CMD != nic.g_send_state)
			check_conn(nic.m_nic_name.c_str(), my_conn, g_conn);
		auto cmd = nic.g_cmd_to_NIC;
		switch (cmd) {
		case CMD_SET_CONN: g_conn.set(true); my_conn = true; break;
		case CMD_RESET_CONN: g_conn.set(false); my_conn = false;  break;
		case CMD_GET_CONN: nic.g_value_from_NIC = g_conn.get(); break;
		}
		nic.g_send_state = NIC_STATE_DONE;
		if (CMD_QUIT == cmd) break;
	}
}

