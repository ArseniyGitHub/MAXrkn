add_executable(server "main.cpp" "src/Database.cpp" "src/Router.cpp" "src/HttpSession.cpp" "src/Models.hpp" "src/WsSession.cpp" "src/ChatRoom.cpp")

target_link_libraries(server PRIVATE Boost::beast Boost::asio Boost::system nlohmann_json::nlohmann_json Boost::date_time Boost::context ws2_32 mswsock libpqxx::pqxx jwt-cpp::jwt-cpp)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  
endif()