#include "ChatRoom.hpp"
#include "WsSession.hpp"


void ChatRoom::join(WsSession* session)
{
	std::lock_guard<decltype(mutex)> lock(mutex);
	sessions.insert(session);
}

void ChatRoom::leave(WsSession* session)
{
	std::lock_guard<decltype(mutex)> lock(mutex);
	sessions.erase(session);
}

void ChatRoom::broadcast(const std::string& msg)
{
	auto s = std::make_shared<std::string>(msg);
	std::lock_guard<decltype(mutex)> lock(mutex);
	for (auto e : sessions) {
		e->send(s);
	}
}
