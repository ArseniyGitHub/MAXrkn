#include "RoomMenenger.hpp"
#include "WsSession.hpp"


void RoomMenenger::join(long long chat_id, WsSession* session)
{
	std::lock_guard<decltype(mutex)> lock(mutex);
	room_to_sessions[chat_id].insert(session);
	session_to_rooms[session].insert(chat_id);
}

void RoomMenenger::leave(long long chat_id, WsSession* session)
{
	std::lock_guard<decltype(mutex)> lock(mutex);
	auto it = room_to_sessions.find(chat_id);
	if (it != room_to_sessions.end()) {
		auto& r = it->second;
		r.erase(session);
		if (r.empty()) {
			room_to_sessions.erase(it);
		}
	}
	auto it2 = session_to_rooms.find(session);
	if (it2 != session_to_rooms.end())
		it2->second.erase(chat_id);
}

void RoomMenenger::broadcast(long long chat_id, const std::string& msg)
{
	auto s = std::make_shared<std::string>(msg);
	std::lock_guard<decltype(mutex)> lock(mutex);
	auto it = room_to_sessions.find(chat_id);
	if (it != room_to_sessions.end()) {
		for (auto& e : it->second) {
			e->send(s);
		}
	}
}

void RoomMenenger::leave_all(WsSession* session) {
	std::lock_guard<decltype(mutex)> lock(mutex);
	auto it = session_to_rooms.find(session);
	if (it != session_to_rooms.end()) {
		for (auto& chat_id : it->second) {
			auto it2 = room_to_sessions.find(chat_id);
			if (it2 != room_to_sessions.end()) {
				auto& r = it2->second;
				r.erase(session);
				if (r.empty()) {
					room_to_sessions.erase(it2);
				}
			}
		}
		session_to_rooms.erase(it);
	}
}
