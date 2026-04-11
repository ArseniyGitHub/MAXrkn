#pragma once
#include "RoomMenenger.hpp"
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <string>
#include <nlohmann/json.hpp>
#include "Database.hpp"
#include <boost/beast/ssl.hpp>


namespace beast = boost::beast;
namespace ws = beast::websocket;
namespace net = boost::asio;
using json = nlohmann::json;

class WsSession : public std::enable_shared_from_this<WsSession> {
public:
	WsSession(beast::ssl_stream<beast::tcp_stream>&&, RoomMenenger*, DBPool*, long long);
	template <typename Body, typename Field>
	void run(beast::http::request<Body, Field>&& req) {
		stream.async_accept(req, beast::bind_front_handler(&WsSession::on_accept, shared_from_this()));
	}
	void send(std::shared_ptr<std::string>&);
	~WsSession() {
		chatroom->leave_all(this);
	}
private:
	void do_read();
	void on_read(beast::error_code, size_t);
	void on_accept(beast::error_code);
	beast::flat_buffer buffer;
	RoomMenenger* chatroom;
	DBPool* dbpool;
	ws::stream<beast::ssl_stream<beast::tcp_stream>> stream;
	long long auth_user_id = 0;
};
