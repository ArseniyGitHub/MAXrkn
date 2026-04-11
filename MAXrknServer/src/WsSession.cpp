#include "WsSession.hpp"

WsSession::WsSession(beast::ssl_stream<beast::tcp_stream>&& t_stream, RoomMenenger* cr, DBPool* db, long long user_id) : stream(std::move(t_stream)), chatroom(cr), dbpool(db), auth_user_id(user_id) {

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
		std::cerr << "Error on read: " << err.message() << std::endl;
		return;
	}
	auto msg_string = beast::buffers_to_string(buffer.data());
	buffer.consume(buffer.size());
	try {
		json m = json::parse(msg_string);
		long long chat_id = m["chat_id"].get<long long>();
		auto con = dbpool->getConnection();
		pqxx::work w(*con);
		auto res = w.exec_params("(WITH msg AS (INSERT INTO messages (sender_id, message_text, send_time, chat_id) values ($1, $2, NOW(), $3) RETURNING id, sender_id, send_time) SELECT i.id, i.send_time, u.username FROM msg i JOIN users u ON u.id = i.sender_id)", m["user_id"].get<long long>(), m["content"].get<std::string>(), chat_id);
		w.commit();
		
		if (!res.empty()) {
			json broadmsg;
			broadmsg["id"] = res[0][0].as<long long>();
			broadmsg["created_at"] = res[0][1].as<std::string>();
			broadmsg["sender_name"] = res[0][2].as<std::string>();
			broadmsg["chat_id"] = chat_id;
			broadmsg["sender_id"] = m["user_id"];
			broadmsg["content"] = m["content"];
			chatroom->broadcast(chat_id, broadmsg.dump());
		}
		
	}
	catch (const std::exception& e) {
		std::cerr << "Ошибка: " << e.what() << std::endl;
	}
	do_read();
}

void WsSession::on_accept(beast::error_code ec)
{

	if (ec)return;
	ws::stream_base::timeout opt;
	opt.handshake_timeout = std::chrono::seconds(30);
	opt.idle_timeout = std::chrono::minutes(5);
	opt.keep_alive_pings = true;
	//stream.set_option(ws::stream_base::timeout::suggested(beast::role_type::server));
	stream.set_option(opt);
	stream.control_callback([] (ws::frame_type kind, boost::string_view payload) {
		/*
		if (kind == ws::frame_type::ping) {
			std::cout << "Received ping: " << payload << std::endl;
		}
		else if (kind == ws::frame_type::pong) {
			std::cout << "Received pong: " << payload << std::endl;
		}
		*/
		});
	// chatroom->join(this);
	do_read();
}
