#include <iostream>

#include "conn.h"

using namespace std;

volatile int g_send_state = 0;			// 0 : nothing to do,   1: node sent cmd to NIC,  2: NIC sent reply to node
constexpr int NIC_STATE_IDLE = 0;
constexpr int NIC_STATE_HAVE_CMD = 1;
constexpr int NIC_STATE_DONE = 2;

volatile bool g_value_from_NIC;

volatile char g_cmd_to_NIC;
constexpr int CMD_QUIT = 0;
constexpr int CMD_SET_CONN = 1;
constexpr int CMD_RESET_CONN = 2;
constexpr int CMD_GET_CONN = 3;

bool send_command_to_NIC(int cmd)
{
		g_cmd_to_NIC = cmd;
		g_send_state = NIC_STATE_HAVE_CMD;
		while (NIC_STATE_DONE != g_send_state);
		bool ret = g_value_from_NIC;
		g_send_state = 0;
		return ret;
}

void do_node(char node_id)
{
	int a;
	bool end_node = false;

	cout << "Hello World, I am node " << node_id << ".\n";
	for (;;) {
		cout << "\nEnter CMD => 0.Quit,    1:Set CONN,     2:Reset CONN,     3:Read CONN : ";
		cin >> a;
		bool ret = send_command_to_NIC(a);
		if (3 == a) {
			cout << "g_conn is ";
			if (true == ret) cout << "True\n";
			else cout << "False\n";
		}

		if (0 == a) break;
	}
}

void check_conn(const char* NIC_NAME,  bool& my_conn, CONN& g_conn)
{
	if (my_conn != g_conn.get()) {
		cout << "\n\n" << NIC_NAME << " changed to";
		if (true == my_conn) cout << " False\n";
		else cout << " True\n";
		my_conn = !my_conn;
		cout << "Enter CMD => ";
	}
}

void do_node_NIC(char node_id, CONN &g_conn)
{
	bool my_conn = g_conn.get();
	for (;;) {
		while (NIC_STATE_HAVE_CMD != g_send_state)
			check_conn("g_conn", my_conn, g_conn);
		auto cmd = g_cmd_to_NIC;
		switch (cmd) {
		case CMD_SET_CONN: g_conn.set(true); my_conn = true; break;
		case CMD_RESET_CONN: g_conn.set(false); my_conn = false;  break;
		case CMD_GET_CONN: g_value_from_NIC = g_conn.get(); break;
		}
		g_send_state = NIC_STATE_DONE;
		if (CMD_QUIT == cmd) break;
	}
}