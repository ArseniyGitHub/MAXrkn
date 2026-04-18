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

void WsSession::handle_send_message(const json& m) {
	try {
		long long chat_id = m["chat_id"].get<long long>();
		if (!chatroom->is_member(chat_id, this)) {
			send_error("You are not a member of this chat");
			return;
		}
		auto con = dbpool->getConnection();
		pqxx::work w(*con);
		auto res = w.exec_params("(WITH msg AS (INSERT INTO messages (sender_id, message_text, send_time, chat_id) values ($1, $2, NOW(), $3) RETURNING id, sender_id, send_time) SELECT i.id, i.send_time, u.username FROM msg i JOIN users u ON u.id = i.sender_id)", auth_user_id, m["content"].get<std::string>(), chat_id);
		w.commit();
		if (!res.empty()) {
			json broadmsg;
			broadmsg["id"] = res[0][0].as<long long>();
			broadmsg["created_at"] = res[0][1].as<std::string>();
			broadmsg["sender_name"] = res[0][2].as<std::string>();
			broadmsg["chat_id"] = chat_id;
			broadmsg["sender_id"] = auth_user_id;
			broadmsg["content"] = m["content"];
			chatroom->broadcast(chat_id, this, broadmsg.dump());
		}
	}
	catch (const std::exception& e) {
		std::cerr << "Ошибка: " << e.what() << std::endl;
	}
}

void WsSession::handle_join_room(const json& m) {
	try {
		long long chat_id = m["chat_id"].get<long long>();
		auto con = dbpool->getConnection();
		pqxx::nontransaction w(*con);
		auto res = w.exec_params("SELECT 1 FROM chat_members WHERE chat_id = $1 AND member_id = $2", chat_id, auth_user_id);
		if(res.empty()) {
			send_error("You are not a member of this chat");
			return;
		}
		chatroom->join(chat_id, this);
		std::cout << "User " << auth_user_id << " joined chat " << chat_id << std::endl;
	}
	catch (const std::exception& e) {
		std::cerr << "Ошибка: " << e.what() << std::endl;
	}
}

void WsSession::handle_typing(const json& m) {
	
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
		auto type = m["type"].get<std::string>();
		if (type == "message") {
			handle_send_message(m);
		}
		else if(type == "join_room") {
			handle_join_room(m);
		}
		else if(type == "typing") {
			handle_typing(m);
		}
		else {
			send_error("Unknown message type");
		}
	}
	catch (const std::exception& e) {
		std::cerr << "Ошибка: " << e.what() << std::endl;
	}
	do_read();
}

void WsSession::send_error(const std::string& error_message) {
	json error_json;
	error_json["type"] = "error";
	error_json["message"] = error_message;
	auto msg_str = std::make_shared<std::string>(error_json.dump());
	send(msg_str);
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

		});
	try {
		auto conn = dbpool->getConnection();
		pqxx::work w(*conn);
		auto res = w.exec_params("SELECT chat_id FROM chat_members WHERE member_id = $1", auth_user_id);
		for (const auto& e : res) {
			long long chat_id = e[0].as<long long>();
			chatroom->join(chat_id, this);
		}
	}
	catch(const std::exception& e) {
		std::cerr << "Error on read: " << e.what() << std::endl;
	}
	do_read();
}
