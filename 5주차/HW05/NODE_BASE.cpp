#include <iostream>
#include <map>
#include <vector>
#include <thread>
using namespace std;
#include <Windows.h>
#include "conn.h"


extern const unsigned int NUM_NODES;
const unsigned int NUM_NODES = 4;		// 허브에서 연결 가능한 최대 노드 개수

void do_hub();
void do_hub_NIC0(CONN& conn);
void do_hub_NIC1(CONN& conn);
void do_hub_NIC2(CONN& conn);
void do_hub_NIC3(CONN& conn);
void do_node(char node_type);
void do_node_NIC(char node_type, CONN& conn);

#define SH_MEM_ID L"Conn"

CONN g_conn[NUM_NODES];

LPWSTR GetErrMessage(DWORD err_no)
{
	// Retrieve the system error message for the last-error code

	LPWSTR lpMsgBuf = nullptr;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	// Display the error message and exit the process
	return lpMsgBuf;
}


CONN::CONN() : stat(nullptr) {}
void CONN::init(int i)
{
	constexpr int BLOCK_SIZE = 512;		// False Sharing을 막기위한 거리두기

	while (true) {
		HANDLE hSHM = OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, SH_MEM_ID);
		if (0 == hSHM) {
			hSHM = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, BLOCK_SIZE * NUM_NODES, SH_MEM_ID);
			if (0 == hSHM) {
				int err = GetLastError();
				wcout << L"전송 매체 연결에 실패했습니다. 다시 시도합니다." << endl;
				continue;
			}
		}
		stat = reinterpret_cast<bool*>(MapViewOfFile(hSHM, PAGE_READONLY, 0, 0, BLOCK_SIZE)) + BLOCK_SIZE * i;
		if (nullptr == stat) {
			int err_no = GetLastError();
			LPWSTR wmess = GetErrMessage(err_no);
			wcout << L"Mapping Error[" << err_no << L"] " << wmess << endl;
			LocalFree(wmess);
		}
		break;
	}
	char port_id = 'A' + i;
	wcout << L"허브의 Port " << port_id << L"가 활성화 되었습니다.\n";
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
	for (int i = 0; i < NUM_NODES; ++i)
		g_conn[i].init(i);
	char node_type;

	for (;;) {
		wcout << L"\n허브이면 (H), 노드이면 연결된 허브의 포트 ID (A, B, C 또는 D)를 입력하시오. : ";
		cin >> node_type;
		node_type = toupper(node_type);
		if ('H' == node_type) break;
		if ((node_type >= 'A') && (node_type <= 'D')) break;
		cout << "Wrong Node Type!\n\n";
	}

	char mess[200];
	cin.getline(mess, 199);
	if ('H' == node_type) {
		vector <thread> nic;
		nic.emplace_back(do_hub_NIC0, ref(g_conn[0]));
		nic.emplace_back(do_hub_NIC1, ref(g_conn[1]));
		nic.emplace_back(do_hub_NIC2, ref(g_conn[2]));
		nic.emplace_back(do_hub_NIC3, ref(g_conn[3]));
		do_hub();
		for (auto& th : nic)
			th.join();
	}
	else {
		thread nic{ do_node_NIC, node_type, ref(g_conn[node_type - 'A']) };
		do_node(node_type);
		nic.join();
	}
}