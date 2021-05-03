#pragma once
#include <memory>
#include <chrono>
#include <mutex>
#include <queue>
#include <concurrent_queue.h>

constexpr int MAX_DATA_SIZE = 100;

class RAW_FRAME {
public:
	bool f_lock;
	int  sequence;
	std::chrono::system_clock::time_point last_update;
	int size;
	char frame_data[MAX_DATA_SIZE];
	
	void Init() {
		sequence = 0;
		f_lock = false;
		last_update = std::chrono::system_clock::now() - std::chrono::seconds(1);
	}
};


class NIC
{
public:
	void SendFrame(int frmae_size, void *frame);
	void RecvFrame(int *frmae_size, char *frame);
	void Init(char mac_addr);
	char GetMACaddr() { return mac_addr; }
private:	
	void check_recv();
	char mac_addr;
	int line_status;
	int last_seq_num;
	RAW_FRAME *raw_frame;
};

