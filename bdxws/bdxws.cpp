#include <api/Loader.h>
#include <string>
#include <iostream>
#include <JsonLoader.h>
#include <time.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include "../sws/server_ws.hpp"
#include <openssl/md5.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <lbpch.h>
#include <stl\KVDB.h>
#include <stl\varint.h>
#include <mcapi/Player.h>
#include <mcapi\Certificate.h>
#include <api\xuidreg\xuidreg.h>
#include <api\myPacket.h>
#include <api\types\helper.h>
#include <api\command\commands.h>
#include <mcapi/Actor.h>
#include <mcapi/Level.h>


 //disable fucking  error when use sprintf(sprintf_t)
#pragma warning(disable:4996)
using namespace std;
//useful vars
using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;
//using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;
shared_ptr<WsServer::Connection> now_connection = nullptr;
unordered_set<shared_ptr<WsServer::Connection>> all_connection;
bool ws_on_cmd;
bool ws_not_inited = true;
int ws_connections_count = 0;
string clientlist = "";
//config var
int wsport = 8080;
string endpoint = "^/mc/?$";
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
//time without sec
inline string gettimen() {
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
//time 1min ago without sec
inline string gettime1r() {
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
	u2a(buf, (time.tm_min - 1));
	str += buf;
	return str;
}
//get realtime with sec
inline std::string gettime() {
	auto timet = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	tm time;
	char buf[3] = { 0 };
	localtime_s(&time, &timet);
	std::string str(std::to_string((time.tm_year + 1900)));
	str += "-";
	u2a(buf, time.tm_mon + 1);
	str += buf; str += "-";
	u2a(buf, time.tm_mday);
	str += buf; str += " ";
	u2a(buf, time.tm_hour);
	str += buf; str += ":";
	u2a(buf, time.tm_min);
	str += buf; str += ":";
	u2a(buf, time.tm_sec);
	str += buf;
	return str;
}
//calcu MD5
inline string MD5(const string& src) {
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
//auth passwd
inline bool auth(string passwdm5) {
	wspasswd = MD5(wspasswdbase + gettimen());
	wspasswd1r = MD5(wspasswdbase + gettime1r());
	//cout << wspasswd << "  " << wspasswd1r << endl;
	if (passwdm5 == wspasswd || passwdm5 == wspasswd1r)
		return true;
	else
		return false;
}
inline void wsinitmsg() {
	cout << "__          __  _     _____            _        _" << endl;
	cout << "\\ \\        / / | |   / ____|          | |      | |" << endl;
	cout << " \\ \\  /\\  / /__| |__| (___   ___   ___| | _____| |_" << endl;
	cout << "  \\ \\/  \\/ / _ \\ '_ \\\\___ \\ / _ \\ / __| |/ / _ \\ __|" << endl;
	cout << "   \\  /\\  /  __/ |_) |___) | (_) | (__|   <  __/ |_" << endl;
	cout << "    \\/  \\/ \\___|_.__/_____/ \\___/ \\___|_|\\_\\___|\\__|" << endl;
	cout << "WebsocketServer Loaded , By WangYneos" << endl;
	cout << "[" << gettime() << " Init][WSI] [WS Port    ] " << wsport << endl;
	cout << "[" << gettime() << " Init][WSI] [Base Passwd] " << wspasswdbase << endl;
	cout << "[" << gettime() << " Init][WSI] [End Point  ] " << endpoint << endl;
}
//send to all client
inline void ws_send_all(string text) {
	if (enablews) {
		if (ws_connections_count != 0) {
			for (auto& a_connection : all_connection) {
				a_connection->send(text);
			}
		}
		else {
			cout << "[" << gettime() << " Error][WSE] [NoClient] " << "WebSocket Not Connected" << endl;

		}
	}
}
inline bool ws_disconn_all() {
	if (enablews) {
		if (ws_connections_count != 0) {
			for (auto& a_connection : all_connection) {
				a_connection->send_close(0);
				return true;
			}
		}
		else {
			cout << "[" << gettime() << " Error][WSE] [NoClient] " << "WebSocket Not Connected" << endl;
			return false;
		}
	}
}
//sent to latest client
inline void ws_send_now_connection(string text) {
	if (enablews) {
		if (ws_connections_count != 0) {
			now_connection->send(text);
		}
		else {
			cout << "[" << gettime() << " Error][WSE] [NoClient] " << "WebSocket Not Connected" << endl;
		}

	}
}
//make a message to client
inline string makemsg(pair<string, string> msg1) {
	rapidjson::StringBuffer mmsg;
	rapidjson::Writer<rapidjson::StringBuffer> writer(mmsg);
	writer.StartObject();
	writer.Key(msg1.first.c_str());
	writer.String(msg1.second.c_str());
	writer.EndObject();
	return mmsg.GetString();

}
inline string makemsg(pair<string, string> msg1, pair<string, string> msg2) {
	rapidjson::StringBuffer mmsg;
	rapidjson::Writer<rapidjson::StringBuffer> writer(mmsg);
	writer.StartObject();

	writer.Key(msg1.first.c_str());
	writer.String(msg1.second.c_str());
	writer.Key(msg2.first.c_str());
	writer.String(msg2.second.c_str());

	writer.EndObject();

	return mmsg.GetString();

}
inline string makemsg(pair<string, string> msg1, pair<string, string> msg2, pair<string, string> msg3) {
	rapidjson::StringBuffer mmsg;
	rapidjson::Writer<rapidjson::StringBuffer> writer(mmsg);
	writer.StartObject();

	writer.Key(msg1.first.c_str());
	writer.String(msg1.second.c_str());
	writer.Key(msg2.first.c_str());
	writer.String(msg2.second.c_str());
	writer.Key(msg3.first.c_str());
	writer.String(msg3.second.c_str());

	writer.EndObject();

	return mmsg.GetString();

}
inline string makemsg(pair<string, string> msg1, pair<string, string> msg2, pair<string, string> msg3, pair<string, string> msg4) {
	rapidjson::StringBuffer mmsg;
	rapidjson::Writer<rapidjson::StringBuffer> writer(mmsg);
	writer.StartObject();

	writer.Key(msg1.first.c_str());
	writer.String(msg1.second.c_str());
	writer.Key(msg2.first.c_str());
	writer.String(msg2.second.c_str());
	writer.Key(msg3.first.c_str());
	writer.String(msg3.second.c_str());
	writer.Key(msg4.first.c_str());
	writer.String(msg4.second.c_str());

	writer.EndObject();
	return mmsg.GetString();
}
inline string makemsg(pair<string, string> msg1, pair<string, string> msg2, pair<string, string> msg3, pair<string, string> msg4, pair<string, string> msg5) {
	rapidjson::StringBuffer mmsg;
	rapidjson::Writer<rapidjson::StringBuffer> writer(mmsg);
	writer.StartObject();

	writer.Key(msg1.first.c_str());
	writer.String(msg1.second.c_str());
	writer.Key(msg2.first.c_str());
	writer.String(msg2.second.c_str());
	writer.Key(msg3.first.c_str());
	writer.String(msg3.second.c_str());
	writer.Key(msg4.first.c_str());
	writer.String(msg4.second.c_str());
	writer.Key(msg5.first.c_str());
	writer.String(msg5.second.c_str());

	writer.EndObject();
	return mmsg.GetString();
}
//handle when msg in

namespace handle {
	void runcmd(bool authsuccess, string msg) {
		string cmd;
		if (authsuccess) {
			rapidjson::Document document;
			document.Parse(msg.c_str());
			rapidjson::Value::ConstMemberIterator in = document.FindMember("cmd");
			if (in != document.MemberEnd()) {
				cmd = in->value.GetString();
			}
			string ret = BDX::runcmdEx(cmd).second;
			ws_send_now_connection(makemsg({ "operate", "runcmd" }, { "Auth", "Success" }, { "text", ret }));
			cout << "[" << gettime() << " INFO][WSM] [" << "Running Cmd" << "] " << "[Auth]Success " << "[CMD]" << cmd << " " << endl;
		}
		else {
			ws_send_now_connection(makemsg({ "operate", "runcmd" }, { "Auth", "Failed" }, { "text", "Password Not Match" }));
			cout << "[" << gettime() << " INFO][WSM] [" << "Running Cmd" << "] " << "[Auth]Fail " << "[CMD]NotExcuted" << endl;
		}
	}


	void runcmdas(bool authsuccess, string msg) {
		string cmd, target;
		if (authsuccess) {
			rapidjson::Document document;
			document.Parse(msg.c_str());
			rapidjson::Value::ConstMemberIterator in = document.FindMember("cmd");
			if (in != document.MemberEnd()) {
				cmd = in->value.GetString();
			}
			rapidjson::Value::ConstMemberIterator in2 = document.FindMember("target");
			if (in2 != document.MemberEnd()) {
				target = in2->value.GetString();
			}
			auto ply = LocateS<WLevel>()->getPlayer(target);
			if (ply.Set()) {
				ply.value().runcmd(cmd);
			}
			if (ply.Set()) {
				ws_send_now_connection(makemsg({ "operate", "runcmdas" }, { "Auth", "Success" }, { "text", "Success" }));
				cout << "[" << gettime() << " INFO][WSM] [" << "Running Cmd As: " << target << "] " << "[Auth]Success " << "[CMD]" << cmd << endl;
			}
			else {
				ws_send_now_connection(makemsg({ "operate", "runcmdas" }, { "Auth", "Success" }, { "text", "Failed" }));
				cout << "[" << gettime() << " INFO][WSM] [" << "Running Cmd" << "] " << "[Auth]Success " << "[CMD]NotSuchPlayer" << endl;
			}
		}
		else {
			cout << "[" << gettime() << " INFO][WSM] [" << "Running Cmd" << "] " << "[Auth]Fail " << "[CMD]NotExcuted" << endl;
			ws_send_now_connection(makemsg({ "operate", "runcmdas" }, { "Auth", "Failed" }, { "text", "Password Not Match" }));
		}
	}
};
string& repall(string& str, const string& olds, const string& news)
{
	string::size_type pos = 0;
	while ((pos = str.find(olds)) != string::npos)
	{
		str = str.replace(str.find(olds), olds.length(), news);
	}
	return str;
}
inline void fw(std::string filen, std::string instr) {
	std::ofstream outfile;
	outfile.open(filen, std::ios::app);
	outfile << instr << std::endl;
	outfile.close();
}
void ws() {
	WsServer server;
	server.config.port = wsport;
	auto& mc = server.endpoint[endpoint];
	mc.on_message = [&server](shared_ptr<WsServer::Connection> connection, shared_ptr<WsServer::InMessage> in_message) {
		string op, passwd, cmd;
		bool passwdmatch = false;
		string in_msg = in_message->string();

		string log = "\"" + in_msg + "\",\"" + connection->remote_endpoint().address().to_string() + "\"";
		fw("wslog.csv", log);
		now_connection = connection;
		cout << "[" << gettime() << " INFO][WSM] [" << "RecivedMsg" << "] " /*<< in_msg << " from " */ << connection->remote_endpoint().address().to_string() + ":" + to_string(connection->remote_endpoint().port()) << endl;
		//get json
		rapidjson::Document document;
		document.Parse(in_msg.c_str());
		rapidjson::Value::ConstMemberIterator iter = document.FindMember("operate");
		if (iter != document.MemberEnd()) {
			op = iter->value.GetString();
		}
		rapidjson::Value::ConstMemberIterator iter2 = document.FindMember("passwd");
		if (iter != document.MemberEnd()) {
			passwd = iter2->value.GetString();
			//cout << "PasswdIn: " << passwd;
			passwdmatch = auth(passwd);
			if (passwdmatch) {
				passwdmatch = true;
			}
			else {
				passwdmatch = false;
			}

		}

		//handle command
		if (!op.empty()) {
			if (op == "runcmd")
				handle::runcmd(passwdmatch, in_msg);
			if (op == "runcmdas")
				handle::runcmdas(passwdmatch, in_msg);
		}
		else {
			ws_send_now_connection(makemsg({ "operate", "unknow" }, { "Auth", "unknow" }, { "text", "Error Read Json>Operate" }));
			cout << "[" << gettime() + " Error][WSE] [Error Msg] Error When Read Json(Operate) form \"" << connection->remote_endpoint().address().to_string() + ":" + to_string(connection->remote_endpoint().port()) << "\"" << endl;
		}
	};

	mc.on_open = [&server](shared_ptr<WsServer::Connection> connection) {
		all_connection = server.get_connections();
		ws_connections_count++;
		cout << "[" << gettime() << " INFO][WSO] [NewConnection] " << connection->remote_endpoint().address().to_string() + ":" + to_string(connection->remote_endpoint().port()) << " Clients " << ws_connections_count << endl;
		string log = "\"NewConnection\",\"" + connection->remote_endpoint().address().to_string() + ":" + to_string(connection->remote_endpoint().port()) + "\"";
		fw("wslog.csv", log);
		clientlist += "\"" + connection->remote_endpoint().address().to_string() + ":" + to_string(connection->remote_endpoint().port()) + "\"\n";
	};

	mc.on_close = [&server](shared_ptr<WsServer::Connection> connection, int status, const string& /*reason*/) {
		all_connection = server.get_connections();
		if (ws_connections_count != 0) {
			ws_connections_count--;
			string str = "\"" + connection->remote_endpoint().address().to_string() + ":" + to_string(connection->remote_endpoint().port()) + "\"\n";
			clientlist = clientlist.replace(clientlist.find(str), str.length(), "");
		}
		cout << "[" << gettime() << " INFO][WSC] [LostConnection] " << connection->remote_endpoint().address().to_string() + ":" + to_string(connection->remote_endpoint().port()) << "Status " << status << " Clients " << ws_connections_count << endl;
		string log = "\"LostConnection\",\"" + connection->remote_endpoint().address().to_string() + ":" + to_string(connection->remote_endpoint().port()) + "\"";
		fw("wslog.csv", log);
	};
	mc.on_handshake = [](shared_ptr<WsServer::Connection> /*connection*/, SimpleWeb::CaseInsensitiveMultimap& /*response_header*/) {
		return SimpleWeb::StatusCode::information_switching_protocols; // Html handshake,then Upgrade to websocket
	};
	mc.on_error = [&server](shared_ptr<WsServer::Connection> connection, const SimpleWeb::error_code& ec) {
		all_connection = server.get_connections();
		if (ws_connections_count != 0) {
			ws_connections_count--;
			string str = "\"" + connection->remote_endpoint().address().to_string() + ":" + to_string(connection->remote_endpoint().port()) + "\"\n";
			clientlist = clientlist.replace(clientlist.find(str), str.length(), "");
		}
		cout << "[" << gettime() << " Error][WSE] [ErrorConnection] " << connection->remote_endpoint().address().to_string() + ":" + to_string(connection->remote_endpoint().port()) << " Clients " << ws_connections_count << endl;
		string log = "\"LostConnection\",\"" + connection->remote_endpoint().address().to_string() + ":" + to_string(connection->remote_endpoint().port()) + "\"";
		fw("wslog.csv", log);
		
	};
	promise<unsigned short> server_port;
	thread server_thread([&server, &server_port]() {
		// Start server
		server.start([&server_port](unsigned short port) {
			server_port.set_value(port);
			cout << "[" << gettime() << " INFO][WSL] [ErrorConnection] Websocket Server Started" <<endl;

		});
	});;
	server_thread.join();
}

void reglist() {
	addListener([](PlayerChatEvent& event) {
		ws_send_all(makemsg({ "operate", "onmsg" }, { "target", event.getPlayer().getName() }, { "text", event.getChat() }));
	});
	addListener([](PlayerJoinEvent& event) {
		ws_send_all(makemsg({ "operate", "onjoin" }, { "target", event.getPlayer().getName() }, { "text", event.getPlayer().getIP() }));
	});
	addListener([](PlayerLeftEvent& event) {
		ws_send_all(makemsg({ "operate", "onleft" }, { "target", event.getPlayer().getName() }, { "text", "Lefted server" }));
	});
	addListener([](PlayerCMDEvent& event) {
		ws_send_all(makemsg({ "operate", "oncmd" }, { "target", event.getPlayer().getName() }, { "text",  event.getCMD() }));
	});
}
THook(void,
	"?die@Player@@UEAAXAEBVActorDamageSource@@@Z",
	Player* _this, ActorDamageSource* a2) {
	if (enablews) {
		string playername = _this->getNameTag();
		ActorUniqueID src_id = a2->getEntityUniqueID();
		Actor* src = LocateS<ServerLevel>()->fetchEntity(src_id, false);
		if (src) {
			string src_type_name = SymCall("?getEntityName@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEBVActor@@@Z", string, Actor*)(src);
			ws_send_all(makemsg({ "operate", "onplayerdie" }, { "target", playername }, { "source",src_type_name }));
		}else {
			ws_send_all(makemsg({ "operate", "onplayerdie" }, { "target", playername }, { "source","unknow" }));
		}
	}
	original(_this, a2);
}
bool wss(CommandOrigin const& ori, CommandOutput& outp, string& op) {
	if (op == "list") {
		outp.addMessage(string("List Of " + to_string(ws_connections_count) + " WS Clients"));
		outp.addMessage(clientlist);
	}
	else if (op == "disall") {
		if (ws_disconn_all())
			outp.addMessage("Succfully disconnect all connection");
		else
			outp.addMessage("No Connected client");
	}
	else
		outp.error("no such command");
}
void startws() {
	addListener([](ServerStartedEvent& event) {
		thread ws_thread(ws);
		ws_thread.detach();
		ws_not_inited = false;

	});
}
void wst_entry() {
	loadconf();
	if (enablews) {
		startws();
		if (ws_not_inited) {
			wsinitmsg();
		}
		thread listener_thread(reglist);
		listener_thread.detach();
		addListener([](RegisterCommandEvent&) {
			MakeCommand("ws", "WebsocketServer", 1);
			CmdOverload(ws, wss, "operate");
		});
	}
}