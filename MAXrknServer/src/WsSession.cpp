#include "WsSession.hpp"

WsSession::WsSession(beast::tcp_stream&& t_stream, ChatRoom* cr, DBPool* db) : stream(std::move(t_stream)), chatroom(cr), dbpool(db) {

}

void WsSession::send(std::shared_ptr<std::string>& req)
{
	stream.async_write(net::buffer(*req), [self = shared_from_this(), req] (auto ec, auto) {
		
		});
}

void WsSession::do_read() {
	stream.async_read(buffer, beast::bind_front_handler(&WsSession::on_read, shared_from_this()));

}

void WsSession::on_read(beast::error_code err, size_t received) {
	boost::ignore_unused(received);
	if (err) {
		return;
	}
	auto msg_string = beast::buffers_to_string(buffer.data());
	try {
		json m = json::parse(msg_string);
		auto con = dbpool->getConnection();
		pqxx::work w(*con);
		w.exec_params("INSERT INTO messages (sender_id, content) values ($1, $2)", m["user_id"].get<int>(), m["text"].get<std::string>());
		w.commit();
		chatroom->broadcast(msg_string);
	}
	catch (const std::exception& e) {
		
	}
}

void WsSession::on_accept(beast::error_code ec)
{
	if (ec)return;
	chatroom->join(this);
	do_read();
}
