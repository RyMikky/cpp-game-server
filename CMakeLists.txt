cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

# Назначаю переменную с путем к файлу настроек conan
set(CONAN_BUILD_INFO "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
if(EXISTS ${CONAN_BUILD_INFO})
	# Если файл найден, значит просто используем готовое
	include(${CONAN_BUILD_INFO})
	conan_basic_setup(TARGETS)
else()
	# Если файла нет, то выполняем команды конфигурации conan
	if(CMAKE_BUILD_TYPE STREQUAL "Release")
		execute_process(COMMAND conan install .. --build=missing -s build_type=Release-s compiler.libcxx=libstdc++11)
	elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
		execute_process(COMMAND conan install .. --build=missing -s build_type=Debug -s compiler.libcxx=libstdc++11)
	endif()
	
	# Если после выполнения команд файл найден используем полученные материалы
	if(EXISTS ${CONAN_BUILD_INFO})
		include(${CONAN_BUILD_INFO})
		conan_basic_setup(TARGETS)
	else()
  		message(FATAL_ERROR "The file ${CONAN_BUILD_INFO} does not exist. Please check `conanfile.txt` first.")
	endif()
endif()

# Оригинальная строка
#include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
#conan_basic_setup(TARGETS)

# библиотека отвечающая за игровую модель
# далее в неё будет подключена библиотека с экстра-данными
# без которой работать не будет 
add_library(GameModel STATIC src/model.h src/model.cpp src/tagged.h src/extra_data.h)

# библиотека с предоставленным генератором лута
add_library(LootGenerator STATIC src/loot_generator.h src/loot_generator.cpp)

# библиотека игрок, с данными о токене, включает игровую модель
add_library(Player STATIC src/token.h src/token.cpp src/player.h src/player.cpp)
target_link_libraries(Player PUBLIC GameModel)

################################################################################

add_executable(game_server
	src/main.cpp
	src/postgres/postgers.cpp
	src/postgres/postgers.h
	src/postgres/tagged_uuid.cpp
	src/postgres/tagged_uuid.h
	src/postgres/common.h
	src/http_server.cpp
	src/http_server.h
	src/boost_json.cpp
	src/boost_json.h
	src/json_loader.cpp
	src/json_loader.h
	src/serialization_handler.cpp
	src/serialization_handler.h
	src/collision_handler.cpp
	src/collision_handler.h
	src/request_handler.cpp
	src/request_handler.h
	src/resource_handler.cpp
	src/resource_handler.h
	src/logger_handler.cpp
	src/logger_handler.h
	src/game_handler.cpp
	src/game_handler.h
	src/time_handler.cpp
	src/time_handler.h
	src/options.cpp
	src/options.h
	src/domain.cpp
	src/domain.h
	src/sdk.h
)

target_include_directories(game_server PUBLIC GameModel LootGenerator Player)
target_link_libraries(game_server PUBLIC GameModel LootGenerator Player) 

target_include_directories(game_server PRIVATE CONAN_PKG::boost)
target_link_libraries(game_server PRIVATE CONAN_PKG::boost CONAN_PKG::libpq CONAN_PKG::libpqxx)

################################################################################

# собираем тесты генератора лута
add_executable(loot_generator_tests
	tests/loot_generator_tests.cpp
)
target_include_directories(loot_generator_tests PUBLIC LootGenerator)
target_link_libraries(loot_generator_tests PUBLIC LootGenerator) 
target_link_libraries(loot_generator_tests PRIVATE CONAN_PKG::catch2 CONAN_PKG::libpq CONAN_PKG::libpqxx)

################################################################################

# собираем тесты игровой модели
add_executable(model_tests
	tests/model_tests.cpp
)
target_include_directories(model_tests PUBLIC GameModel LootGenerator)
target_link_libraries(model_tests PUBLIC GameModel LootGenerator) 
target_link_libraries(model_tests PRIVATE CONAN_PKG::catch2 CONAN_PKG::libpq CONAN_PKG::libpqxx)

################################################################################

# собираем тесты игрока
add_executable(player_tests
	tests/player_tests.cpp
)
target_include_directories(player_tests PUBLIC GameModel LootGenerator Player)
target_link_libraries(player_tests PUBLIC GameModel LootGenerator Player) 
target_link_libraries(player_tests PRIVATE CONAN_PKG::catch2 CONAN_PKG::libpq CONAN_PKG::libpqxx)

################################################################################

# собираем тесты определителя коллизий
add_executable(collision_handler_tests
	tests/collision_handler_tests.cpp
	src/collision_handler.cpp
	src/collision_handler.h
	src/domain.cpp
	src/domain.h
	src/sdk.h
)
target_include_directories(collision_handler_tests PUBLIC GameModel LootGenerator Player)
target_link_libraries(collision_handler_tests PUBLIC GameModel LootGenerator Player) 
target_include_directories(collision_handler_tests PRIVATE CONAN_PKG::boost)
target_link_libraries(collision_handler_tests PRIVATE CONAN_PKG::catch2 CONAN_PKG::boost CONAN_PKG::libpq CONAN_PKG::libpqxx)

################################################################################

include(CTest)
include(${CONAN_BUILD_DIRS_CATCH2}/Catch.cmake) 

catch_discover_tests(collision_handler_tests) 
catch_discover_tests(loot_generator_tests) 
catch_discover_tests(player_tests) 
catch_discover_tests(model_tests)  
