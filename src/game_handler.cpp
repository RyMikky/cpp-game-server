#include "game_handler.h"
#include <boost/asio.hpp>
#include <algorithm>

namespace game_handler {

	namespace fs = std::filesystem;

	size_t TokenHasher::operator()(const Token& token) const noexcept {
		// Возвращает хеш значения, хранящегося внутри token
		return _hasher(*token);
	}

	size_t TokenPtrHasher::operator()(const Token* token) const noexcept {
		// Возвращает хеш значения, хранящегося внутри token
		return _hasher(*(*token));
	}

	// -------------------------- class GameSession --------------------------

	bool GameSession::add_new_player(std::string_view name) {
		// смотрим есть ли место в текущей игровой сессии
		auto id = std::find(players_id_.begin(), players_id_.end(), false);
		if (id != players_id_.end()) {
			// если место есть, то запрашиваем уникальный токен
			const Token* player_token = game_handler_.get_unique_token(shared_from_this());
			// создаём игрока в текущей игровой сессии
			session_players_[player_token] = 
				std::move(Player{ uint16_t(std::distance(players_id_.begin(), id)), name, player_token });
			// делаем пометку в булевом массиве
			return	*id = true;
		}
		else {
			return false;
		}
	}

	bool GameSession::remove_player(const Token* token) {
		if (!session_players_.count(token)) {
			return false;
		}
		else {
			// освобождаем id текущего игрока по токену
			players_id_[session_players_.at(token).get_player_id()] = false;
			// удаляем запись о игроке вместе со структурой
			session_players_.erase(token);
			return true;
		}
	}

	Player* GameSession::get_player_by_token(const Token* token) {
		if (session_players_.count(token)) {
			return &session_players_.at(token);
		}
		return nullptr;
	}

	// -------------------------- class GameHandler --------------------------

	size_t MapPtrHasher::operator()(const model::Map* map) const noexcept {
		// Возвращает хеш значения, хранящегося внутри map
		return _hasher(map->GetName()) + _hasher(*(map->GetId()));
	}

	// Обеспечивает вход игрока в игровую сессию
	std::string enter_to_game_session(std::string_view name, std::string_view map) {
		return {};
	}

	// Возвращает ответ на запрос по присоединению к игре
	http_handler::Response GameHandler::join_game_response(http_handler::StringRequest&& req) {

		if (req.method_string() != http_handler::Method::POST) {
			// сюда вставить респонс о недопустимом типе
			return method_not_allowed_impl(std::move(req), http_handler::Method::POST);
		}

		try
		{

			/*for (const auto& header : req) {
				std::cout << "  "sv << header.name_string() << ": "sv << header.value() << std::endl;
			}*/

			auto body_iter = req.find("Body");

			if (body_iter == req.end()) {
				// если нет тела запроса, тогда запрашиваем
				return bad_request_response(std::move(req), 
					"invalidArgument"sv, "Header body whit two arguments <userName> and <mapId> expected"sv);
			}
			std::string body_string{ body_iter->value().begin(), body_iter->value().end() };

			// парсим тело запроса, все исключения в процессе будем ловить в catch_блоке
			boost::json::value req_data = json_detail::ParseTextToBoostJson(body_string);

			// если в блоке вообще нет графы "userName" или "mapId"
			if (!req_data.as_object().count("userName") || !req_data.as_object().count("mapId")) {
				return bad_request_response(std::move(req), "invalidArgument"sv, "Two arguments <userName> and <mapId> expected"sv);
			}

			// если в "userName" пустота
			if (req_data.as_object().at("userName") == "") {
				return bad_request_response(std::move(req), "invalidArgument"sv, "Invalid name"sv);
			}

			// ищем запрошенную карту
			auto map = game_simple_.FindMap(
				model::Map::Id{ std::string(
					req_data.as_object().at("mapId").as_string()) });

			if (map == nullptr) {
				// если карта не найдена, то кидаем отбойник
				return map_not_found_response_impl(std::move(req));
			}
		}
		catch (const std::exception&)
		{

		}
		

		std::string player_name, game_map;
		
		return {};
	}
	// Возвращает ответ на запрос по поиску конкретной карты
	http_handler::Response GameHandler::find_map_response(http_handler::StringRequest&& req, std::string_view find_request_line) {

		// ищем запрошенную карту
		auto map = game_simple_.FindMap(model::Map::Id{ std::string(find_request_line) });

		if (map == nullptr) {
			// если карта не найдена, то кидаем отбойник
			return map_not_found_response_impl(std::move(req));
		}
		else {

			http_handler::StringResponse response(http::status::ok, req.version());
			response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
			response.set(http::field::cache_control, "no-cache");

			// загружаем тело ответа из жидомасонского блока по полученному выше блоку параметров карты
			response.body() = json_detail::GetMapInfo(map);

			return response;
		}

	}
	// Возвращает ответ со списком загруженных карт
	http_handler::Response GameHandler::map_list_response(http_handler::StringRequest&& req) {
		http_handler::StringResponse response(http::status::ok, req.version());
		response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
		response.set(http::field::cache_control, "no-cache");
		response.body() = json_detail::GetMapList(game_simple_.GetMaps());

		return response;
	}
	// Возвращает ответ, что запрос некорректный
	http_handler::Response GameHandler::bad_request_response(
		http_handler::StringRequest&& req, std::string_view code, std::string_view message) {
		http_handler::StringResponse response(http::status::bad_request, req.version());
		response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
		response.set(http::field::cache_control, "no-cache");
		response.body() = json_detail::GetErrorString(code, message/*"badRequest"sv, "Bad request"sv*/);

		return response;
	}

	const Token* GameHandler::get_unique_token(std::shared_ptr<GameSession> session) {

		const Token* result = nullptr;
		boost::asio::post(strand_, 
			// получаем токен в стредне с помощью имплементации в приватной области класса
			[this, session, &result]() { result = get_unique_token_impl(session); });

		return result;
	}

	const Token* GameHandler::get_unique_token_impl(std::shared_ptr<GameSession> session) {

		bool isUnique = true;         // создаём реверсивный флаг
		Token unique_token{ "" };       // создаём токен-болванку

		while (isUnique)
		{
			// генерируем новый токен
			unique_token = Token{ detail::GenerateToken32Hex() };
			// если сгенерированный токен уже есть, то флаг так и останется поднятым и цикл повторится
			isUnique = token_list_.count(unique_token);
		}

		auto insert = token_list_.insert({ unique_token,  session });
		return &(insert.first->first);
	}

	bool GameHandler::reset_token(std::string_view token) {

		bool result = false;
		boost::asio::post(strand_,
			// удаляем с помощью приватной имплементации
			[this, token, &result]() { result = reset_token_impl(token); });
		return result;
	}

	bool GameHandler::reset_token_impl(std::string_view token) {
		Token remove{ std::string(token) };

		// TODO Заглушка, скорее всего корректно работать не будет, надо переделать + удаление в GameSession

		if (token_list_.count(remove)) {
			token_list_.at(remove)->remove_player(&remove);
			return token_list_.erase(remove);
		}
		else {
			return false;
		}
	}

	// Возвращает ответ, что упомянутая карта не найдена
	http_handler::Response GameHandler::map_not_found_response_impl(http_handler::StringRequest&& req) {
		http_handler::StringResponse response(http::status::not_found, req.version());
		response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
		response.set(http::field::cache_control, "no-cache");
		response.body() = json_detail::GetErrorString("mapNotFound"sv, "Map not found"sv);

		return response;
	}

	// Возвращает ответ, что запрошенный метод не ражрешен, доступный указывается в аргументе allow
	http_handler::Response GameHandler::method_not_allowed_impl(http_handler::StringRequest&& req, std::string_view allow) {
		http_handler::StringResponse response(http::status::method_not_allowed, req.version());
		response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
		response.set(http::field::cache_control, "no-cache");
		response.set(http::field::allow, allow);
		response.body() = json_detail::GetErrorString("invalidMethod"sv, ("Only "s + std::string(allow) + " method is expected"s));

		return response;
	}
	
} //namespace game_handler