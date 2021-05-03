#include <iostream>
#include <map>
#include <vector>
#include <thread>
#include <concurrent_queue.h>

using namespace std;
#include <Windows.h>
#include "conn.h"



#define SH_MEM_ID L"Conn"

NIC g_nic;

concurrency::concurrent_queue <RAW_FRAME> frame_queue;

void do_node(NIC &g_nic);
void interrupt_from_link(NIC &g_nic, int recv_size, char* frame);


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

void NIC::Init(char mac)
{
	mac_addr = mac;
	const int BLOCK_SIZE = sizeof(RAW_FRAME);
	while (true) {
		HANDLE hSHM = OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, SH_MEM_ID);
		if (0 == hSHM) {
			hSHM = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, BLOCK_SIZE, SH_MEM_ID);
			if (0 == hSHM) {
				int err = GetLastError();
				wcout << L"전송 매체 연결에 실패했습니다. 다시 시도합니다." << endl;
				continue;
			}
		}
		raw_frame = reinterpret_cast<RAW_FRAME*>(MapViewOfFile(hSHM, PAGE_READONLY, 0, 0, BLOCK_SIZE));
		if (nullptr == raw_frame) {
			int err_no = GetLastError();
			LPWSTR wmess = GetErrMessage(err_no);
			wcout << L"Mapping Error[" << err_no << L"] " << wmess << endl;
			LocalFree(wmess);
			return;
		}
		break;
	}
	last_seq_num = raw_frame->sequence;
}

void NIC::check_recv()
{
	if (raw_frame->sequence <= last_seq_num) {
		this_thread::sleep_for(10ms);
		return;
	}
	while (true == raw_frame->f_lock) {
		bool old_value = false;
		if (true == atomic_compare_exchange_strong(reinterpret_cast<atomic_bool *>(&raw_frame->f_lock), &old_value, true)) break;
		this_thread::sleep_for(10ms);
	}
	frame_queue.push(*raw_frame);
	last_seq_num = raw_frame->sequence;
	raw_frame->f_lock = false;
}

void NIC::SendFrame(int frame_size, void* frame)
{
	using namespace chrono;

	auto wait_time = system_clock::now() - raw_frame->last_update;
	if (wait_time < 1s) {
		check_recv();
		this_thread::sleep_for(1s - wait_time);
	}

	while (true == raw_frame->f_lock) {
		bool old_value = false;
		if (true == atomic_compare_exchange_strong(reinterpret_cast<atomic_bool *>(&raw_frame->f_lock), &old_value, true)) break;
		check_recv();
	}
	memcpy(raw_frame->frame_data, frame, frame_size);
	raw_frame->last_update = system_clock::now();
	raw_frame->sequence++;
	raw_frame->size = frame_size;
	last_seq_num = raw_frame->sequence;
	raw_frame->f_lock = false;
}

void NIC::RecvFrame(int* frame_size, char* frame)
{
	RAW_FRAME f;
	*frame_size = 0;
	check_recv();
	if (false == frame_queue.try_pop(f)) return;
	*frame_size = f.size;
	memcpy(frame, f.frame_data, f.size);
	return;
}

void do_node_NIC(char node_id, NIC& g_conn)
{
	while (true) {
		int data_size;
		char frame[MAX_DATA_SIZE];
		g_conn.RecvFrame(&data_size, frame);
		if (0 != data_size)
			interrupt_from_link(g_conn, data_size, frame);
	}
}

int main()
{
	std::wcout.imbue(std::locale("korean"));

	char node_type;

	for (;;) {
		wcout << L"\n새 노드를 시작합니다. MAC 주소 (A 에서 Z)를 입력하시오. : ";
		cin >> node_type;
		node_type = toupper(node_type);
		if ('H' == node_type) break;
		if ((node_type >= 'A') && (node_type <= 'Z')) break;
		cout << "Wrong Node Type!\n\n";
	}

	char mess[200];
	cin.getline(mess, 199);
	thread nic{ do_node_NIC, node_type, ref(g_nic) };
	g_nic.Init(node_type);
	do_node(g_nic);
	nic.join();
}