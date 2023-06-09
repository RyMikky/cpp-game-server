﻿#pragma once

#include <iostream>
#include <string>
#include <type_traits>
#include <boost/json.hpp>

#include "domain.h"

namespace json_detail {

	using namespace std::literals;
	namespace json = boost::json;

	// парсер входящей строки из текста в boost::json
	json::value ParseTextToJSON(std::string_view line);

	// возвращает строковое представление json-словаря с информацией по конкретному аргументу
	std::string GetDebugArgument(std::string_view argument, std::string_view value);
	// возвращает строковое представление json-словаря с информацией по коду и сообщению о ошибке
	std::string GetErrorString(std::string_view code, std::string_view message);
	// возвращает строковое представление json-словаря с полной информацией по запрошенной карте
	std::string GetMapInfo(const model::Map* data);
	// возвращает строковое представление json-массива с полной информацией по вектору карт игры
	std::string GetMapsList(const std::vector<model::Map>& maps);

	// ----------------- блок методов генерации тела ответов на запросы к api-сервера -----------------

	// возвращает строкове предаставление json-словаря с информацией о новом загруженном игроке
	std::string GetSessionPlayerJoin(game_handler::PlayerPtr player);
	// возвращает строковое представление json_словаря с информацией о всех игроках в указанной сессии
	std::string GetSessionPlayersList(game_handler::SPIterator begin, game_handler::SPIterator end);
	// возвращает строковое представление json_словаря с информацией о всех игроках в указанной сессии
	std::string GetSessionPlayersList(const game_handler::SessionPlayers& players);
	// возвращает строковое представление json_словаря с информацией о состоянии в указанной сессии
	std::string GetSessionStateList(const game_handler::SessionPlayers& players, const game_handler::SessionLoots& loots);

	namespace detail {

		// возвращает json-словарь с информацией по конкретному аргументу
		json::value GetDebugArgument(std::string_view argument, std::string_view value);
		// возвращает json-словарь с информацией по коду и сообщению о ошибке
		json::value GetErrorValue(std::string_view code, std::string_view message);
		// возвращает json-массив с информацией о офисах по запрошенной карте
		json::array GetMapOffices(const model::Map* data);
		// возвращает json-массив с информацией о строениях по запрошенной карте
		json::array GetMapBuilds(const model::Map* data);
		// возвращает json-массив с информацией о дорогах по запрошенной карте
		json::array GetMapRoads(const model::Map* data);
		// возвращает json-массив с информацией о инвентаре игрока
		json::array GetPlayerBag(const game_handler::Player& player);

	} // namespace detail

} // namespace json_detail