#include <iostream>
#include <string>
#include <thread>
#include "conn.h"
#define lagTime 200ms

using namespace std;

void do_node(char node_id)
{
	char nodeID{};
	bool startNode{ false };

	cout << "Enter NODE ID(A, B, C, D) : ";
	cin >> nodeID;

	while (true)
	{
		if (nodeID == 'A' || nodeID == 'a')
		{
			if (!startNode)
			{
				cout << "노드 A를 시작합니다." << endl;
				startNode = true ;
			}
			
			std::string str;

			cout << "문자를 전송할 노드의 ID와 전송할 문자를 같이 입력 : ";
			getline(std::cin, str);

			g_conn.set(true);
			this_thread::sleep_for(lagTime);

			switch (str[0])
			{
			case 'B': case 'b':
			{
				for (int i = 0; i < 2; ++i)
				{
					g_conn.set(false);
					this_thread::sleep_for(lagTime);
				}

				for (int i = 0; i < str.size(); ++i)
				{
					//char c = str[i];

					for (int j = 0; j < 8; ++j)
					{
						g_conn.set(str[i] & true);

						str[i] >>= 1;

						this_thread::sleep_for(lagTime);
					}
				}

				break;
			}
			case 'C': case 'c':
			{
				g_conn.set(false);
				this_thread::sleep_for(lagTime);
				g_conn.set(true);
				this_thread::sleep_for(lagTime);

				break;
			}
			case 'D': case 'd':
			{
				for (int i = 0; i < 2; ++i)
				{
					g_conn.set(true);
					this_thread::sleep_for(lagTime);
				}

				break;
			}
			default:
			{
				cout << "잘못된 입력입니다." << endl;
				cout << "다시 입력해주십시오." << endl;

				break;
			}
			}
		}
		// ASCII 코드 값이 대소문자 b~d 사이의 값일 때
		else if ((nodeID >= 66 && nodeID <= 68) || (nodeID >= 98 && nodeID <= 100))
		{
			if (!startNode)
			{
				cout << "노드 " << nodeID << "를 시작합니다." << endl;
				cout << "노드 A와 연결을 시도합니다." << endl;
				startNode = true;
			}

			if (g_conn.get())
			{
				cout << "노드 " << nodeID << "와 연결되었습니다." << endl;

				this_thread::sleep_for(lagTime);

				if (!g_conn.get())
				{
					this_thread::sleep_for(lagTime);

					//노드 B
					if (!g_conn.get())
					{
						this_thread::sleep_for(lagTime);

					}
					// 노드 C
					else
					{
						this_thread::sleep_for(lagTime);


					}
				}
				// 노드 D
				else
				{
					this_thread::sleep_for(lagTime);

				}
			}


		}
		else
		{
			cout << "노드 " << nodeID << "가 존재하지 않습니다." << endl;
			cout << "프로그램을 종료합니다." << endl;

			break;
		}
	}
}