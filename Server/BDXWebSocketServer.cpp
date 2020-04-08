#include <api/Loader.h>
#include <string>
#include <iostream>
#include <JsonLoader.h>
#include <time.h>
#include <chrono>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include "../tesws/client_ws.hpp"
#include "../tesws/server_ws.hpp"
#include <future>
#include <openssl/md5.h>
#include <rapidjson/document.h>
#include <api\types\helper.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

//#include <json/value.h>

#include<lbpch.h>
#include<stl\KVDB.h>
#include<api\myPacket.h>
#include<mcapi/Player.h>
#include<stl\varint.h>
#include<api\xuidreg\xuidreg.h>
#include<mcapi\Certificate.h>
#include<api\types\helper.h>
#include "mcswst.h"
//disable fucking  error when use sprintf(sprintf_t)
#pragma warning(disable:4996)
using namespace std;

//useful vars
using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;
using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;
shared_ptr<WsServer::Connection> now_connection = nullptr;
unordered_set<shared_ptr<WsServer::Connection>> all_connection;
bool ws_on_cmd;
bool ws_not_inited = true;
int ws_connections_count = 0;
bool passwdmatch = false;

//config var
int wsport = 8000;
string endpoint = "^/mc/?$";
string logfilec = "log/behaviorlog";
bool enablews = true;
string wspasswdbase = "passwd";
string wspasswd, wspasswd1r;
//load config to var
void loadconf() {
	try {
		ConfigJReader jr("config/websocket.json");
		jr.bind("wsport", wsport);
		jr.bind("enablews", enablews);
		jr.bind("wspasswd", wspasswdbase);
		jr.bind("endpoint", endpoint);
	}
	catch (string e) {
		printf("JSON ERROR %s\n", e.c_str());
		exit(1);
	}
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
string gettimen() {
	auto timet = chrono::system_clock::to_time_t(chrono::system_clock::now());
	tm time;
	char buf[3] = { 0 };
	localtime_s(&time, &timet);
	string str(to_string((time.tm_year + 1900)));
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
string gettime1r() {
	auto timet = chrono::system_clock::to_time_t(chrono::system_clock::now());
	tm time;
	char buf[3] = { 0 };
	localtime_s(&time, &timet);
	string str(to_string((time.tm_year + 1900)));
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
string MD5(const string& src) {
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
bool auth(string passwdm5) {
	wspasswd = MD5(wspasswdbase + gettimen());
	wspasswd1r = MD5(wspasswdbase + gettime1r());
	if (passwdm5 == wspasswd || passwdm5 == wspasswd1r)
		return true;
	else
		return false;
}
void wsinitmsg() {
	cout << "WS Port " << wsport << endl;
	cout << "BasePW  " << wspasswdbase << endl;
	cout << "BaseMD5 " << MD5(wspasswd) << endl;
}
void ws_send_all(string text) {
	if (enablews) {
		if (ws_connections_count != 0) {
			for (auto& a_connection : all_connection) {
				a_connection->send(text);
			}
		}
		else {
			printf("WebSocket Not Connected\n");

		}
	}
}
void ws_send_now_connection(string text) {
	if (enablews) {
		if (ws_connections_count != 0) {
			now_connection->send(text);
		}
		else {
			printf("WebSocket Not Connected\n");
		}

	}
}
string makemsg(string type1, string type1m, string type2, string type2m, string type3, string type3m) {
	rapidjson::StringBuffer msg;
	rapidjson::Writer<rapidjson::StringBuffer> writer(msg);
	writer.StartObject();

	writer.Key(type1.c_str());
	writer.String(type1m.c_str());

	writer.Key(type2.c_str());
	writer.String(type2m.c_str());

	writer.Key(type3.c_str());
	writer.String(type3m.c_str());

	writer.EndObject();

	return msg.GetString();
}
void handlecmd(string operate,bool authsuccess,string cmd) {
	if (operate == "runcmd") {
		if (authsuccess) {
			ws_on_cmd = true;
			BDX::runcmd(cmd);
		}
		else
			ws_send_all(makemsg("operate", operate, "onError", "Auth", "text", "Password Not Match"));


	}
	else if (operate == "runcmdas" && authsuccess) {

	}
	else {
		ws_send_all(makemsg("operate", operate, "onError", "finding cmd", "CMD", "not found"));
	}
}
string op, passwd, cmd;
void ws() {
	WsServer server;
	server.config.port = wsport;
	auto& echo = server.endpoint[endpoint];
	echo.on_message = [&server](shared_ptr<WsServer::Connection> connection, shared_ptr<WsServer::InMessage> in_message) {
		string in_msg = in_message->string();
		now_connection = connection;

		cout << "WebSocket Message Recived: \n\"" << in_msg << "\"\nfrom " << connection.get() << endl;
		//get json
		rapidjson::Document document;
		document.Parse(in_msg.c_str());
		rapidjson::Value::ConstMemberIterator iter = document.FindMember("op");
		if (iter != document.MemberEnd()) {
			op = iter->value.GetString();
			cout << "WSOP : " << op << endl;
		}
		rapidjson::Value::ConstMemberIterator iter2 = document.FindMember("passwd");
		if (iter != document.MemberEnd()) {
			passwd = iter2->value.GetString();
			//cout << "PasswdIn: " << passwd;
			passwdmatch = auth(passwd);
			if (passwdmatch) {
				passwdmatch = true;
				cout << "PasswdMatch" << endl;
			}
			else {
				passwdmatch = false;
				cout << "PasswdNotMatch" << endl;
			}

		}
		rapidjson::Value::ConstMemberIterator iter3 = document.FindMember("cmd");
		if (iter != document.MemberEnd()) {
			cmd = iter3->value.GetString();
			cout << "WSCommand : " << cmd << endl;
		}
		//handle command
		handlecmd(op, passwdmatch, cmd);
	};

	echo.on_open = [&server](shared_ptr<WsServer::Connection> connection) {
		cout << "WS: New connection " << connection.get();
		all_connection = server.get_connections();
		ws_connections_count++;
		cout << " ClientCounts " << ws_connections_count << endl;
	};

	echo.on_close = [&server](shared_ptr<WsServer::Connection> connection, int status, const string& /*reason*/) {
		cout << "WS: Closed connection " << connection.get() << " with status code " << status;
		all_connection = server.get_connections();
		ws_connections_count--;
		cout << " ClientCounts " << ws_connections_count << endl;
	};
	echo.on_handshake = [](shared_ptr<WsServer::Connection> /*connection*/, SimpleWeb::CaseInsensitiveMultimap& /*response_header*/) {
		return SimpleWeb::StatusCode::information_switching_protocols; // Html handshake,then Upgrade to websocket
	};
	echo.on_error = [&server](shared_ptr<WsServer::Connection> connection, const SimpleWeb::error_code& ec) {
		cout << "Server: Error in connection " << connection.get() << ". "
			<< "Error: " << ec << ", error message: " << ec.message();
		all_connection = server.get_connections();
		ws_connections_count--;
		cout << " ClientCounts " << ws_connections_count << endl;
	};
	promise<unsigned short> server_port;
	thread server_thread([&server, &server_port]() {
		// Start server
		server.start([&server_port](unsigned short port) {
			server_port.set_value(port);
			wsinitmsg();
		});
	});;
	server_thread.join();
}

void reglist() {
	addListener([](PlayerChatEvent& event) {
		ws_send_all(makemsg("operate", string("onmsg" ), "target", event.getPlayer().getName(), "text", event.getChat()));
	});
	addListener([](PlayerJoinEvent& event) {
		ws_send_all(makemsg("operate", string("onjoin"), "target", event.getPlayer().getName(), "text", string("Joined server")));
	});
	addListener([](PlayerLeftEvent& event) {
		ws_send_all(makemsg("operate", string("onleft"), "target", event.getPlayer().getName(), "text", string("Lefted server")));
	});
	addListener([](PlayerCMDEvent& event) {
		ws_send_all(makemsg("operate", string("onCMD" ), "target", event.getPlayer().getName(), "CMD",  event.getCMD()));
	});
}

void wst_entry() {
	loadconf();
	reglist();
	if (ws_not_inited) {
		thread ws_thread(ws);
		ws_thread.detach();
		ws_not_inited = false;
	}
}

bool Get_cmd = false;
LIGHTBASE_API std::vector<string> getPlayerList();
THook(void, "?addMessage@CommandOutput@@AEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEBV?$vector@VCommandOutputParameter@@V?$allocator@VCommandOutputParameter@@@std@@@3@W4CommandOutputMessageType@@@Z",
	void* _this, string const& commandfeedback, void* a2, __int64 a3, int a4) {
	if (ws_on_cmd) {
		Get_cmd = true;
		ws_on_cmd = false;
	}
	/*if (ws_on_cmd) {
		cout << "WS Command Result: " << commandfeedback << endl;
		ws_send_now_connection(makemsg("operate", op, "Auth", string("PasswdMatch"), "feedback", commandfeedback));
		ws_on_cmd = false;
	}*/

	original(_this, commandfeedback, a2, a3, a4);
}
THook(void, "??$_Insert_string@DU?$char_traits@D@std@@_K@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@QEBD_K@Z", unsigned long long a1, char* output, unsigned long long a3) {
	if (Get_cmd) {
		cout << "WS Command Result Got" << endl;
		ws_send_now_connection(makemsg("operate", op, "Auth", string("PasswdMatch"), "feedback", output));
		Get_cmd = false;
	}
	original(a1,output,a3);

}

/*
THook(void, "?send@CommandOutputSender@@UEAAXAEBVCommandOrigin@@AEBVCommandOutput@@@Z", void* _this, const CommandOrigin* a2, const CommandOutput* a3) {
	cout << "a" << endl;
	std::string fuck;
	const string Memory;
	const struct CommandOutput* v3=a3;
	const CommandOutput* v9 = v3 + 2;
	const CommandOutput* v10 = v3 + 3;
	vector<string> str(2);
	str[0] = v9 + 8;
	str[1] = v9 + 40;
	while (v9 != v10) {
		fuck = SymCall("?get@I18n@@SA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEBV23@AEBV?$vector@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@V?$allocator@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@@3@@Z",std::string,const string&, const& vector<string>)(&Memory, str);
	}//basic_ostream<char,std::char_traits<char>>
	
	original(_this, a2, a3);
	cout << "b" << endl;
}/*
THook(std::string, "?get@I18n@@SA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEBV23@@Z", void* _this, std::string const& a1) {
	cout << "bi" << endl;
	string val = original(_this, "accessibility.button.close");
	cout << val << endl;
	cout << "bi" << endl;
	return val;

}
THook(void, "?handleCommandOutputCallback@CommandOrigin@@UEBAX$$QEAVValue@Json@@@Z", class Json::Value&  a1) {
	if (faq) {
		cout << a1.toStyledString() << endl;
		faq = false;
	}
	original(a1);
}*/


