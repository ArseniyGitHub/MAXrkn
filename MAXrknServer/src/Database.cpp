#include "Database.hpp"

template <typename T>
constexpr std::string lines(T a) { return std::string(a); }
constexpr std::string lines() { return ""; }
template <typename T, typename... other>
constexpr std::string lines(T a, other... args) {
	return std::string(a) + "\n" + lines(args...);
}

DBPool::DBPool(const std::string& connectionStr, size_t size) : connectionString(connectionStr), connectionsCount(size) {
	for (size_t i = 0; i < size; i++)
	{
		connections.push(std::make_unique<pqxx::connection>(pqxx::connection(connectionStr)));
	}
	initDB();
}

void DBPool::initDB() {
	try {
		pqxx::connection c(connectionString);
		pqxx::work w(c);
		std::cout << "Отправляем Ваши данные правительству США...\n";

		w.exec(lines(
			"CREATE TABLE IF NOT EXISTS users ",
			"(id serial primary key, ",
			"username text unique not null, ",
			"phone_number text, ",
			"email text, ",
			"password text, ",
			"role text, ",
			"birthday timestamptz, ",
			"create_time timestamptz",
			");"
		));
		w.exec(lines(
			"CREATE TABLE IF NOT EXISTS chats ",
			"(id serial primary key, ",
			"name text, ",
			"chat_type text default 'private', ",
			"description text default '', ",
			"created timestamptz",
			");"
		));
		w.exec(lines(
			"CREATE TABLE IF NOT EXISTS messages ",
			"(id serial primary key, ",
			"chat_id integer, ",
			"sender_id integer, ",
			"message_text text, ",
			"send_time timestamptz, ",
			"reply_to_msg_id integer, ",
			"foreign key (chat_id) references chats (id) on delete cascade, ",
			"foreign key (sender_id) references users (id) on delete set NULL",
			");"
		));
		w.exec(lines(
			"CREATE TABLE IF NOT EXISTS attachments ",
			"(id serial primary key, ",
			"msg_id integer, ",
			"file_path text, ",
			"send_data timestamptz, ",
			"foreign key (msg_id) references messages (id) on delete cascade ",
			");"
		));
		w.exec(lines(
			"CREATE TABLE IF NOT EXISTS chat_members ",
			"(chat_id integer, ",
			"member_id integer, ",
			"role text, ",
			"primary key (chat_id, member_id), ",
			"foreign key (chat_id) references chats (id) on delete cascade, ",
			"foreign key (member_id) references users (id) on delete cascade",
			");"
		));
		w.commit();
	}
	catch (const std::exception& e) {
		std::cerr << "Ошибка инициализации базы данных: " << e.what() << std::endl;
		throw;
	}
}

void DBPool::deleteDB() {

}

std::unique_ptr<pqxx::connection, std::function<void(pqxx::connection*)>> DBPool::getConnection() {
	std::unique_lock<std::mutex> lock(connectionsMutex);
	if (connections.empty())throw std::runtime_error("нет свободных подключений в пуле");
	auto c = std::move(connections.top());
	connections.pop();
	return std::unique_ptr<pqxx::connection, std::function<void(pqxx::connection*)>>(c.release(), [this] (auto* conn) {
			std::lock_guard<std::mutex> l(this->connectionsMutex);
			connections.push(std::unique_ptr<pqxx::connection>(conn));
		});
}
