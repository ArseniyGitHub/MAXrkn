#define _WIN32_WINNT 0x0601
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include <thread>
#include <memory>
#include "src/Database.hpp"
#include "src/HttpSession.hpp"
#include "src/Router.hpp"

namespace net = boost::asio;
using tcp = net::ip::tcp;

class Listener : public std::enable_shared_from_this<Listener> {
	net::io_context* io;
	tcp::acceptor acc;
	ChatRoom* cr;
	DBPool* dbpool;
	std::shared_ptr<Router> router;
	void on_accept(beast::error_code ec, tcp::socket socket) {
		if (ec) {
			std::cerr << "Listener error: accept error: " << ec.what() << std::endl;
			if (ec == net::error::operation_aborted || ec == net::error::bad_descriptor) {
				return;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		else {
			std::make_shared<HttpSession>(std::move(socket), router, cr, dbpool)->run();
		}
		do_accept();
	}
	void do_accept() {
		acc.async_accept(net::make_strand(*io), beast::bind_front_handler(&Listener::on_accept, shared_from_this()));
	}
public:
	Listener(net::io_context& ioc, tcp::endpoint listen_endpoint, std::shared_ptr<Router> r, ChatRoom* cr, DBPool* db) : io(&ioc), router(r), acc(ioc), cr(cr), dbpool(db) {
		beast::error_code ec;
		acc.open(listen_endpoint.protocol(), ec);
		if (ec) {
			std::cerr << "Listener error: opening listener: " << ec.what() << std::endl;
			return;
		}
		acc.set_option(net::socket_base::reuse_address(true), ec);
		if (ec) {
			std::cerr << "Listener error: reusing endpoint: " << ec.what() << std::endl;
			return;
		}
		acc.bind(listen_endpoint, ec);
		if (ec) {
			std::cerr << "Listener error: binding endpoint: " << ec.what() << std::endl;
			return;
		}
		acc.listen(net::socket_base::max_listen_connections, ec);
		if (ec) {
			std::cerr << "Listener error: listening endpoint: " << ec.what() << std::endl;
			return;
		}
	}
	void run() {
		std::cout << "Сервер запущен!\n";
		do_accept();
	}
};

int main() {
	system("chcp 65001");
	try {
		std::shared_ptr<DBPool> db(new DBPool("postgresql://postgres:a1b3c5@localhost:5432/MainUSA_DB", 10));
		std::shared_ptr<Router> r(new Router(*db));
		ChatRoom* cr = new ChatRoom();
		const auto address = net::ip::make_address("0.0.0.0");
		unsigned short port = 24242;
		int thread_count = std::max(1u, std::thread::hardware_concurrency());
		net::io_context ioc;
		auto l = std::make_shared<Listener>(ioc, tcp::endpoint(address, port), r, cr, db.get());
		l->run();
		std::vector<std::thread> thrPool(thread_count - 1);
		for (auto& e : thrPool) {
			e = std::thread([&] () {ioc.run(); });
		}
		ioc.run();
		for (auto& e : thrPool) {
			e.join();
		}
	}
	catch (const std::exception& e) {
		std::cerr << e.what();
	}
	return 0;
}