#include <iostream>
#include "../sws/client_ws.hpp"
#include <future>
#include <openssl/md5.h>
#include<string>
#include <stdarg.h>
#pragma comment( linker, "/subsystem:windows /entry:mainCRTStartup")
#pragma warning(disable:4996)
using namespace std;
std::string addr;
using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;
std::shared_ptr<WsClient::Connection> scon = nullptr;

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
template <size_t size> void u2a(char(&buf)[size], int num) {
	int nt = size - 1;
	buf[nt] = 0;
	for (auto i = nt - 1; i >= 0; --i) {
		char d = '0' + (num % 10);
		num /= 10;
		buf[i] = d;
	}
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
string& repall(string& str, const string& olds, const string& news)
{
	string::size_type pos = 0;
	while ((pos = str.find(olds)) != string::npos)
	{
		str = str.replace(str.find(olds), olds.length(), news);
	}
	return str;
}


void wsclient() {
	WsClient client(addr);
	client.on_message = [](shared_ptr<WsClient::Connection> /*connection*/, shared_ptr<WsClient::InMessage> in_message) {
		string msg = in_message->string();
		cout << "FromServer: " + in_message->string() << endl;

	};

	client.on_open = [](shared_ptr<WsClient::Connection> connection) {
		scon = connection;
	};

	client.on_close = [](shared_ptr<WsClient::Connection> /*connection*/, int /*status*/, const string& /*reason*/) {
		scon = nullptr;
	};

	client.on_error = [](shared_ptr<WsClient::Connection> /*connection*/, const SimpleWeb::error_code& /*ec*/) {
		scon = nullptr;
	};
	client.start();
}

std::string gettime1r() {
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
	u2a(buf, time.tm_min - 1);
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
					in_msg = in_msg.replace(in_msg.find("{\"passwd\"") + 1, 45, "");
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
//{"passwd":"%pwd%","operate":"runcmd","cmd":"list"}
pair<string, string> getpasswd(string msg, string passwd) {
	string bp = getbody(msg);
	string pw = passwd + gettime() + "@" + bp;
	return { MD5(pw),pw };
}
int main(int argc, char* args[]) {
	for (int a = 0; a < argc; a++)
		cout << "[" << a << "] " << args[a] << endl;
	if (string(args[1]) == "cmd") {
		if (argc == 5) {
			addr = string(args[2]);
			string pw = string(args[3]);
			string cmd = string(args[4]);
			thread t(wsclient);
			t.detach();
			Sleep(100);
			cmd = "{ \"operate\":\"runcmd\",\"passwd\":\"%pwd%\",\"cmd\":\"" + cmd + "\" }";
			cout << cmd << endl;
			if (cmd.find("%pwd%")) {
				repall(cmd, "%pwd%", MD5(""));
			}
			string apw = getpasswd(cmd, pw).first;
			if (cmd.find("D41D8CD98F00B204E9800998ECF8427E")) {
				repall(cmd, "D41D8CD98F00B204E9800998ECF8427E", apw);
			}
			if (scon == nullptr) {
				cout << "Cannot Connect To Server" << endl;
				return 0;
			}
			scon->send("{\"operate\":\"setdesp\",\"desp\":\"ArgsClient\"}");
			scon->send(cmd);
			Sleep(100);
			scon->send_close(0);
			return 0;
		}
		else {
			cout << "Args count not match" << endl;
		}
	}
	else if (string(args[1]) == "ping") {
		if (argc == 3) {
			addr = string(args[2]);
			thread t(wsclient);
			t.detach();
			Sleep(500);
			if (scon == nullptr) {
				cout << "Cannot Connect To Server" << endl;
				return 0;
			}
			cout << "tes" << endl;
			scon->send("{\"operate\":\"setdesp\",\"desp\":\"ArgsClient\"}");
			scon->send("{ \"operate\":\"ping\"}");
			Sleep(500);
			scon->send_close(0);
			return 0;
		}else
			if (argc == 4) {
				addr = string(args[2]);
				int times = atoi(args[3]);
				for (int a = 0; a < times; a++) {
					thread t(wsclient);
					t.detach();
					Sleep(500);
					if (scon == nullptr) {
						cout << "Cannot Connect To Server " << "[" << a << "]" << endl;
						return 0;
					}
					scon->send("{\"operate\":\"setdesp\",\"desp\":\"ArgsClient\"}");
					scon->send("{ \"operate\":\"ping\"}");
					Sleep(500);
					scon->send_close(0);
				}
				return 0;
		}
		return 0;
	}
}
