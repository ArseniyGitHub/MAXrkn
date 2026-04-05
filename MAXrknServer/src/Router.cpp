#include "Router.hpp"
#include <jwt-cpp/jwt.h>
#include <jwt-cpp/traits/nlohmann-json/traits.h>

using jwt_traits = jwt::traits::nlohmann_json;
using jwt_token = jwt::basic_claim<jwt_traits>;

http::message_generator Router::handle_request(http::request<http::string_body>&& req) {
	auto const method = req.method();
	auto const target = req.target();
	try {
		switch (method) {
		case http::verb::post:
			if (target == "/api/register") {
				return handle_register(std::move(req));
			}
			else if (target == "/api/messages") {
				return handle_send_message(std::move(req));
			}
			else if (target == "/api/login") {
				return handle_login(std::move(req));
			}
			break;
		case http::verb::get:
			if (target == "/api/messages") {
				return handle_get_messages(std::move(req));
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
		std::string secret = "amogus228";
		auto token = jwt::create<jwt_traits>()
			.set_issuer("MAXrknServer")
			.set_type("JWT")
			.set_expires_at(std::chrono::system_clock::now() + std::chrono::hours(24))
			.set_payload_claim("user_id", jwt_token(std::to_string(id)))
			.sign(jwt::algorithm::hs256{ secret });
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

http::message_generator Router::handle_send_message(http::request<http::string_body>&& req)
{
	return make_json_responce(req, {}, http::status::ok);
}

http::message_generator Router::handle_get_messages(http::request<http::string_body>&& req)
{
	try {
		auto conn = dbpool->getConnection();
		pqxx::nontransaction n(*conn);
		pqxx::result r = n.exec("SELECT m.id, u.username, m.message_text, m.send_time, m.sender_id "
			"FROM messages m "
			"LEFT JOIN users u ON m.sender_id = u.id "
			"ORDER BY m.send_time ASC LIMIT 50");
		json history = json::array();
		for (const auto& e : r) {
			json message;
			message["id"] = e["id"].as<long long>();
			message["sender_name"] = e["username"].is_null() ? "Imposter" : e["username"].as<std::string>();
			message["sender_id"] = e["sender_id"].as<long long>();
			message["created_at"] = e["send_time"].as<std::string>();
			message["content"] = e["message_text"].as<std::string>();
			history.push_back(message);
		}
		return make_json_responce(req, history, http::status::ok);
	}
	catch (const std::exception& e) {
		std::cerr << "Ошибка получения истории: " << e.what() << std::endl;
		return make_error_responce(req, http::status::internal_server_error, "Some errors");
	}
}


http::message_generator Router::make_error_responce(const http::request<http::string_body>& body, http::status s, std::string_view str) {
	return make_json_responce(body, {}, s);
}

Router::Router(DBPool& pool) : dbpool(&pool) {

}

bool Router::verify_token(const std::string& token, size_t& user_id) {
	try {
		std::string secret = "amogus228";
		auto real_token = jwt::decode<jwt_traits>(token);
		jwt::verify<jwt_traits>()
			.allow_algorithm(jwt::algorithm::hs256{ secret })
			.with_issuer("MAXrknServer")
			.verify(real_token);
		return true;
	}
	catch (const std::exception& e) {
		std::cerr << "Ошибка при проверке токена: " << e.what() << std::endl;
		return false;
	}
}