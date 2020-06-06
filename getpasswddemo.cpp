#include <iostream>
#include <time.h>
#include <chrono>
#include <string>
#include <openssl\md5.h>
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
string getbody(string in_msg) {
	for (int a = 0; a == 0;) {
		if (in_msg.find("\"passwd\"") != string::npos) {
			int len = in_msg.length() - in_msg.find(",\"passwd\"");
			if (len > 44) {
				if (in_msg.find(",\"passwd\"") != string::npos) {
					in_msg = in_msg.replace(in_msg.find(",\"passwd\""), 44, "");
				}
				if (in_msg.find("{\"passwd\":\"") != string::npos) {
					in_msg = in_msg.replace(in_msg.find("{\"passwd\"") + 1, 44, "");
				}
			}
			else { a = 1; }
		}
		else {
			a = 1;
		}

	}
	std::cout << in_msg << endl;
	return in_msg;
}
pair<string, string> getpasswd(string msg, string passwd) {
	string bp = getbody(msg);
	cout << "bp" << bp << endl;
	string pw = passwd + gettime() + "@" + bp;
	return { MD5(pw),pw };
}
int main()
{
	auto tes1 = getpasswd("{\"passwd\":\"" + MD5("t") + "\",\"text\":\"tes\"}", "pw");
	auto tes2 = getpasswd("{\"text\":\"tes2\",\"passwd\":\"" + MD5("t") + "\"}", "pw");
	std::cout << tes1.first << " - " << tes1.second << endl;
	std::cout <<tes2.first << " - " << tes2.second << endl;
	cout << MD5("") << endl;
	system("pause");
}
