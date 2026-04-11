#pragma once
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <memory>
#include "Router.hpp"
#include "WsSession.hpp"
#include <boost/beast/ssl.hpp>
#include <boost/asio/ssl.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class HttpSession : public std::enable_shared_from_this<HttpSession> {
public:
	HttpSession(tcp::socket&&, net::ssl::context*, std::shared_ptr<Router>, RoomMenenger*, DBPool*);
	void run();
private:
	void do_read();
	void on_read(beast::error_code, size_t);
	void on_write(bool, beast::error_code, size_t);
	void do_close();
	void on_handshake(beast::error_code);
	beast::ssl_stream<beast::tcp_stream> stream;
	beast::flat_buffer buffer;
	std::shared_ptr<Router> router;
	http::request<http::string_body> req;
	RoomMenenger* chatroom;
	DBPool* dbpool;

};