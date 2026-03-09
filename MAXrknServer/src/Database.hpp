#pragma once
#include <pqxx/pqxx>
#include <string>
#include <memory>
#include <vector>
#include <stack>
#include <mutex>
#include <iostream>
#include <stdio.h>
#include <functional>

class DBPool {
	std::string connectionString;
	size_t connectionsCount;
	std::stack<std::unique_ptr<pqxx::connection>> connections;
	std::mutex connectionsMutex;
	void initDB();
public:
	DBPool(const std::string& connectionStr, size_t size);
	void deleteDB();
	std::unique_ptr<pqxx::connection, std::function<void(pqxx::connection*)>> getConnection();
};