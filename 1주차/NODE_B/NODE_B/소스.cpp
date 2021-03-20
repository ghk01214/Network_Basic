#include <iostream>
#include <thread>
#include <string>
#include "conn.h"

using namespace std;

#define WAITING 250ms
#define GAP 35ms

void do_node(char node_id)
{
	int a;
	bool isMine = false;	// 첫글자를 확인해서 그것이 나의 것이면 true 아니면 false인 변수

	cout << "Hello World, I am node " << node_id << ".\n";
	getchar();

	if (node_id == 'a' || node_id == 'A') {

		string str;

		while (true)
		{
			cout << "Enter string to send: ";

			getline(cin, str);

			cout << "You entered [ " << str << " ]" << endl;

			int i_size = str.size();

			for (int i = 0; i < i_size + 1; ++i) {
				g_conn.set(true);

				if (i == 1) {
					for (int j = 0; j < 8; ++j)
					{
						g_conn.set((i_size & (1 << j)) >> j);
						this_thread::sleep_for(GAP);
					}
				}
				else {
					unsigned char c = '0';
					if (i == 0)
						c = str.at(i);
					else
						c = str.at(i - 1);
					cout << c;
					for (int j = 0; j < 8; ++j)
					{
						g_conn.set((c & (1 << j)) >> j);
						this_thread::sleep_for(GAP);
					}
					if (i == 0)
						this_thread::sleep_for(10ms);
				}
				this_thread::sleep_for(WAITING);
			}
			i_size = 0;
			g_conn.set(false);

			cout << endl;
		}

	}
	else if (
		node_id == 'b' || node_id == 'B'
		|| node_id == 'c' || node_id == 'C'
		|| node_id == 'd' || node_id == 'D') {

		unsigned char c = 0;

		while (true)
		{
			bool isMine = false;
			// 1차로 체크, 맞으면 다음으로 넘어가기 아니면 계속 읽되 기다리기
			while (!isMine)
			{
				if (true == g_conn.get())
				{
					for (int i = 0; i < 8; ++i)
					{
						this_thread::sleep_for(GAP);
						c |= (g_conn.get() << i);
					}

					if (c == node_id) {
						c = 0;
						isMine = true;
						g_conn.set(false);
					}
					else {
						c = 0;
						this_thread::sleep_for(WAITING);
					}
				}
			}

			this_thread::sleep_for(WAITING);

			// 2차로 갯수 체크
			bool bCnt = true;
			int i_size = 0;
			while (bCnt)
			{
				if (true == g_conn.get())
				{
					for (int i = 0; i < 8; ++i)
					{
						this_thread::sleep_for(GAP);
						i_size |= (g_conn.get() << i);
					}
					g_conn.set(false);
					bCnt = false;
				}
			}

			this_thread::sleep_for(WAITING);

			// 계속 읽다가 끝이나면 맨 위로 돌아가기

			cout << "NODE A sent [ ";
			int i_cnt = 0;
			while (isMine)
			{
				if (true == g_conn.get())
				{
					for (int i = 0; i < 8; ++i)
					{
						this_thread::sleep_for(GAP);
						c |= (g_conn.get() << i);
					}
					++i_cnt;
					cout << c;
					c = 0;
					g_conn.set(false);
					this_thread::sleep_for(WAITING);
				}
				else if (i_cnt == i_size - 1) {
					cout << " ]" << endl;
					g_conn.set(false);
					i_cnt = 0;
					i_size = 0;
					break;
				}
			}
		}
	}
}