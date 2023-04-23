#pragma once

#include <iostream>
#include <string>
#include <type_traits>
#include <boost/json.hpp>

#include "domain.h"

namespace json_detail {

	using namespace std::literals;
	namespace json = boost::json;

	// парсер входящей строки из текста в boost::json
	json::value parse_text_to_json(std::string_view line);

	// возвращает строковое представление json-словаря с информацией по конкретному аргументу
	std::string get_debug_argument(std::string_view argument, std::string_view value);
	// возвращает строковое представление json-словаря с информацией по коду и сообщению о ошибке
	std::string get_error_string(std::string_view code, std::string_view message);
	// возвращает строковое представление json-словаря с полной информацией по запрошенной карте
	std::string get_map_info(const model::Map* data);
	// возвращает строковое представление json-массива с полной информацией по вектору карт игры
	std::string get_map_list(const std::vector<model::Map>& maps);

	// ----------------- блок методов генерации тела ответов на запросы к api-сервера -----------------

	// возвращает строкове предаставление json-словаря с информацией о новом загруженном игроке
	std::string get_session_join_player(game_handler::PlayerPtr player);
	// возвращает строковое представление json_словаря с информацией о всех игроках в указанной сессии
	std::string get_session_players_list(game_handler::SPIterator begin, game_handler::SPIterator end);
	// возвращает строковое представление json_словаря с информацией о всех игроках в указанной сессии
	std::string get_session_players_list(const game_handler::SessionPlayers& players);
	// возвращает строковое представление json_словаря с информацией о состоянии в указанной сессии
	std::string get_session_state_list(const game_handler::SessionPlayers& players);

	namespace detail {

		// возвращает json-словарь с информацией по конкретному аргументу
		json::value get_debug_argument(std::string_view argument, std::string_view value);
		// возвращает json-словарь с информацией по коду и сообщению о ошибке
		json::value get_error_value(std::string_view code, std::string_view message);
		// возвращает json-массив с информацией о офисах по запрошенной карте
		json::array get_map_offices(const model::Map* data);
		// возвращает json-массив с информацией о строениях по запрошенной карте
		json::array get_map_builds(const model::Map* data);
		// возвращает json-массив с информацией о дорогах по запрошенной карте
		json::array get_map_roads(const model::Map* data);

	} // namespace detail

} // namespace json_detail