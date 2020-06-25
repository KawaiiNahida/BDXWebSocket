#include <iostream>
#include <time.h>
#include <chrono>
#include <string>
#include <openssl\md5.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include "../BehaviorLog/csvwriter.h"
#pragma warning(disable:4996)


using namespace std;
#pragma warning(disable:4996)
template <size_t size> void u2a(char(&buf)[size], int num) {
	int nt = size - 1;
	buf[nt] = 0;
	for (auto i = nt - 1; i >= 0; --i) {
		char d = '0' + (num % 10);
		num /= 10;
		buf[i] = d;
	}
}
std::string MD5(const string& src) {
	MD5_CTX ctx;

	string md5_string;
	unsigned char md[16] = { 0 };
	char tmp[33] = { 0 };

	MD5_Init(&ctx);
	MD5_Update(&ctx, src.c_str(), src.size());
	MD5_Final(md, &ctx);

	for (int i = 0; i < 16; ++i)
	{
		memset(tmp, 0x00, sizeof(tmp));
		sprintf(tmp, "%02X", md[i]);
		md5_string += tmp;
	}
	return md5_string;
}
std::string gettime() {
	auto timet = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	tm time;
	char buf[3] = { 0 };
	localtime_s(&time, &timet);
	std::string str(std::to_string((time.tm_year + 1900)));
	u2a(buf, time.tm_mon + 1);
	str += buf;
	u2a(buf, time.tm_mday);
	str += buf;
	u2a(buf, time.tm_hour);
	str += buf;
	u2a(buf, time.tm_min);
	str += buf;
	return str;
}
pair<string, string> getpasswd(string msg, string passwd) {
	string pw = passwd + gettime() + "@"+msg;
	return { MD5(pw),pw };
}/*
typedef pair<string, string> twos;
string makejson(initializer_list<twos> args)
{
	rapidjson::StringBuffer mmsg;
	rapidjson::Writer<rapidjson::StringBuffer> writer(mmsg);
	writer.StartObject();
	for (auto count = args.begin(); count != args.end(); count++) {
		writer.Key(count->first.c_str());
		writer.String(count->second.c_str());
	}
	writer.EndObject();
	return mmsg.GetString();
}*/
int main()
{
	auto tes1 = getpasswd("{\"passwd\":\"\",\"text\":\"tes\"}", "pw");
	cout << "{\"passwd\":\"" + tes1.first + "\",\"text\":\"tes\"}" << endl;
	std::cout << tes1.first << " - " << tes1.second << endl;
	system("pause");
}
