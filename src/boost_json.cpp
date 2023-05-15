#include "boost_json.h"

namespace json_detail {

	// парсер входящей строки из текста в boost::json
	json::value ParseTextToJSON(std::string_view line) {
		return json::parse(line.data());
	}

	// возвращает строковое представление json-словаря с информацией по конкретному аргументу
	std::string GetDebugArgument(std::string_view argument, std::string_view value) {
		return json::serialize(detail::GetDebugArgument(argument, value));
	}

	// возвращает строковое представление json-словаря с информацией по коду и сообщению о ошибке
	std::string GetErrorString(std::string_view code, std::string_view message) {
		return json::serialize(detail::GetErrorValue(code, message));
	}

	// возвращает строковое представление json-словаря с полной информацией по запрошенной карте
	std::string GetMapInfo(const model::Map* data) {

		json::object result;          // базовый ресурс ответа

		result.emplace("id", *data->GetId());
		result.emplace("name", data->GetName());

		/*result.emplace("lootTypes", detail::GetMapLootTypes(data));

		auto d = data->GetExtraData("lootTypes");
		auto json = d->AsArray<boost::json::array>()->Data();*/

		/*result.emplace("lootTypes",
			data->GetExtraData("lootTypes")->AsArray<boost::json::array>()->Data());*/

		/*result.emplace("lootTypes",
			data->GetExtraDataAsArray<boost::json::array>("lootTypes")->Data());*/

		result.emplace("lootTypes",
			data->GetRawExtraDataAs<boost::json::array>("lootTypes"));

		result.emplace("roads", detail::GetMapRoads(data));
		result.emplace("buildings", detail::GetMapBuilds(data));
		result.emplace("offices", detail::GetMapOffices(data));

		return json::serialize(result);
	}

	// возвращает строковое представление json-массива с полной информацией по вектору карт игры
	std::string GetMapsList(const std::vector<model::Map>& maps) {

		json::array result;   // по условию требуется запихать в массив

		for (const auto& map : maps) {
			// бежим по листу и добавляем в созданый массив элементы
			result.push_back(
				json::object{
					{"id", *map.GetId()},
					{"name", map.GetName()}
				}
			);
		}
		return json::serialize(result);
	}

	// ----------------- блок методов генерации тела ответов на запросы к api-сервера -----------------

	// возвращает строкове предаставление json-словаря с информацией о новом загруженном игроке
	std::string GetSessionPlayerJoin(game_handler::PlayerPtr player) {
		json::object result;          // базовый ресурс ответа

		result.emplace("authToken", std::string(player->GetPlayerToken()));
		result.emplace("playerId", (int)player->GetPlayerId());

		return json::serialize(result);
	}

	// возвращает строковое представление json_словаря с информацией о всех игроках в указанной сессии
	std::string GetSessionPlayersList(game_handler::SPIterator begin, game_handler::SPIterator end) {

		json::object result;          // базовый ресурс ответа

		for (game_handler::SPIterator it = begin; it != end; it++) {

			result.emplace(
				std::to_string(it->second.GetPlayerId()),
				json::object{ {"name", it->second.GetPlayerName()} }
			);
		}

		return json::serialize(result);
	}

	// возвращает строковое представление json_словаря с информацией о всех игроках в указанной сессии
	std::string GetSessionPlayersList(const game_handler::SessionPlayers& players) {

		json::object result;          // базовый ресурс ответа

		for (const auto& it : players) {

			result.emplace(
				std::to_string(it.second.GetPlayerId()),
				json::object{ {"name", it.second.GetPlayerName()} }
			);
		}

		return json::serialize(result);
	}

	// возвращает строковое представление json_словаря с информацией о состоянии в указанной сессии
	std::string GetSessionStateList(const game_handler::SessionPlayers& players, const game_handler::SessionLoots& loots) {

		json::object players_list;                  // базовый словарь с данными о игроках

		for (const auto& it : players) {

			json::object player_data;               // объект игрок в котором будут все данные

			json::array pos {                       // данные по позиции
				it.second.GetPlayerPosition().x_, 
				it.second.GetPlayerPosition().y_ };

			json::array speed{                      // данные по скорости
				it.second.GetPlayerSpeed().xV_,
				it.second.GetPlayerSpeed().yV_ };

			player_data.emplace("pos", pos);
			player_data.emplace("speed", speed);

			switch (it.second.GetPlayerDirection())
			{
			default:
				case game_handler::PlayerDirection::NORTH:
					player_data.emplace("dir", "U");
					break;
				case game_handler::PlayerDirection::SOUTH:
					player_data.emplace("dir", "D");
					break;
				case game_handler::PlayerDirection::WEST:
					player_data.emplace("dir", "L");
					break;
				case game_handler::PlayerDirection::EAST:
					player_data.emplace("dir", "R");
					break;
				break;
			}
			
			players_list.emplace(
				std::to_string(it.second.GetPlayerId()), player_data);
		}

		json::object loots_list;                    // базовый словарь с данными о луте в сессии

		for (const auto& it : loots) {

			json::array pos { it.second.pos_.x_, it.second.pos_.y_ };    // данные по позиции
			json::object loot_data{ {"type", it.second.type_}, {"pos", pos} };  // данные по луту

			// записываем данные о луте
			loots_list.emplace(std::to_string(it.first), loot_data);
		}

		return json::serialize(json::object{ {"players", players_list}, {"lostObjects", loots_list } });
	}

	namespace detail {

		// возвращает json-словарь с информацией по конкретному аргументу
		json::value GetDebugArgument(std::string_view argument, std::string_view value) {
			return json::object{
				{"argument", argument.data()},
				{"value", value.data()}
			};
		}

		// возвращает json-словарь с информацией по коду и сообщению о ошибке
		json::value GetErrorValue(std::string_view code, std::string_view message) {
			return json::object{
				{"code", code.data()},
				{"message", message.data()}
			};
		}

		// возвращает json-массив с информацией о типах лута по запрошенной карте
		json::value GetMapLootTypes(const model::Map* data) {
			json::array result;

			// бежим по массиву типов лута
			for (auto& loot_type : data->GetLootTypes()) {
			
				// записываем параметры типа лута
				json::object pre_result{
					{ "name", loot_type.GetName() },
					{ "file", loot_type.GetFile() },
					{ "type", loot_type.GetType() },
					{ "rotation", loot_type.GetRotation() },
					{ "color", loot_type.GetColor() },
					{ "scale", loot_type.GetScale() }
				};

				result.push_back(pre_result);
			}

			return result;
		}

		// возвращает json-массив с информацией о офисах по запрошенной карте
		json::array GetMapOffices(const model::Map* data) {
			json::array result;

			// бежим по массиву офисов
			for (auto& office : data->GetOffices()) {

				// записываем параметры офиса
				json::object pre_result{
					{ "id", *office.GetId() },
					{ "x", office.GetPosition().x },
					{ "y", office.GetPosition().y },
					{ "offsetX", office.GetOffset().dx },
					{ "offsetY", office.GetOffset().dy }
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
					{ "x", build.GetBounds().position.x },
					{ "y", build.GetBounds().position.y },
					{ "w", build.GetBounds().size.width },
					{ "h", build.GetBounds().size.height }
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
					{ "x0", road.GetStart().x },
					{ "y0", road.GetStart().y },
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

	} // namespace detail

} // namespace json_detail