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
		std::cout << msg_string << std::endl;
		auto con = dbpool->getConnection();
		pqxx::work w(*con);
		auto res = w.exec_params("(WITH msg AS (INSERT INTO messages (sender_id, message_text, send_time) values ($1, $2, NOW()) RETURNING id, sender_id, send_time) SELECT i.id, i.send_time, u.username FROM msg i JOIN users u ON u.id = i.sender_id)", m["user_id"].get<int>(), m["content"].get<std::string>());
		w.commit();
		if (!res.empty()) {
			json broadmsg;
			broadmsg["id"] = res[0][0].as<long long>();
			broadmsg["created_at"] = res[0][1].as<std::string>();
			broadmsg["sender_name"] = res[0][2].as<std::string>();
			broadmsg["sender_id"] = m["user_id"];
			broadmsg["content"] = m["content"];
			chatroom->broadcast(broadmsg.dump());
		}
		
	}
	catch (const std::exception& e) {
		std::cerr << "Ошибка: " << e.what() << std::endl;
	}
}

void WsSession::on_accept(beast::error_code ec)
{
	if (ec)return;
	chatroom->join(this);
	do_read();
}
