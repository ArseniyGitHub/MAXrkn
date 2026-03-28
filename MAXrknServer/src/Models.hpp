#include <string>
#include <nlohmann/json.hpp>
#include <optional>

struct User {
	size_t id;
	std::string username;
	std::string password;
	std::optional<std::string> phone_number;
	std::optional<std::string> email;
	std::optional<std::string> birthday;
	std::optional<std::string> create_time;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(User, id, username, phone_number, email, birthday, create_time)
};

struct Message {
	size_t chat_id;
	size_t sender_id;
	size_t id;
	std::string message_text;
	std::string send_time;
	size_t reply_to_msg_id;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(Message, id, chat_id, sender_id, message_text, send_time, reply_to_msg_id)
};

struct RegisterRequest {
	std::string username;
	std::string password;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(RegisterRequest, username, password)
};

struct LoginResponce {
	std::string token;
	size_t user_id;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(LoginResponce, token, user_id)
};

struct Chat {
	size_t chat_id;
	std::string name;
	std::string description;
	std::string chat_type;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(Chat, chat_id, name, description, chat_type)
};