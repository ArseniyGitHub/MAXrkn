#include "Router.hpp"

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
	return make_json_responce(req, {}, http::status::ok);
}

http::message_generator Router::handle_login(http::request<http::string_body>&& req)
{
	return make_json_responce(req, {}, http::status::ok);
}

http::message_generator Router::handle_send_message(http::request<http::string_body>&& req)
{
	return make_json_responce(req, {}, http::status::ok);
}

http::message_generator Router::handle_get_messages(http::request<http::string_body>&& req)
{
	return make_json_responce(req, { {"status", "ok"}}, http::status::ok);
}


http::message_generator Router::make_error_responce(const http::request<http::string_body>& body, http::status s, std::string_view str) {
	return make_json_responce(body, {}, s);
}

Router::Router(DBPool& pool) : dbpool(&pool) {

}