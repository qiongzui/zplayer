// zplayer_test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <zplayer.h>
#include <windows.h>

int main()
{
	//SetConsoleOutputCP(CP_UTF8);

    std::cout << "Hello World!\n";
	std::string filePath = R"(C:\Users\51917\Downloads\20240418-153114.mp4)";
	std::string filePath1 = R"(C:\Users\51917\Desktop\test\2.mp4)";
	auto zplayer = zplayer_create(nullptr);
	zplayer_open(zplayer, filePath1.c_str());
	zplayer_play(zplayer);

	int duration = -1;
	int curMs = -1;
	zplayer_query(zplayer, MsgType::MsgType_DurationMs, &duration);
	while (true) {
		zplayer_query(zplayer, MsgType::MsgType_CurrentTimestampMs, &curMs);
		if (curMs >= duration)
		{
			break;
		}
	}
	zplayer_close(&zplayer);
	std::cout << "play finish" << std::endl;
	getchar();
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
