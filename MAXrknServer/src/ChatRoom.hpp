#pragma once
#include <string>
#include <mutex>
#include <memory>
#include <unordered_set>

class WsSession;

class ChatRoom {
	std::unordered_set<WsSession*> sessions;
	std::mutex mutex;
public:
	void join(WsSession*);
	void leave(WsSession*);
	void broadcast(const std::string&);
};