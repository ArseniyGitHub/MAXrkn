#pragma once
#include <string>
#include <mutex>
#include <memory>
#include <unordered_set>
#include <unordered_map>

class WsSession;

class RoomMenenger {
	std::unordered_map<long long, std::unordered_set<WsSession*>> room_to_sessions;
	std::unordered_map<WsSession*, std::unordered_set<long long>> session_to_rooms;
	std::mutex mutex;
public:
	void join(long long, WsSession*);
	void leave(long long, WsSession*);
	void broadcast(long long, WsSession*, const std::string&);
	void leave_all(WsSession*);
	bool is_member(long long, WsSession*);
};

