#pragma once
#include <boost/beast.hpp>
#include <string_view>
#include <memory>
#include "Database.hpp"
#include "Models.hpp"
#include <nlohmann/json.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
using json = nlohmann::json;

class Router {
public:
	Router(DBPool&);
	http::message_generator handle_request(http::request<http::string_body>&&);
	bool verify_token(const std::string& token, size_t& user_id);
private:
	DBPool* dbpool;
	http::message_generator make_error_responce(const http::request<http::string_body>&, http::status, std::string_view);
	http::message_generator make_json_responce(const http::request<http::string_body>&, const nlohmann::json&, http::status);
	http::message_generator handle_register(http::request<http::string_body>&&);
	http::message_generator handle_login(http::request<http::string_body>&&);
	http::message_generator handle_get_messages(http::request<http::string_body>&&);
	http::message_generator handle_send_message(http::request<http::string_body>&&);
};