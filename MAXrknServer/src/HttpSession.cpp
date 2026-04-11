#include "HttpSession.hpp"
#include <iostream>

HttpSession::HttpSession(tcp::socket&& s, net::ssl::context* ctx, std::shared_ptr<Router> r, RoomMenenger* cr, DBPool* db) : stream(std::move(s), *ctx), router(r), chatroom(cr), dbpool(db) {
	
}

void HttpSession::on_handshake(beast::error_code ec) {
	if (ec) {
		std::cerr << "SSL Handshake error: " << ec.message() << std::endl;
		return;
	}
	do_read();
}

void HttpSession::run() {
	beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));
	stream.async_handshake(net::ssl::stream_base::server, beast::bind_front_handler(&HttpSession::on_handshake, shared_from_this()));
	// net::dispatch(stream.get_executor(), beast::bind_front_handler(&HttpSession::do_read, shared_from_this()));
}

void HttpSession::do_read() {
	req = {};
	beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));
	http::async_read(stream, buffer, req, beast::bind_front_handler(&HttpSession::on_read, shared_from_this()));
}

void HttpSession::on_read(beast::error_code err, size_t received) {
	boost::ignore_unused(received);
	if (err == http::error::end_of_stream) {
		do_close();
		return;
	}
	if (err) {
		std::cerr << "Read error: \n" << err.message() << std::endl;
		return;
	}
	if (beast::websocket::is_upgrade(req)) {
		std::string auth_header = req[beast::http::field::authorization];
		long long user_id;
		if (auth_header.starts_with("Bearer ")) {
			std::string token = auth_header.substr(7);
			if (router->verify_token(token, user_id)) {
				beast::get_lowest_layer(stream).expires_never();
				std::make_shared<WsSession>(std::move(this->stream), chatroom, dbpool, user_id)->run(std::move(req));
				return;
			}
		}
		std::cerr << "Warn: unauthorized attempt blocked\n";
		do_close();
		return;
	}
	auto resp = router->handle_request(std::move(req));
	bool keep_alive = resp.keep_alive();
	beast::async_write(stream, std::move(resp), beast::bind_front_handler(&HttpSession::on_write, shared_from_this(), keep_alive));
}

void HttpSession::do_close() {
	beast::error_code err;
	beast::get_lowest_layer(stream).socket().shutdown(tcp::socket::shutdown_send, err);
}

void HttpSession::on_write(bool keep_alive, beast::error_code ec, size_t transfered) {
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