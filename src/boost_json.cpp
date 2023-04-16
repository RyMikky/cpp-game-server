#include "boost_json.h"

namespace json_detail {
	
	// возвращает json-словарь с информацией по коду и сообщению о ошибке
	json::value GetErrorValue(std::string_view code, std::string_view message) {
		return json::object { 
			{"code", code.data()}, 
			{"message", message.data()} 
		};
	}

	// возвращает строковое представление json-словаря с информацией по коду и сообщению о ошибке
	std::string GetErrorString(std::string_view code, std::string_view message) {
		return json::serialize(GetErrorValue(code, message));
	}

	// парсер входящей строки из текста в boost::json
	json::value ParseTextToBoostJson(std::string_view line) {
		return json::parse(line.data());
	}

	// возвращает строковое представление json-массива с полной информацией по вектору карт игры
	std::string GetMapList(const std::vector<model::Map>& maps) {

		json::array result;   // по условию требуется запихать в массив

		for (const auto& map : maps) {
			// бежим по листу и добавляем в созданый массив элементы
			result.push_back(
				json::object {
					{"id", *map.GetId()},
					{"name", map.GetName()}
				}
			);
		}
		return json::serialize(result);
	}

	// возвращает json-массив с информацией о офисах по запрошенной карте
	json::array GetMapOffices(const model::Map* data) {
		json::array result;

		// бежим по массиву офисов
		for (auto& office : data->GetOffices()) {

			// записываем параметры офиса
			json::object pre_result {
				{"id", *office.GetId()},
				{"x", office.GetPosition().x},
				{"y", office.GetPosition().y},
				{"offsetX", office.GetOffset().dx},
				{"offsetY", office.GetOffset().dy}
			};

			result.push_back(pre_result);
		}

		return result;
	}

	// возвращает json-массив с информацией о строениях по запрошенной карте
	json::array GetMapBuilds(const model::Map* data) {
		json::array result;

		// бежим по массиву строений
		for (auto& build : data->GetBuildings()) {

			// записываем параметры строения
			json::object pre_result{
				{"x", build.GetBounds().position.x},
				{"y", build.GetBounds().position.y},
				{"w", build.GetBounds().size.width},
				{"h", build.GetBounds().size.height}
			};

			result.push_back(pre_result);
		}

		return result;
	}

	// возвращает json-массив с информацией о дорогах по запрошенной карте
	json::array GetMapRoads(const model::Map* data) {
		json::array result;

		// бежим по массиву дорог
		for (auto& road : data->GetRoads()) {
			
			// записываем координаты начала дороги
			json::object pre_result{
				{"x0", road.GetStart().x},
				{"y0", road.GetStart().y},
			};

			// добавляем координату конца дороги
			if (road.IsHorizontal()) {
				pre_result.emplace("x1", road.GetEnd().x);
			}
			else {
				pre_result.emplace("y1", road.GetEnd().y);
			}

			result.push_back(pre_result);
		}

		return result;
	}

	// возвращает строковое представление json-словаря с полной информацией по запрошенной карте
	std::string GetMapInfo(const model::Map* data) {

		json::object result;          // базовый ресурс ответа

		result.emplace("id", *data->GetId());
		result.emplace("name", data->GetName());

		result.emplace("roads", GetMapRoads(data));
		result.emplace("buildings", GetMapBuilds(data));
		result.emplace("offices", GetMapOffices(data));

		return json::serialize(result);
	}

	// возвращает строкове предаставление json-словаря с информацией о новом загруженном игроке
	std::string GetNewPlayerInfo(game_handler::PlayerPtr player) {
		json::object result;          // базовый ресурс ответа

		result.emplace("authToken", std::string(player->get_player_token()));
		result.emplace("playerId", (int)player->get_player_id());

		return json::serialize(result);
	}

} // namespace json_detail