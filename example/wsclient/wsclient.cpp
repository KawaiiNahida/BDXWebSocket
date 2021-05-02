#include <iostream>
#include "../sws/client_ws.hpp"
#include "../bdxws/encrypt_helper.h"
#include <rapidjson/document.h>
#include <string>
#include <stdarg.h>
#include <windows.h>
#include "./json.hpp"
#include <chrono>
#pragma warning(disable:4996)
using namespace std;
std::string addr;
using json = nlohmann::json;
using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;
std::shared_ptr<WsClient::Connection> scon = nullptr;
short counts = -1;
string pw;
namespace config {
	string wspasswdbase = "passwd";
	enum encrypt_type {
		none = 0,
		aes_cbc_pck7padding = 1,
	};
	int encrypt_mode = config::encrypt_type::none;
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

/// <summary>
/// Encrypt the packet using the key and method that config provide
/// </summary>
/// <param name="str">the packet to send</param>
/// <param name="connection">the ws connection ptr</param>
inline void encrypt_send(const string& str, std::shared_ptr<WsClient::Connection> connection) {
	if (config::encrypt_mode == config::encrypt_type::aes_cbc_pck7padding) {

		json j =
		{
			{"type", "encrypted"},
			{
				"params", {
					{"mode", "aes_cbc_pck7padding"},
					{"raw", base64_aes_cbc_encrypt(str, (unsigned char*)config::wspasswdbase.substr(0, 16).c_str(), (unsigned char*)config::wspasswdbase.substr(16, 16).c_str())}
				}
			}
		};
		connection->send(j.dump());
		return;
	}
	if (config::encrypt_mode == config::encrypt_type::none) {
		connection->send(str);
		return;
	}

}

namespace ClientMsgHandle {

	void joinleft(json in_json, int mode) {
		if (mode == 1) {
			cout << "Player:" << in_json["sender"] << "(withXuid:" << in_json["xuid"] << ")(IPAddr:" << in_json["ip"] << ")Joined the Server" << endl;
		}
		if (mode == 0) {
			cout << "Player:" << in_json["sender"] << "(withXuid:" << in_json["xuid"] << ")Left the Server" << endl;
		}
	}
	void chat(json in_json) {
		cout << "[" << in_json["sender"] << "] " << in_json["text"] << endl;
	}
	void cmdfb(json in_json) {
		cout << "CmdRst> " << in_json["result"] << endl;
	}
	void action_switch(json in_json, shared_ptr<WsClient::Connection> connection) {
		//cout << "PackJson > " << in_json.dump(4) << endl;
		if (in_json["type"].is_string()) {
			if (in_json["type"] != "pack") {
				throw string("Error Packet Type");
				Sleep(1000);
				connection->send_close(0);
			}
			else {
				if (in_json["cause"].is_string()) {
					if (in_json["cause"] == "join") {
						ClientMsgHandle::joinleft(in_json["params"], 0);
						return;
					}
					if (in_json["cause"] == "left") {
						ClientMsgHandle::joinleft(in_json["params"], 1);
						return;
					}
					if (in_json["cause"] == "chat") {
						ClientMsgHandle::chat(in_json["params"]);
						return;
					}
					if (in_json["cause"] == "mobdie") {
						cout << "MobDie: " << in_json["params"].dump();
						return;
					}
					if (in_json["cause"] == "runcmdfeedback") {
						ClientMsgHandle::cmdfb(in_json["params"]);
						return;
					}
					throw string("JsonParseError [params][params] No Such Action");

				}
				else {
					throw string("JsonParseError [action] Not Found or Not a object");
				}
			}
		}
		else {
			throw string("JsonParseError [type] Not Found or Not a object");
		}
	}
}
void wsclient() {
	WsClient client(addr);
	client.on_message = [](shared_ptr<WsClient::Connection> connection, shared_ptr<WsClient::InMessage> in_message) {
		string msg = in_message->string();
		cout << "FromServer: " + in_message->string() << endl;
		try {

			json in_json = json::parse(msg);


			if (in_json.is_object()) {
				if (in_json["type"].is_string()) {
					if (in_json["type"] == "encrypted") {
						if (in_json["params"].is_object()) {
							//handle switch(action) 

							if (in_json["params"]["mode"].is_string()) {

								if (in_json["params"]["mode"] == "aes_cbc_pck7padding") {

									if (in_json["params"]["raw"].is_string()) {
										string str = base64_aes_cbc_decrypt(in_json["params"]["raw"], (unsigned char*)config::wspasswdbase.substr(0, 16).c_str(), (unsigned char*)config::wspasswdbase.substr(16, 16).c_str());
										json params_json = json::parse(str);
										if (params_json.is_object())
											ClientMsgHandle::action_switch(params_json, connection);
									}
									else {

										throw string("JsonParseError> Require ObjectType Raw Json with at least one member");
									}



								}
								else {
									throw string("DecodeError> Encrypt mode Not Support");
								}
							}
						}
						else {
							throw string("JsonParseError> [params] Not Found or Not a object");
						}
					}
					else {
						if (in_json["type"] == "pack") {
							ClientMsgHandle::action_switch(in_json, connection);
						}
						else {
							throw string("Unknow Packet Type");
						}
					}





				}
				//json_parse type↓
				else {
					throw string("JsonParseError [type] Not Found or Not a string");
				}
			}
			else {
				throw string("JsonParseError> Require ObjectType Json with at least one member");
			}



		}

		catch (string exp) {
			cout << "Expection " << exp << endl;
		}
		catch (json::parse_error& e) {
			cout << "Expection " << e.what() << endl;
		}
		catch (json::type_error& e) {
			cout << "Expection " << e.what() << endl;
		}
		catch (...) {
			cout << "Unknow Expection" << endl;
		}
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


int main(int argc, char* args[]) {
	SetConsoleTitleA("BDXWebsocket TestClient");
	system("chcp  65001");
	cout << "__          __  _     _____            _        _    _____ _ _            _" << endl;
	cout << "\\ \\        / / | |   / ____|          | |      | |  / ____| (_)          | |" << endl;
	cout << " \\ \\  /\\  / /__| |__| (___   ___   ___| | _____| |_| |    | |_  ___ _ __ | |_" << endl;
	cout << "  \\ \\/  \\/ / _ \\ '_ \\\\___ \\ / _ \\ / __| |/ / _ \\ __| |    | | |/ _ \\ '_ \\| __|" << endl;
	cout << "   \\  /\\  /  __/ |_) |___) | (_) | (__|   <  __/ |_| |____| | |  __/ | | | |_" << endl;
	cout << "    \\/  \\/ \\___|_.__/_____/ \\___/ \\___|_|\\_\\___|\\__|\\_____|_|_|\\___|_| |_|\\__|" << endl;
	string title = "BDXWebsocket TestClient";
	SetConsoleTitleA(title.c_str());
	std::cout << "ServerAddress: ws://";
	std::getline(cin, addr);
	title = title + addr;
	SetConsoleTitleA(title.c_str());
	thread t(wsclient);
	t.detach();
	Sleep(100);

	std::cout << "Passwd: ";
	string passwd;
	std::getline(cin, passwd);
	std::cin.clear();
	if (passwd == "")
		config::encrypt_mode = config::encrypt_type::none;
	else {
		config::encrypt_mode = config::encrypt_type::aes_cbc_pck7padding;
		config::wspasswdbase = MD5(passwd);
		cout << "Passwd " << config::wspasswdbase << endl;
	}
	title = title + " PW:" + passwd;
	SetConsoleTitleA(title.c_str());
	pw = md5(passwd);
	int c = 0;
	while (scon == nullptr) {
		cout << "Connecting......" << endl;
		Sleep(10000);
		c++;
		if (c > 15) {
			cout << "ConnectFailed" << endl;
			exit(1);
		}
	}
	while (scon != nullptr) {
		string cmd;
		std::getline(cin, cmd);
		std::cin.clear();
		/*

		{
			"is_encrypt":true,
			"action": "runcmdrequest",
			"params": {
				"cmd": "kick WangYneos nmsl",
				"id": 0
			}
		}

		*/
		json runcmdfeedback =
		{
			{"type", "pack"},
			{"action","runcmdrequest"},
			{
				"params", {
					{"cmd", cmd},
					{"id", "114514"},
				}
			}
		};
		encrypt_send(runcmdfeedback.dump(), scon);
	}
	cout << "Conection Closed!!!!!" << endl;
	Sleep(10000000);
	return 0;
}
