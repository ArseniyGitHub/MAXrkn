# CMakeList.txt: файл проекта CMake верхнего уровня; проведите глобальную конфигурацию
# и включите сюда подпроекты.
#
cmake_minimum_required (VERSION 3.16)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_HOME}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "" FORCE)
    # include("$ENV{VCPKG_HOME}/scripts/buildsystems/vcpkg.cmake")
endif()

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("MAXrkn")

find_package(Boost REQUIRED COMPONENTS asio beast system date_time context)
find_package(jwt-cpp CONFIG REQUIRED)
# find_package(SFML COMPONENTS Network Graphics Window Audio System CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(libpqxx CONFIG REQUIRED)

# find_package(PkgConfig REQUIRED)



# Включите подпроекты.
add_subdirectory ("MAXrknServer")
