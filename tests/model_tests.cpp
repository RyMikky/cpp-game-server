#include <cmath>
#include <vector>
#include <catch2/catch_test_macros.hpp>

#include "../src/model.h"

using namespace std::literals;

SCENARIO("Model test module") {


	using model::Road;
	using model::Office;
	using model::Building;

	using model::Map;

	using model::Game;

	GIVEN("a Game model") {

		Game game;

		AND_GIVEN ("a new map - map1") {

			Map map(Map::Id{ "map1" }, "map1");

			THEN ("we can make new horizontal road") {

				Road h_road(model::Road::HORIZONTAL, { 0, 0 }, 10);

				THEN("we can add road on map and check that") {
					map.AddRoad(h_road);
					CHECK(map.GetRoads().size() == 1);
				}

				AND_THEN("we can make new vertical road, add to map, and check that") {
					Road v_road(Road::VERTICAL, { 0, 0 }, 10);
					map.AddRoad(v_road);
					CHECK(map.GetRoads().size() == 1);
				}
			}

			AND_THEN("we can set default dog speed on map, and check that") {
				map.SetOnMapSpeed(10);
				CHECK(map.GetOnMapSpeed() == 10);
			}

			AND_THEN("we can add to map some extra-data, for example std::vector<bool>") {

				std::vector<bool> raw_data = { false, false, true, false, true };
				std::vector<bool> raw_data_copy(raw_data);

				map.AddExtraData("ExtraVector", extra_data::ExtraDataType::template_array,
					std::move(std::make_shared<extra_data::ExtraTemplateArrayData<std::vector<bool>>>(std::move(raw_data))));

				CHECK(raw_data.size() == 0);

				THEN("we can check was extra data base have 1 item") {
					CHECK(map.GetExtraDataCount() == 1);
				}

				AND_THEN("we can give back the extra data as ExtraDataTemplateArray<T>") {
					auto extra_data = map.GetExtraDataAsArray<std::vector<bool>>("ExtraVector");

					THEN("we can give back our std::vector<bool> and check the data") {
						auto returned_data = extra_data->Data();

						for (size_t i = 0; i != raw_data_copy.size(); ++i) {
							CHECK(raw_data_copy[i] == returned_data[i]);
						}
					}
				}
			}

			AND_THEN("we can add to map some extra-data, for example std::vector<bool>") {

				std::vector<bool> raw_data = { false, false, true, false, true };
				std::vector<bool> raw_data_copy(raw_data);

				map.AddExtraArrayData("ExtraVector", std::move(raw_data));

				CHECK(raw_data.size() == 0);

				THEN("we can check was extra data base have 1 item") {
					CHECK(map.GetExtraDataCount() == 1);
				}

				AND_THEN("we can give back the extra data as ExtraDataTemplateArray<T>") {
					auto extra_data = map.GetExtraDataAsArray<std::vector<bool>>("ExtraVector");

					THEN("we can give back our std::vector<bool> and check the data") {
						auto returned_data = extra_data->Data();

						for (size_t i = 0; i != raw_data_copy.size(); ++i) {
							CHECK(raw_data_copy[i] == returned_data[i]);
						}

					}
				}

				AND_THEN("we can add someone extra data with same name and give exception") {

					CHECK(map.GetExtraDataCount() == 1);
					std::vector<bool> new_raw_data = { true, true, false, true, false };
					CHECK_THROWS_AS(map.AddExtraArrayData("ExtraVector", std::move(new_raw_data)), std::invalid_argument);

					AND_THEN("raw data will be in safe") {
						CHECK(new_raw_data.size());
						CHECK(map.GetExtraDataCount() == 1);
					}

					AND_THEN("we can give back our std::vector<bool> and check the data") {
						auto returned_data = map.GetRawExtraDataAs<std::vector<bool>>("ExtraVector");

						for (size_t i = 0; i != raw_data_copy.size(); ++i) {
							CHECK(raw_data_copy[i] == returned_data[i]);
						}
					}
				}
			}
		}
	}
}