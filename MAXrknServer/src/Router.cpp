#include "Router.hpp"

http::message_generator Router::handle_request(http::request<http::string_body>&& req) {
	return make_json_responce(req, {}, {});
}

http::message_generator Router::make_json_responce(const http::request<http::string_body>& req, const nlohmann::json& body, http::status s) {
	http::response<http::string_body> r{ s, req.version()};
	r.set(http::field::content_type, "application/json");
	r.keep_alive(req.keep_alive());
	r.body() = body.dump();
	r.prepare_payload();
	return r;
}

http::message_generator Router::make_error_responce(const http::request<http::string_body>& body, http::status s, std::string_view str) {
	return make_json_responce(body, {}, s);
}