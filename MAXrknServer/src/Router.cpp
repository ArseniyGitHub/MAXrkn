#include "Router.hpp"
#include <jwt-cpp/jwt.h>
#include <jwt-cpp/traits/nlohmann-json/traits.h>
#include "../secret.hpp"


using jwt_traits = jwt::traits::nlohmann_json;
using jwt_token = jwt::basic_claim<jwt_traits>;

http::message_generator Router::handle_request(http::request<http::string_body>&& req) {
	auto const method = req.method();
	auto const target = req.target();
	try {
		switch (method) {
		case http::verb::post:
			if (target.starts_with("/api/register")) {
				return handle_register(std::move(req));
			}
			else if (target.starts_with("/api/login")) {
				return handle_login(std::move(req));
			}
			else if (target.starts_with("/api/chats")) {
				return handle_create_chat(std::move(req));
			}
			break;
		case http::verb::get:
			if (target.starts_with("/api/messages")) {
				return handle_get_messages(std::move(req));
			}
			else if (target.starts_with("/api/chats")) {
				return handle_get_chats(std::move(req));
			}
		}
		
		return make_error_responce(req, http::status::not_found, "Не удалось найти страницу. Обратитесь в техподдержку MAX/VK.");
	}
	catch (const std::exception& e) {
		std::cerr << "Внутренняя ошибка: " << e.what() << std::endl;
		return make_error_responce(req, http::status::internal_server_error, "Внутренняя ошибка сервера");
	}
}

http::message_generator Router::make_json_responce(const http::request<http::string_body>& req, const nlohmann::json& body, http::status s) {
	http::response<http::string_body> r{ s, req.version()};
	r.set(http::field::content_type, "application/json");
	r.keep_alive(req.keep_alive());
	r.body() = body.dump();
	r.prepare_payload();
	return r;
}

http::message_generator Router::handle_get_chats(http::request<http::string_body>&& req) {
	try {
		std::string auth = req[http::field::authorization];
		long long user_id;
		if(!auth.starts_with("Bearer ")) {
			return make_error_responce(req, http::status::unauthorized, "Unauthorized");
		}
		auth = auth.substr(7);
		if(!verify_token(auth, user_id)) {
			return make_error_responce(req, http::status::unauthorized, "Unauthorized");
		}
		auto conn = dbpool->getConnection();
		pqxx::work w(*conn);
		auto res = w.exec_params("SELECT c.id, c.name, c.description, c.chat_type FROM chats c JOIN chat_members cm ON c.id = cm.chat_id WHERE cm.member_id = $1", user_id);
		json chats = json::array();
		for (const auto& row : res) {
			json chat;
			chat["chat_id"] = row[0].as<long long>();
			chat["name"] = row[1].as<std::string>();
			chat["description"] = row[2].as<std::string>();
			chat["chat_type"] = row[3].as<std::string>();
			chats.push_back(chat);
		}
		return make_json_responce(req, chats, http::status::ok);
	}
	catch(const std::exception& e) {
		std::cerr << "Error getting chats: " << e.what() << std::endl;
		return make_error_responce(req, http::status::internal_server_error, "Server error");
	}
}

http::message_generator Router::handle_register(http::request<http::string_body>&& req)
{
	try {
		auto data = json::parse(req.body());
		if (!data.contains("username")) throw;
		if (!data.contains("password")) throw;
		std::string username = data["username"].get<std::string>();
		std::string password = data["password"].get<std::string>();
		auto conn = dbpool->getConnection();
		pqxx::work w(*conn);
		auto check = w.exec_params("SELECT id FROM users WHERE username = $1", username);
		if(!check.empty())
			return make_error_responce(req, http::status::conflict, "User with this username already exists");
		w.exec_params("INSERT INTO users (username, password, create_time) VALUES ($1, $2, NOW())", username, password);
		w.commit();
		json answer;
		answer["status"] = "success";
		return make_json_responce(req, answer, http::status::ok);
	}
	catch (const std::exception& e) {
		std::cerr << "Ошибка при авторизации: " << e.what() << std::endl;
		return make_error_responce(req, http::status::bad_request, "Registration failed");
	}
}

http::message_generator Router::handle_login(http::request<http::string_body>&& req)
{
	try {
		auto data = json::parse(req.body());
		if (!data.contains("username")) throw;
		if (!data.contains("password")) throw;
		std::string username = data["username"].get<std::string>();
		std::string password = data["password"].get<std::string>();
		auto conn = dbpool->getConnection();
		pqxx::work w(*conn);
		auto result = w.exec_params("SELECT id, password FROM users WHERE username = $1", username);
		if (result.empty())
			return make_error_responce(req, http::status::unauthorized, "Uncorrect username/password");
		std::string dbpassword = result[0][1].as<std::string>();
		if(dbpassword != password)
			return make_error_responce(req, http::status::unauthorized, "Uncorrect username/password");
		size_t id = result[0][0].as<size_t>();
		auto token = jwt::create<jwt_traits>()
			.set_issuer("MAXrknServer")
			.set_type("JWT")
			.set_expires_at(std::chrono::system_clock::now() + std::chrono::hours(24))
			.set_payload_claim("user_id", jwt_token(std::to_string(id)))
			.sign(jwt::algorithm::hs256{ Secret::secret_key });
		json answer;
		answer["token"] = token;
		answer["user_id"] = id;
		return make_json_responce(req, answer, http::status::ok);
	}
	catch(const std::exception& e) {
		std::cerr << "Ошибка при авторизации: " << e.what() << std::endl;
		return make_error_responce(req, http::status::bad_request, "Auth error");
	}
}

http::message_generator Router::handle_get_messages(http::request<http::string_body>&& req)
{
	try {
		std::string auth = req[http::field::authorization];
		long long user_id;
		if (!auth.starts_with("Bearer ")) {
			return make_error_responce(req, http::status::unauthorized, "Unauthorized");
		}
		auth = auth.substr(7);
		if (!verify_token(auth, user_id)) {
			return make_error_responce(req, http::status::unauthorized, "Unauthorized");
		}
		auto conn = dbpool->getConnection();
		pqxx::nontransaction n(*conn);
		std::string target = req.target();
		long long chat_id;
		auto i = target.find("chat_id=");
		if (i != std::string::npos) {
			chat_id = std::stoll(target.substr(i + 8));
		}
		else {
			return make_error_responce(req, http::status::bad_request, "Chat id is required");
		}
		pqxx::result check = n.exec_params("SELECT 1 FROM chat_members WHERE chat_id = $1 AND member_id = $2", chat_id, user_id);
		if (check.empty()) {
			return make_error_responce(req, http::status::forbidden, "You are not a member of this chat");
		}
		pqxx::result r = n.exec_params("SELECT m.id, u.username, m.message_text, m.send_time, m.sender_id "
			"FROM messages m "
			"LEFT JOIN users u ON m.sender_id = u.id "
			"WHERE m.chat_id = $1 "
			"ORDER BY m.send_time", chat_id);
		json history = json::array();
		for (const auto& e : r) {
			json message;
			message["id"] = e["id"].as<long long>();
			message["sender_name"] = e["username"].is_null() ? "Unknown user" : e["username"].as<std::string>();
			message["sender_id"] = e["sender_id"].as<long long>();
			message["created_at"] = e["send_time"].as<std::string>();
			message["content"] = e["message_text"].as<std::string>();
			message["chat_id"] = chat_id;
			history.push_back(message);
		}
		return make_json_responce(req, history, http::status::ok);
	}
	catch (const std::exception& e) {
		std::cerr << "Ошибка получения истории: " << e.what() << std::endl;
		return make_error_responce(req, http::status::internal_server_error, "Some errors");
	}
}

http::message_generator Router::handle_create_chat(http::request<http::string_body>&& req) {
	try {
		std::string auth = req[http::field::authorization];
		long long user_id;
		if (!auth.starts_with("Bearer ")) {
			return make_error_responce(req, http::status::unauthorized, "Unauthorized");
		}
		auth = auth.substr(7);
		if (!verify_token(auth, user_id)) {
			return make_error_responce(req, http::status::unauthorized, "Unauthorized");
		}
		json data = json::parse(req.body());
		std::string chat_name = data.value("name", "New chat");
		std::string description = data.value("description", "");
		std::string chat_type = data.value("chat_type", "private");
		if (chat_type != "private" && chat_type != "group" && chat_type != "channel") {
			return make_error_responce(req, http::status::bad_request, "Chat type must be 'private', 'group' or 'channel'");
		}
		auto conn = dbpool->getConnection();
		pqxx::work w(*conn);
		long long chat_id = w.exec_params("INSERT INTO chats (name, description, chat_type, created) VALUES ($1, $2, $3, NOW()) RETURNING id", chat_name, description, chat_type)[0][0].as<long long>();
		w.exec_params("INSERT INTO chat_members (chat_id, member_id, role) VALUES ($1, $2, 'owner')", chat_id, user_id);
		w.commit();
		return make_json_responce(req, { {"chat_id", chat_id}, {"name", chat_name}, {"description", description}, {"chat_type", chat_type} }, http::status::created);
	}
	catch(const std::exception& e) {
		std::cerr << "Error creating chat: " << e.what() << std::endl;
		return make_error_responce(req, http::status::internal_server_error, "Server error");
	}
}


http::message_generator Router::make_error_responce(const http::request<http::string_body>& body, http::status s, std::string_view str) {
	return make_json_responce(body, {}, s);
}

Router::Router(DBPool& pool) : dbpool(&pool) {

}

bool Router::verify_token(const std::string& token, long long& user_id) {
	try {
		auto real_token = jwt::decode<jwt_traits>(token);
		jwt::verify<jwt_traits>()
			.allow_algorithm(jwt::algorithm::hs256{ Secret::secret_key })
			.with_issuer("MAXrknServer")
			.verify(real_token)
			;
		if (real_token.has_payload_claim("user_id")) {
			user_id = std::stoll(real_token.get_payload_claim("user_id").as_string());
		}
		return true;
	}
	catch (const std::exception& e) {
		std::cerr << "Ошибка при проверке токена: " << e.what() << std::endl;
		return false;
	}
}