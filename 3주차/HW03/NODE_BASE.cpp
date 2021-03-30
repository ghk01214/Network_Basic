#include <iostream>
#include <map>
#include <thread>
using namespace std;
#include <Windows.h>
#include "conn.h"


void do_node(char node_type);
void do_node_NIC(char node_type);

#define SH_MEM_ID L"Conn"

CONN g_conn;


CONN::CONN() : stat(nullptr) {}
void CONN::init()
{
	while (true) {
		HANDLE hSHM = OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, SH_MEM_ID);
		if (0 == hSHM) {
			hSHM = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 1, SH_MEM_ID);
			if (0 == hSHM) {
				int err = GetLastError();
				wcout << L"전송 매체 연결에 실패했습니다. 다시 시도합니다." << endl;
				continue;
			}
		}
		stat = (bool*)MapViewOfFile(hSHM, PAGE_READONLY, 0, 0, 0);
		break;
	}
	wcout << L"전송 매체에 연결되었습니다.\n";
}

void CONN::set(bool value)
{
	*stat = value;
}

bool CONN::get()
{
	return *stat;
}

int main()
{
	std::wcout.imbue(std::locale("korean"));
	g_conn.init();
	char node_type;

	for (;;) {
		cout << "Enter NODE ID (A, B, C, D, ... H) : ";
		cin >> node_type;
		node_type = toupper(node_type);
		if ((node_type >= 'A') && (node_type <= 'H')) break;
		cout << "Wrong Node Type!\n\n";
	}
	char mess[200];
	cin.getline(mess, 199);
	thread nic{ do_node_NIC, node_type };
	do_node(node_type);
	nic.join();
}