#pragma once

#include <filesystem>
#include <fstream>

#include "boost_json.h"              // все инклюды получим из базовой системы и domain.h

namespace json_loader {

	namespace json = boost::json;

	namespace detail {

		// парсер элементов карты - здания
		void parse_map_buildings_data(model::Map& map, json::value&& builds);
		// парсер элементов карты - офисы
		void parse_map_offices_data(model::Map& map, json::value&& offices);
		// парсер элементов карты - дороги
		void parse_map_roads_data(model::Map& map, json::value&& roads);
		// базовый парсер элементов полученного config.json
		model::Game parse_game_maps_data(json::value&& maps);
		// базовый парсер элементов полученного config.json, вместе с базовой скоростью
		model::Game parse_game_maps_data(json::value&& maps, double default_dog_speed);

	} // namespace detail

	model::Game load_game(const std::filesystem::path& json_path);

}  // namespace json_loader