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

//config var
int wsport = 8080;
string endpoint = "^/mc/?$";
bool enablews = true;
string wspasswdbase = "passwd";
inline bool logmsg = false;

vector<pair<string, string>> wslist;
void setdesp(string server, string des) {
	wslist.push_back({ server,des });
}
pair<int, string> getdesp(string server) {
	string retdes = "NULL";
	int retpoint = -1;
	for (int a = 0; a < wslist.size(); a++) {
		if (wslist[a].first == server) {
			retdes = wslist[a].second;
			retpoint = a;
		}
	}
	return { retpoint,retdes };
}
void rmdes(string server) {
	pair<int, string> wsserver = getdesp(server);
	if (wsserver.first != -1)
		wslist.erase(wslist.begin() + wsserver.first);
}
//load config to var
void loadconf() {
	try {
		ConfigJReader jr("config/websocket.json");
		jr.bind("wsport", wsport);
		jr.bind("enablews", enablews);
		jr.bind("wspasswd", wspasswdbase);
		jr.bind("endpoint", endpoint);
		jr.bind("logmsg", logmsg, false);
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
	return in_msg;
}
inline string& repall(string& str, const string& olds, const string& news)
{
	string::size_type pos = 0;
	while ((pos = str.find(olds)) != string::npos)
	{
		str = str.replace(str.find(olds), olds.length(), news);
	}
	return str;
}
inline bool auth(string in_pw, string msg) {
	string auth_msg = repall(msg, in_pw, "");
	string passwdn = wspasswdbase + gettimen() + "@" + auth_msg;
	string passwd1r = wspasswdbase + gettime1r() + "@" + auth_msg;
	//cout << passwdn << endl;
	if (in_pw == MD5(passwdn) || in_pw == MD5(passwd1r))
		return true;
	else
		return false;
}
/*
inline bool auth(string passwdm5) {
	wspasswd = MD5(wspasswdbase + gettimen());
	wspasswd1r = MD5(wspasswdbase + gettime1r());
	//cout << wspasswd << "  " << wspasswd1r << endl;
	if (passwdm5 == wspasswd || passwdm5 == wspasswd1r)
		return true;
	else
		return false;
}*/
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
//make json
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
}
inline int ws_get_all_connection_count() {
	if (enablews) {
		int a = 0;
		for (auto& a_connection : all_connection) {
			a = a + 1;
		}
		return a;
	}
	else {
		return -1;
	}
}
//send to all client
inline void ws_send_all(string text) {
	if (enablews) {
		if (ws_get_all_connection_count() != 0) {
			for (auto& a_connection : all_connection) {
				a_connection->send(text);
			}
		}
		else {
			cout << "[" << gettime() << " Error][WSE] [NoClient] " << "WebSocket Enabled But No Client Connected" << endl;

		}
	}
}
inline bool ws_disconn_all() {
	if (enablews) {
		if (ws_get_all_connection_count() != 0) {
			for (auto& a_connection : all_connection) {
				a_connection->send_close(0);
				return true;
			}
		}
		else {
			cout << "[" << gettime() << " Error][WSE] [NoClient] " << "WebSocket Enabled But No Client Connected" << endl;
			return false;
		}
	}
	return false;
}
//sent to latest client
inline void ws_send_now_connection(string text) {
	if (enablews) {
		if (ws_get_all_connection_count() != 0) {
			now_connection->send(text);
		}
		else {
			cout << "[" << gettime() << " Error][WSE] [NoClient] " << "WebSocket Encbled No Client Connected" << endl;
		}

	}
}
inline vector<shared_ptr<WsServer::Connection>> ws_get_all_vector() {
	vector<shared_ptr<WsServer::Connection>> conns;
	for (auto& a_connection : all_connection) {
		conns.push_back(a_connection);
	}
	return conns;
}
inline string ws_get_all_json() {
	if (enablews) {
		int tmp = -1;
		vector<shared_ptr<WsServer::Connection>> con = ws_get_all_vector();
		rapidjson::StringBuffer mmsg;
		rapidjson::Writer<rapidjson::StringBuffer> writer(mmsg);
		writer.StartObject();
		writer.Key("Count");
		writer.Int(ws_get_all_connection_count());
		for (auto& a_connection : all_connection) {
			tmp++;
			writer.Key(to_string(tmp).c_str());
			writer.String(string(con[tmp]->remote_endpoint().address().to_string() + ":" + to_string(con[tmp]->remote_endpoint().port())).c_str());
		}
		writer.EndObject();
		return mmsg.GetString();
	}
	else {
		return "";
	}
}
inline string ws_get_all_string() {
	if (enablews) {
		vector<shared_ptr<WsServer::Connection>> con = ws_get_all_vector();
		string str = "";
		for (int i = 0; i < con.size(); i++)
		{
			char p[20];
			sprintf(p, "%p", con[i].get());
			str += "[" + to_string(i) + "] " + con[i]->remote_endpoint().address().to_string() + ":" + to_string(con[i]->remote_endpoint().port()) + " \"" + getdesp(p).second + "\"\n";
		}
		return str;
	}
	else {
		return "";
	}
}
inline bool ws_send_id(int id, string text) {
	if (enablews) {
		vector<shared_ptr<WsServer::Connection>> con = ws_get_all_vector();
		if (id < con.size()) {
			con[id]->send(text);
			return true;
		}
		else {
			return false;
		}
	}
	else {
		return false;
	}
}
inline bool ws_dis_id(int id) {
	if (enablews) {
		vector<shared_ptr<WsServer::Connection>> con = ws_get_all_vector();
		if (id < con.size()) {
			con[id]->send_close(1000);
			return true;
		}
		else {
			return false;
		}
	}
	else {
		return false;
	}
}
//handle when msg in

namespace handle {
	inline void runcmd(bool authsuccess, string msg, optional<string> msgid) {
		string cmd;
		if (authsuccess) {
			rapidjson::Document document;
			document.Parse(msg.c_str());
			rapidjson::Value::ConstMemberIterator in = document.FindMember("cmd");
			if (in != document.MemberEnd()) {
				cmd = in->value.GetString();
			}
			string ret = BDX::runcmdEx(cmd).second;
			if (msgid.Set())
				ws_send_now_connection(makejson({ { "operate", "runcmd" }, { "Auth", "Success" }, { "text", ret }, { "msgid",string(msgid.val()) } }));
			else
				ws_send_now_connection(makejson({ { "operate", "runcmd" }, { "Auth", "Success" }, { "text", ret } }));
			cout << "[" << gettime() << " INFO][WSM] [" << "Running Cmd" << "] " << "[Auth]Success " << "[CMD][" << cmd << "] " << endl;
		}
		else {
			if (msgid.Set())
				ws_send_now_connection(makejson({ { "operate", "runcmd" }, { "Auth", "Failed" }, { "text", "Password Not Match" }, { "msgid",string(msgid.val()) } }));
			else
				ws_send_now_connection(makejson({ { "operate", "runcmd" }, { "Auth", "Failed" }, { "text", "Password Not Match" } }));
			cout << "[" << gettime() << " INFO][WSM] [" << "Running Cmd" << "] " << "[Auth]Fail " << "[CMD]NotExcuted" << endl;
		}
	}
	inline void setdespcription(string msg, string server) {
		string desp;
		rapidjson::Document document;
		document.Parse(msg.c_str());
		rapidjson::Value::ConstMemberIterator in = document.FindMember("desp");
		if (in != document.MemberEnd()) {
			desp = in->value.GetString();
		}
		auto wsserver = getdesp(server);
		if (wsserver.first != -1) {
			setdesp(server, desp);
		}
		else {
			rmdes(server);
			setdesp(server, desp);
		}
	}
	/*
	inline void runcmdas(bool authsuccess, string msg,optional<string> msgid) {
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
				ws_send_now_connection(makejson({ "operate", "runcmdas" }, { "Auth", "Success" }, { "text", "Success" }));
				cout << "[" << gettime() << " INFO][WSM] [" << "Running Cmd As: " << target << "] " << "[Auth]Success " << "[CMD]" << cmd << endl;
			}
			else {
				ws_send_now_connection(makejson({ "operate", "runcmdas" }, { "Auth", "Success" }, { "text", "Failed" }));
				cout << "[" << gettime() << " INFO][WSM] [" << "Running Cmd" << "] " << "[Auth]Success " << "[CMD]NotSuchPlayer" << endl;
			}
		}
		else {
			cout << "[" << gettime() << " INFO][WSM] [" << "Running Cmd" << "] " << "[Auth]Fail " << "[CMD]NotExcuted" << endl;
			ws_send_now_connection(makejson({ "operate", "runcmdas" }, { "Auth", "Failed" }, { "text", "Password Not Match" }));
		}
	}*/
};

inline void fw(std::string filen, std::string instr) {
	if (logmsg) {
		std::ofstream outfile;
		outfile.open(filen, std::ios::app);
		outfile << instr << std::endl;
		outfile.close();
	}
}
inline void ws() {
	WsServer server;
	server.config.port = wsport;
	auto& mc = server.endpoint[endpoint];
	mc.on_message = [&server](shared_ptr<WsServer::Connection> connection, shared_ptr<WsServer::InMessage> in_message) {
		string op, passwd, cmd, msgid;
		bool passwdmatch = false;
		string in_msg = in_message->string();
		now_connection = connection;
		//get json
		rapidjson::Document document;
		document.Parse(in_msg.c_str());
		rapidjson::Value::ConstMemberIterator parse = document.FindMember("operate");
		if (parse != document.MemberEnd()) {
			op = parse->value.GetString();
		}
		cout << "[" << gettime() << " INFO][WSM] [" << "RecivedMsg" << "] " /*<< in_msg*/ << " [" << connection->remote_endpoint().address().to_string() + ":" + to_string(connection->remote_endpoint().port()) << "]" << endl;
		rapidjson::Value::ConstMemberIterator parse2 = document.FindMember("passwd");
		if (parse2 != document.MemberEnd()) {
			passwd = parse2->value.GetString();
			//cout << "PasswdIn: " << passwd;
			passwdmatch = auth(passwd, in_msg);
			if (passwdmatch) {
				passwdmatch = true;
			}
			else {
				passwdmatch = false;
			}
		}
		rapidjson::Value::ConstMemberIterator parse3 = document.FindMember("msgid");
		if (parse3 != document.MemberEnd()) {
			msgid = parse3->value.GetString();
		}
		//handle command
		if (!op.empty()) {
			string log = "\"" + gettime() + "\",\"" + "\"" + in_msg + "\",\"" + connection->remote_endpoint().address().to_string() + "\"";
			if (op == "runcmd") {
				fw("wslog.csv", log);
				handle::runcmd(passwdmatch, in_msg, msgid);
			}/*
			if (op == "runcmdas") {
				handle::runcmdas(passwdmatch, in_msg, msgid);
				fw("wslog.csv", log);
			}*/
			if (op == "ping") {
				connection->send(BDX::runcmdEx("ws ping").second);
			}
			if (op == "setdesp") {
				char p[20];
				sprintf(p, "%p", connection.get());
				handle::setdespcription(in_msg, string(p));
			}


		}
		else {
			ws_send_now_connection(makejson({ { "operate", "unknow" }, { "Auth", "unknow" }, { "text", "Error Read Json>Operate" } }));
			cout << "[" << gettime() + " Error][WSE] [Error Msg] Error When Read Json(Operate) form \"" << connection->remote_endpoint().address().to_string() + ":" + to_string(connection->remote_endpoint().port()) << "\"" << endl;
		}
	};

	mc.on_open = [&server](shared_ptr<WsServer::Connection> connection) {
		all_connection = server.get_connections();
		cout << "[" << gettime() << " INFO][WSO] [NewConnection] " << connection->remote_endpoint().address().to_string() + ":" + to_string(connection->remote_endpoint().port()) << " Clients " << ws_get_all_connection_count() << endl;
		string log = "\"NewConnection\",\"" + connection->remote_endpoint().address().to_string() + ":" + to_string(connection->remote_endpoint().port()) + "\"";
	};

	mc.on_close = [&server](shared_ptr<WsServer::Connection> connection, int status, const string& /*reason*/) {
		all_connection = server.get_connections();
		cout << "[" << gettime() << " INFO][WSC] [LostConnection] " << connection->remote_endpoint().address().to_string() + ":" + to_string(connection->remote_endpoint().port()) << " Status " << status << " Clients " << ws_get_all_connection_count() << endl;
		string log = "\"LostConnection\",\"" + connection->remote_endpoint().address().to_string() + ":" + to_string(connection->remote_endpoint().port()) + "\"";
		char p[20];
		sprintf(p, "%p", connection.get());
		rmdes(string(p));
	};
	mc.on_handshake = [](shared_ptr<WsServer::Connection> /*connection*/, SimpleWeb::CaseInsensitiveMultimap& /*response_header*/) {
		return SimpleWeb::StatusCode::information_switching_protocols; // Html handshake,then Upgrade to websocket
	};
	mc.on_error = [&server](shared_ptr<WsServer::Connection> connection, const SimpleWeb::error_code& ec) {
		all_connection = server.get_connections();
		cout << "[" << gettime() << " Error][WSE] [ErrorConnection] " << connection->remote_endpoint().address().to_string() + ":" + to_string(connection->remote_endpoint().port()) << " Clients " << ws_get_all_connection_count() << endl;
		string log = "\"LostConnection\",\"" + connection->remote_endpoint().address().to_string() + ":" + to_string(connection->remote_endpoint().port()) + "\"";
		char p[20];
		sprintf(p, "%p", connection.get());
		rmdes(string(p));
	};
	promise<unsigned short> server_port;
	thread server_thread([&server, &server_port]() {
		// Start server
		server.start([&server_port](unsigned short port) {
			server_port.set_value(port);
			cout << "[" << gettime() << " INFO][WSL] [InitService] Websocket Server Started" << endl;

		});
	});;
	server_thread.join();
}

inline void reglist() {
	addListener([](PlayerChatEvent& event) {
		ws_send_all(makejson({ { "operate", "onmsg" }, { "target", event.getPlayer().getName() }, { "text", event.getChat() } }));
	});
	addListener([](PlayerJoinEvent& event) {
		ws_send_all(makejson({ { "operate", "onjoin" }, { "target", event.getPlayer().getName() }, { "text", event.getPlayer().getIP() } }));
	});
	addListener([](PlayerLeftEvent& event) {
		ws_send_all(makejson({ { "operate", "onleft" }, { "target", event.getPlayer().getName() }, { "text", "Lefted server" } }));
	});
	addListener([](PlayerCMDEvent& event) {
		ws_send_all(makejson({ { "operate", "oncmd" }, { "target", event.getPlayer().getName() }, { "text",  string(event.getCMD())} }));
	});
	addListener([](PlayerChangeDimEvent& event) {
		ws_send_all(makejson({ {"operate","oncdim" }, { "target",event.getPlayer().getName() }, { "from",to_string(event.SrcDim) }, { "to",to_string(event.DstDim) } }));
	});
}

/*
typedef std::pair<string, std::vector<string>> pairsvs;
//; public: virtual struct std::pair<cstring, vector<string>> ActorDamageSource::getDeathMessage(string a1,Actor* a2)const
//std::pair<string,std::vector<string > >
THook(pairsvs,"?getDeathMessage@ActorDamageSource@@UEBA?AU?"
"$pair@V?$basic_string@DU?$char_traits@D@std@"
"@V?$allocator@D@2@@std@@V?$vector@V?$basic_s"
"tring@DU?$char_traits@D@std@@V?$allocator@D@"
"2@@std@@V?$allocator@V?$basic_string@DU?$cha"
"r_traits@D@std@@V?$allocator@D@2@@std@@@2@@2"
"@@std@@V?$basic_string@DU?$char_traits@D@std"
"@@V?$allocator@D@2@@3@PEAVActor@@@Z",
string a1, Actor* a2){
	cout << a1 << endl;
	pairsvs a =  original(a1, a2);
	cout << a.first << endl;
	return a;
}

; public: virtual struct std::pairstring, class std::vector<string>> ActorDamageSource::getDeathMessage(string, class Actor *)const
?getDeathMessage@ActorDamageSource@@UEBA?AU?$pair@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@V?$vector@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@V?$allocator@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@@2@@std@@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@3@PEAVActor@@@Z proc near

*/
/*
enum ActorDamageCause :int {
	Override = 0,
	Contact = 1,
	EntityAttack = 2,
	Projectile_0 = 3,
	Suffocation = 4,
	Fall_0 = 5,
	Fire_1 = 6,
	FireTick = 7,
	Lava_2 = 8,
	Drowning = 9,
	BlockExplosion = 10,
	EntityExplosion = 11,
	Void = 12,
	Suicide = 13,
	Magic = 14,
	Wither = 15,
	Starve = 16,
	Anvil_1 = 17,
	Thorns_0 = 18,
	FallingBlock_0 = 19,
	Piston_0 = 20,
	FlyIntoWall = 21,
	Magma = 22,
	Fireworks_0 = 23,
	Lightning_0 = 24,
	Charging = 25,
	Temperature = 26,
	All_0 = 27,
	None_18 = 28,
};*/
THook(void,
	"?die@Player@@UEAAXAEBVActorDamageSource@@@Z",
	Player* _this, ActorDamageSource* a2) {
	if (enablews) {
		string playername = _this->getNameTag();
		ActorUniqueID src_id = a2->getEntityUniqueID();
		//__int64* cause = (unsigned long long*)a2;
		//unsigned int rcause = *(unsigned long long*)(cause + 8);
		//string* cause_n = SymCall("?lookupCauseName@ActorDamageSource@@SAAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@W4ActorDamageCause@@@Z", string * ,unsigned int)(rcause);
		//cout << rcause <<"--"<<cause_n->c_str()<< endl;
		Actor* src = LocateS<ServerLevel>()->fetchEntity(src_id, false);
		if (src) {
			string src_type_name = SymCall("?getEntityName@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEBVActor@@@Z", string, Actor*)(src);
			ws_send_all(makejson({ { "operate", "onplayerdie" }, { "target", playername }, { "source",src_type_name } }));
		}
		else {
			ws_send_all(makejson({ { "operate", "onplayerdie" }, { "target", playername }, { "source","unknow" } }));
		}
	}
	original(_this, a2);
}
//

void startws() {
	addListener([](ServerStartedEvent& event) {
		thread ws_thread(ws);
		ws_thread.detach();
		ws_not_inited = false;

	});
}
enum ws1 :int {
	listws = 1,
	disall = 2,
	ping = 3,
};
enum ws2 :int {
	sendtoid = 1,
	disid = 2,
};
bool wss(CommandOrigin const& ori, CommandOutput& outp, MyEnum<ws1> op) {
	switch (op) {
	case listws:
		outp.addMessage(string("List Of " + to_string(ws_get_all_connection_count()) + " WS Clients"));
		outp.addMessage(ws_get_all_string());
		break;
	case disall:
		if (ws_disconn_all())
			outp.addMessage("Succfully disconnect all connection");
		else
			outp.addMessage("No Connected client");
		break;
	case ping:
		outp.addMessage("Server is Okay");
		break;
	}
	return true;
}
bool wss2(CommandOrigin const& ori, CommandOutput& outp, MyEnum<ws2> op, int num, optional<string>& text) {
	switch (op) {
	case sendtoid:
		if (text.set) {
			if (ws_send_id(num, text.val())) {
				outp.addMessage("Success");
			}
			else {
				outp.addMessage("Failed");
			}
		}
		break;
	case disid:
		if (ws_dis_id(num)) {
			outp.addMessage("Success");
		}
		else {
			outp.addMessage("Failed");
		}
		break;
	}
	return true;
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
			CEnum<ws1> _1("type", { "listws","disall","ping" });
			CEnum<ws2> _2("type2", { "sendtoid","disid" });
			CmdOverload(ws, wss, "operate");
			CmdOverload(ws, wss2, "operate", "wsid", "text");
		});
	}
}