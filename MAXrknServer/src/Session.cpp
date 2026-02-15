#include "Session.hpp"
#include <iostream>

Session::Session(tcp::socket&& s, std::shared_ptr<Router> r) : stream(std::move(s)), router(r) {
	
}

void Session::run() {
	net::dispatch(stream.get_executor(), beast::bind_front_handler(&Session::do_read, shared_from_this()));
}

void Session::do_read() {
	req = {};
	stream.expires_after(std::chrono::seconds(30));
	http::async_read(stream, buffer, req, beast::bind_front_handler(&Session::on_read, shared_from_this()));

}

void Session::on_read(beast::error_code err, size_t received) {
	boost::ignore_unused(received);
	if (err == http::error::end_of_stream) {
		do_close();
		return;
	}
	if (err) {
		std::cerr << "Read error: \n" << err.message() << std::endl;
		return;
	}
}

void Session::do_close() {
	beast::error_code err;
	stream.socket().shutdown(tcp::socket::shutdown_send, err);
}

void Session::on_write(bool keep_alive, beast::error_code ec, size_t transfered) {
	boost::ignore_unused(transfered);
	if (ec) {
		std::cerr << "Write error: \n" << ec.message() << std::endl;
		return;
	}
	if (!keep_alive) {
		do_close();
		return;
	}

	do_read();
}