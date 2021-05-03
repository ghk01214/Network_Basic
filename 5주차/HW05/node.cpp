#include <iostream>
#include <string>
#include <map>
#include <mutex>
#include "conn.h"

using namespace std;

enum FrameType
{
	requestARP = 75, responseARP, requestDHCP, responseDHCP, sendMessage
};

struct ARPRequest
{
	char fromMac;						// 0
	char toMac;							// 0
	char frameType = requestARP;		// ARP request = 75
	int nodeAddress;
};

struct ARPResponse
{
	char fromMac;
	char toMac;
	char frameType;		// ARP response = 76
	int nodeAddress;
	int macAddress;
};

struct DHCPRequest
{
	char fromMac;
	const char toMac = 0;				// should be 0
	char frameType = requestDHCP;		// DHCP request = 77
};

struct DHCPResponse
{
	char fromMac;
	char toMac;
	char frameType;		// DHCP response = 78
	int nodeAddress;
	int lanAddress;
	int macAddress;
};

struct SendMessage
{
	char fromMac;
	char toMac;
	char frameType;		// MESSAGE packet = 78
	int size;
	string data;

	SendMessage() { data.reserve(MAX_DATA_SIZE); }
};

class AddressManager
{
public:
	int GetMacAddress(int nodeAddress)
	{
		int macAddress = -1;

		tableLock.lock();

		if (node2macTable.count(nodeAddress) != 0)
			macAddress = node2macTable[nodeAddress];

		tableLock.unlock();

		return macAddress;
	}
	void SetAddress(int macAddress, int nodeAddress)
	{
		tableLock.lock();

		node2macTable[nodeAddress] = macAddress;
		mac2nodeTable[macAddress] = nodeAddress;

		tableLock.unlock();
	}
	int AssignNodeAddress(int macAddress)
	{
		int nodeAddress = -1;

		tableLock.lock();

		if (mac2nodeTable.count(macAddress) == 0)
		{
			for (int i = 2; i < 9; ++i)
			{
				if (node2macTable.count(i)  == 0)
				{
					node2macTable[i] = macAddress;
					mac2nodeTable[macAddress] = i;
					nodeAddress = i;

					break;
				}
			}
		}

		tableLock.unlock();

		return nodeAddress;
	}
private:
	mutex tableLock;
	map<char, char> mac2nodeTable;
	map<char, char> node2macTable;
};

AddressManager addressManager;

void DHCP(const char mac_addr, NIC& g_nic)
{
	if (mac_addr == 'A')
	{
		
	}
}

void do_node(NIC& g_nic)
{
	const char mac_addr = g_nic.GetMACaddr();

	DHCPRequest request;
	request.fromMac = mac_addr;

	string frame;
	frame.push_back(request.fromMac);
	frame.push_back((char)request.toMac);
	frame.push_back((char)request.frameType);

	if (mac_addr != 'A')
		g_nic.SendFrame(sizeof(frame), &frame);

	SendMessage message;
	message.fromMac = mac_addr;
	message.data.reserve(MAX_DATA_SIZE);

	cout << "Hello World, I am a node with MAC address [" << mac_addr << "]." << endl;

	while (true)
	{
		cout << "\nEnter Message to Send : ";
		getline(cin, message.data);

		message.toMac = message.data[0];
		g_nic.SendFrame(sizeof(message), &message);
	}
}

void interrupt_from_link(NIC& g_nic, int recv_size, char* frame)
{
	const char mac_addr = g_nic.GetMACaddr();

	switch (frame[2])
	{
	case requestARP:
	{
		ARPRequest* p = (ARPRequest*)frame;


		break;
	}
	case responseARP:
	{
		ARPResponse* p = (ARPResponse*)frame;

		break;
	}
	case requestDHCP:
	{
		DHCPRequest* p = (DHCPRequest*)frame;
		int newNodeAddress = addressManager.AssignNodeAddress(p->fromMac);


		break;
	}
	case responseDHCP:
	{
		DHCPResponse* p = (DHCPResponse*)frame;

		if (mac_addr != p->toMac)
			break;

		break;
	}
	case sendMessage:
	{

		break;
	}
	}

	char* from_mac = frame;
	char* to_mac = frame + 1;
	char* mess = frame + 2;

	if (*to_mac != mac_addr)
		return;
	cout << "\n\nMessage from NODE " << *from_mac << " : ";
	cout << mess << endl;
	cout << "\nEnter Message to Send : ";
}