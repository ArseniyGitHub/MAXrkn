# CMakeList.txt: файл проекта CMake верхнего уровня; проведите глобальную конфигурацию
# и включите сюда подпроекты.
#
cmake_minimum_required (VERSION 3.16)

if(DEFINED ENV{VCPKG_HOME})
    
    include("$ENV{VCPKG_HOME}/scripts/buildsystems/vcpkg.cmake")
else()
    include("$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
endif()
set     (VCPKG_INCLUDE_DIR "$ENV{VCPKG_HOME}/installed/${VCPKG_TARGET_TRIPLET}/include")
set         (VCPKG_LIB_DIR "$ENV{VCPKG_HOME}/installed/${VCPKG_TARGET_TRIPLET}/lib")
set             (VCPKG_BIN_DIR "$ENV{VCPKG_HOME}/installed/${VCPKG_TARGET_TRIPLET}/bin")
set (VCPKG_DEBUG_BIN_DIR "$ENV{VCPKG_HOME}/installed/${VCPKG_TARGET_TRIPLET}/debug/bin")
set (VCPKG_DEBUG_LIB_DIR "$ENV{VCPKG_HOME}/installed/${VCPKG_TARGET_TRIPLET}/debug/lib")
include_directories(VCPKG_INCLUDE_DIR)
include_directories(${OPENSSL_INCLUDE_DIR})

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

find_package(Boost REQUIRED COMPONENTS asio beast system date_time context)
find_package(jwt-cpp CONFIG REQUIRED)
# find_package(SFML COMPONENTS Network Graphics Window Audio System CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
# find_package(PkgConfig REQUIRED)

project ("MAXrkn")

# Включите подпроекты.
add_subdirectory ("MAXrknServer")
