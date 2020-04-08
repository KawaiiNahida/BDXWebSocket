#include <iostream>
#include "../tesws/client_ws.hpp"
#include "../tesws/server_ws.hpp"
#include <future>
#include <openssl/md5.h>
#include <rapidjson/document.h>
#include<stl\KVDB.h>
#include<api\types\helper.h>
#include<string>
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
void wsclient() {
    WsClient client(addr);
    client.on_message = [](shared_ptr<WsClient::Connection> /*connection*/, shared_ptr<WsClient::InMessage> in_message) {
        cout << "WS Client: \"" << in_message->string() << "\"" << endl;
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
    u2a(buf, time.tm_min-1);
    str += buf;
    return str;
}
int main() {
    system("chcp  65001");
    std::cout << "conn add: ws://";
    //cin >> addr;
    std::getline(cin, addr);
    std::cout << std::endl;
    std::cout << "Passwd: ";
    string passwd;
    std::getline(cin, passwd);
    std::cout << std::endl;
    std::cin.clear();
    std::cout << "Mode: ";
    string mode;
    std::getline(cin, mode);
    std::cout << std::endl;
    std::cin.clear();
    thread t(wsclient);
    t.detach();
    if (mode == "hand") {
        for (;;) {
            Sleep(10);
            cout << "wait for input" << endl;
            cout << "passwd " << passwd + gettime() << "  " << MD5(passwd + gettime()) << endl;
            cout << "passwd " << passwd + gettime1r() << "  " << MD5(passwd + gettime1r()) << endl;
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
        std::cout << std::endl;
        std::cin.clear();
        for (;;) {
            Sleep(2);
            std::cout << std::endl;
            std::cin.clear();
            //cout << "wait for input" << endl;
            std::string inmsg;
            std::getline(cin, inmsg);
            std::cin.clear();
            string cmd = "{ \"op\":\"" + subm + "\",\"passwd\" : \"" + MD5(passwd + gettime()) + "\",\"cmd\" : \"" + inmsg + "\" }";
            //cout << cmd << endl;
            scon->send(cmd);
            inmsg = "";
        }
    }
    return 0;
}