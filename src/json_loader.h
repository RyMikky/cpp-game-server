#pragma once

#include <filesystem>
#include <fstream>

#include "boost_json.h"              // все инклюды получим из базовой системы и domain.h

namespace json_loader {

	namespace json = boost::json;

	namespace detail {

		// парсер элементов карты - здания
		void ParseMapBuildingsData(model::Map& map, json::value&& builds);
		// парсер элементов карты - офисы
		void ParseMapOfficesData(model::Map& map, json::value&& offices);
		// парсер элементов карты - дороги
		void ParseMapRoadsData(model::Map& map, json::value&& roads);
		// базовый парсер элементов полученного config.json
		model::Game ParseGameMapsData(json::value&& maps);
		// базовый парсер элементов полученного config.json, вместе с базовой скоростью
		model::Game ParseGameMapsData(json::value&& maps, double default_dog_speed);

	} // namespace detail

	model::Game LoadGameConfiguration(const std::filesystem::path& json_path);

}  // namespace json_loader