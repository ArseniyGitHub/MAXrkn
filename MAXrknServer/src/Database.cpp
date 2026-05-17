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
			"CREATE TABLE IF NOT EXISTS storage_files ",
			"(id uuid default gen_random_uuid() primary key, ",
			"file_path text, ",
			"sha256_hash text unique not null, ",
			"file_size bigint, ",
			"created_at timestamptz default NOW(), ",
			"ref_count bigint default 1",
			");"
		));
		w.exec(lines(
			"CREATE TABLE IF NOT EXISTS users ",
			"(id bigint GENERATED ALWAYS AS IDENTITY primary key, ",
			"username text unique not null, ",
			"name text, ",
			"phone_number text, ",
			"email text, ",
			"password text, ",
			"role text default 'user', ",
			"birthday timestamptz, ",
			"create_time timestamptz",
			");"
		));
		w.exec(lines(
			"CREATE TABLE IF NOT EXISTS chats ",
			"(id bigint GENERATED ALWAYS AS IDENTITY primary key, ",
			"name text, ",
			"chat_type text default 'private', ",
			"description text default '', ",
			"created timestamptz",
			");"
		));
		w.exec(lines(
			"CREATE TABLE IF NOT EXISTS messages ",
			"(id bigint GENERATED ALWAYS AS IDENTITY primary key, ",
			"chat_id bigint, ",
			"sender_id bigint, ",
			"message_text text, ",
			"send_time timestamptz, ",
			"reply_to_msg_id bigint default NULL, ",
			"is_forwarded boolean default false, ",
			"foreign key (chat_id) references chats (id) on delete cascade, ",
			"foreign key (sender_id) references users (id) on delete set NULL, ",
			"foreign key (reply_to_msg_id) references messages (id) on delete set NULL",
			");"
		));
		w.exec(lines(
			"CREATE TABLE IF NOT EXISTS attachments ",
			"(id uuid default gen_random_uuid() primary key, ",
			"msg_id bigint, ",
			"uploader_id bigint, ",
			"display_name text, ",
			"content_type text, ",
			"send_data timestamptz, ",
			"file_id uuid, ",
			"foreign key (msg_id) references messages (id) on delete cascade, ",
			"foreign key (uploader_id) references users (id), ",
			"foreign key (file_id) references storage_files (id)",
			");"
		));
		w.exec(lines(
			"CREATE TABLE IF NOT EXISTS chat_members ",
			"(chat_id bigint, ",
			"member_id bigint, ",
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
