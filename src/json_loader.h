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
		// парсер элементов карты - типы лута
		// на данный момент не используется
		void ParseMapLootTypesData(model::Map& map, json::value&& loot_types);

		// парсер карт для созданной игровой модели
		void ParseGameMapsData(model::Game& game, json::value&& maps);
		// парсер настройки карты - настройки генератора лута
		loot_gen::LootGeneratorConfig ParseGameLootGenConfig(json::value&& config);
		
		/*
		* Базовый конфигуратор игровой модели.
		* Подготавливает игровую модель по общим данным.
		* Запрашивает конфигурацию дополнительных элементов у блока парсеров карт и их содержимого.
		*/
		model::Game ParseGameBaseConfig(json::object&& config);

	} // namespace detail

	model::Game LoadGameConfiguration(const std::filesystem::path& json_path);

}  // namespace json_loader