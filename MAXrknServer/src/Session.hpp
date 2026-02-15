#pragma once
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <memory>
#include "Router.hpp"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
public:
	Session(tcp::socket&&, std::shared_ptr<Router>);
	void run();
private:
	void do_read();
	void on_read(beast::error_code, size_t);
	void on_write(bool, beast::error_code, size_t);
	void do_close();
	beast::tcp_stream stream;
	beast::flat_buffer buffer;
	std::shared_ptr<Router> router;
	http::request<http::string_body> req;
};