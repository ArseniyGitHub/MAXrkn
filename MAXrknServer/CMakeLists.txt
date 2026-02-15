add_executable(server "main.cpp" "src/Database.cpp" "src/Router.cpp" "src/Session.cpp" "src/Models.hpp")

target_link_libraries(server PRIVATE Boost::beast Boost::asio Boost::system nlohmann_json::nlohmann_json Boost::date_time Boost::context ws2_32 mswsock)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET server PROPERTY CXX_STANDARD 23)
endif()