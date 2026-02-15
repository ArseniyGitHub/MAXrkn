#pragma once
#include <boost/beast.hpp>
#include <string_view>
#include <memory>
#include "Database.hpp"
#include "Models.hpp"
#include <nlohmann/json.hpp>

namespace beast = boost::beast;
namespace http = beast::http;

class Router {
public:
	Router(DBPool&);
	http::message_generator handle_request(http::request<http::string_body>&&);
private:
	DBPool dbpool;
	http::message_generator make_error_responce(const http::request<http::string_body>&, http::status, std::string_view);
	http::message_generator make_json_responce(const http::request<http::string_body>&, const nlohmann::json&, http::status);
};