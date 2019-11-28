#pragma once
#pragma warning(disable : 4996)
#pragma warning(disable : 4267)

#include <set>
#include <map>
#include <string>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>


typedef websocketpp::server<websocketpp::config::asio> server;
using websocketpp::connection_hdl;


class MyRTSPClient;
class UsageEnvironment;


class FMp4Server {
public:
	FMp4Server(UsageEnvironment* env, int port);
	void run_sync();
	void run_async();
	void stop();
	void send(connection_hdl hdl, uint8_t* buf, int buf_size);
protected:
	void on_open(connection_hdl hdl);
	void on_close(connection_hdl hdl);
	void on_message(connection_hdl hdl, server::message_ptr msg);
	server m_server;
	std::set <MyRTSPClient*> m_rcs;
	UsageEnvironment* env;
	std::map<std::string, std::string> proxy;
};
