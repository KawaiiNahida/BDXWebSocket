#include <iostream>
#include "../sws/client_ws.hpp"
//#include "../sws/server_ws.hpp"
#include <future>
#include <openssl/md5.h>
#include <rapidjson/document.h>
#include<stl\KVDB.h>
#include<api\types\helper.h>
#include<string>
#include <stdarg.h>
#pragma warning(disable:4996)
using namespace std;
std::string addr;
using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;
std::shared_ptr<WsClient::Connection> scon = nullptr;
short counts = -1;
HANDLE terminal;
void SetCursorPos(COORD point) {
    HANDLE TerminalHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleCursorPosition(TerminalHandle, point);
}
void clear(COORD pos, int size)
{
    SetCursorPos(pos);
    for (int a = 0; a < size; a = a++) {
        cout << " ";
    }
    cout << endl;

}

COORD GetCursorPos() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO concur;
    GetConsoleScreenBufferInfo(hConsole, &concur);
    return { concur.dwCursorPosition.X, concur.dwCursorPosition.Y };
}
COORD getwindowsize() {
    CONSOLE_SCREEN_BUFFER_INFO terminal;
    HANDLE terminalhandle = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(terminalhandle, &terminal);
    //cout << terminal.dwCursorPosition.X << " " << terminal.dwCursorPosition.X << endl;
    return { terminal.srWindow.Right,terminal.srWindow.Bottom };
}
void dolog(string log) {
    if (counts < getwindowsize().Y - 1) {
        SetCursorPos({ 0,counts + 1 });
        cout << log << endl;
        counts++;
        SetCursorPos({ GetCursorPos().X,getwindowsize().Y });
    }
    else {
        system("clear");
        counts = 1;
        SetCursorPos({ 0,0 });
        cout << log << endl;
        SetCursorPos({ 0,getwindowsize().Y });
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
bool formator = false;
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
void wsclient() {
    WsClient client(addr);
    client.on_message = [](shared_ptr<WsClient::Connection> /*connection*/, shared_ptr<WsClient::InMessage> in_message) {
        //dolog("FormServer: " + in_message->string());
        string msg = in_message->string();
        //cout << "FromServer: " + msg << endl;
        if (formator) {
            rapidjson::Document document;
            document.Parse(msg.c_str());
            rapidjson::Value::ConstMemberIterator iter = document.FindMember("operate");
            if (iter != document.MemberEnd()) {
                string op = iter->value.GetString();
                if (op == "onmsg") {
                    string target, text;
                    rapidjson::Value::ConstMemberIterator iter2 = document.FindMember("target");
                    if (iter2 != document.MemberEnd()) {
                        target = iter2->value.GetString();
                    }
                    rapidjson::Value::ConstMemberIterator iter3 = document.FindMember("text");
                    if (iter3 != document.MemberEnd()) {
                        text = iter3->value.GetString();
                    }
                    cout << target << " > " << text << endl;

                }
                if (op == "onjoin") {
                    string target, text;
                    rapidjson::Value::ConstMemberIterator iter2 = document.FindMember("target");
                    if (iter2 != document.MemberEnd()) {
                        target = iter2->value.GetString();
                    }
                    rapidjson::Value::ConstMemberIterator iter3 = document.FindMember("text");
                    if (iter3 != document.MemberEnd()) {
                        text = iter3->value.GetString();
                    }
                    cout << target << " > " << text << endl;

                }
                if (op == "onleft") {
                    string target, text;
                    rapidjson::Value::ConstMemberIterator iter2 = document.FindMember("target");
                    if (iter2 != document.MemberEnd()) {
                        target = iter2->value.GetString();
                    }
                    rapidjson::Value::ConstMemberIterator iter3 = document.FindMember("text");
                    if (iter3 != document.MemberEnd()) {
                        text = iter3->value.GetString();
                    }
                    cout << target << " > " << text << endl;

                }
                if (op == "onCMD") {
                    string target, text;
                    rapidjson::Value::ConstMemberIterator iter2 = document.FindMember("target");
                    if (iter2 != document.MemberEnd()) {
                        target = iter2->value.GetString();
                    }
                    rapidjson::Value::ConstMemberIterator iter3 = document.FindMember("text");
                    if (iter3 != document.MemberEnd()) {
                        text = iter3->value.GetString();
                    }
                    cout << target << " > " << text << endl;

                }
                if (op == "runcmd") {
                    string feedback;
                    rapidjson::Value::ConstMemberIterator iter2 = document.FindMember("Auth");
                    if (iter2 != document.MemberEnd()) {
                        rapidjson::Value::ConstMemberIterator iter3 = document.FindMember("text");
                        if (iter3 != document.MemberEnd()) {
                            feedback = iter3->value.GetString();
                        }
                    }

                    feedback = repall(feedback, "\\n", "\n");
                    cout << "Result:\n" << feedback << endl;

                }
            }
        }
        else { cout << "FromServer: " + in_message->string() << endl; }

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

int main() {
    SetConsoleTitleA("BDXWebsocket TestClient");
    system("chcp  65001");
    cout << "__          __  _     _____            _        _    _____ _ _            _" << endl;
    cout << "\\ \\        / / | |   / ____|          | |      | |  / ____| (_)          | |" << endl;
    cout << " \\ \\  /\\  / /__| |__| (___   ___   ___| | _____| |_| |    | |_  ___ _ __ | |_" << endl;
    cout << "  \\ \\/  \\/ / _ \\ '_ \\\\___ \\ / _ \\ / __| |/ / _ \\ __| |    | | |/ _ \\ '_ \\| __|" << endl;
    cout << "   \\  /\\  /  __/ |_) |___) | (_) | (__|   <  __/ |_| |____| | |  __/ | | | |_" << endl;
    cout << "    \\/  \\/ \\___|_.__/_____/ \\___/ \\___|_|\\_\\___|\\__|\\_____|_|_|\\___|_| |_|\\__|" << endl;
    string title = "BDXWebsocket TestClient ";
    SetConsoleTitleA(title.c_str());
    std::cout << "ServerAddress: ws://";
    std::getline(cin, addr);
    title = title + addr;
    SetConsoleTitleA(title.c_str());
    std::cout << "BasePasswd: ";
    string passwd;
    std::getline(cin, passwd);
    std::cin.clear();
    title = title + " BPW:" + passwd;
    SetConsoleTitleA(title.c_str());
    std::cout << "Mode: ";
    string mode;
    std::getline(cin, mode);
    std::cin.clear();
    title = title + " Mode:" + mode;
    SetConsoleTitleA(title.c_str());
    thread t(wsclient);
    t.detach();
    if (mode == "hand") {
        for (;;) {
            Sleep(10);
            cout << "Now   passwd " << passwd + gettime() << "  " << MD5(passwd + gettime()) << endl;
            cout << "-1Min passwd " << passwd + gettime1r() << "  " << MD5(passwd + gettime1r()) << endl;
            std::string inmsg;
            std::getline(cin, inmsg);
            std::cin.clear();
            std::cout << std::endl;
            scon->send(inmsg);
            inmsg = "";
        }
    }
    if (mode == "auto") {
        std::cout << "submode: ";
        string subm;
        std::getline(cin, subm);
        title = title + " SubMode:" + subm;
        SetConsoleTitleA(title.c_str());
        std::cin.clear();
        system("clear");

        for (;;) {
            Sleep(2);

            std::cin.clear();
            //cout << "wait for input" << endl;
            std::string inmsg;
            std::getline(cin, inmsg);
            std::cin.clear();
            if (inmsg == "close") {
                scon->send_close(1000);
            }
            else {
                string title2 = title + " NowPW:" + passwd + gettime();
                SetConsoleTitleA(title2.c_str());
                string cmd = "{ \"op\":\"" + subm + "\",\"passwd\" : \"" + MD5(passwd + gettime()) + "\",\"cmd\" : \"" + inmsg + "\" }";
                cout << "SendOut Msg: " << cmd << endl;
                //      from server:
                scon->send(cmd);
                inmsg = "";
            }
        }
    }
    if (mode == "cmd") {
        system("clear");
        formator = true;
        for (;;) {
            Sleep(2);

            std::cin.clear();
            std::string inmsg;
            std::getline(cin, inmsg);
            std::cin.clear();
            if (inmsg == "close") {
                scon->send_close(1000);
            }
            else {
                string title2 = title + " NowPW:" + passwd + gettime();
                SetConsoleTitleA(title2.c_str());
                string cmd = "{ \"operate\":\"runcmd\",\"passwd\" : \"" + MD5(passwd + gettime()) + "\",\"cmd\" : \"" + inmsg + "\" }";
                scon->send(cmd);
                inmsg = "";
            }
        }
    }
    return 0;
}