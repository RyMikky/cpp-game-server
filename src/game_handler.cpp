#include "game_handler.h"
#include <boost/asio.hpp>
#include <algorithm>

namespace game_handler {

	namespace fs = std::filesystem;
	namespace json = boost::json;

	size_t TokenHasher::operator()(const Token& token) const noexcept {
		// Возвращает хеш значения, хранящегося внутри token
		return _hasher(*token);
	}

	size_t TokenPtrHasher::operator()(const Token* token) const noexcept {
		// Возвращает хеш значения, хранящегося внутри token
		return _hasher(*(*token));
	}

	// -------------------------- class GameSession --------------------------

	// отвечает есть ли в сессии свободное местечко
	bool GameSession::have_free_space() {
		// смотрим есть ли место в текущей игровой сессии
		auto id = std::find(players_id_.begin(), players_id_.end(), false);

		return id != players_id_.end();
	}

	Player* GameSession::add_new_player(std::string_view name) {
		// смотрим есть ли место в текущей игровой сессии
		auto id = std::find(players_id_.begin(), players_id_.end(), false);
		if (id != players_id_.end()) {
			// если место есть, то запрашиваем уникальный токен
			const Token* player_token = game_handler_.get_unique_token(shared_from_this());
			// создаём игрока в текущей игровой сессии
			session_players_[player_token] = 
				std::move(Player{ uint16_t(std::distance(players_id_.begin(), id)), name, player_token });
			// делаем пометку в булевом массиве
			*id = true;

			// возвращаем игрока по токену
			return get_player_by_token(player_token);
		}
		else {
			return nullptr;
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

	// Возвращает ответ на запрос о списке игроков в данной сессии
	http_handler::Response GameHandler::player_list_response(http_handler::StringRequest&& req) {

		if (req.method_string() != http_handler::Method::GET && req.method_string() != http_handler::Method::HEAD) {
			// если у нас ни гет и ни хед запрос, то кидаем отбойник
			return method_not_allowed_impl(std::move(req), http_handler::Method::GET, http_handler::Method::HEAD);
		}

		try
		{
			// ищем тушку авторизации среди хеддеров запроса
			auto auth_iter = req.find("Authorization");
			if (auth_iter == req.end()) {
				// если нет тушки по авторизации, тогда кидаем отбойник
				return unauthorized_response(std::move(req),
					"invalidToken"sv, "Authorization header is missing"sv);
			}

			// из тушки запроса получаем строку
			// так как строка должна иметь строгий вид Bearer <токен>, то мы легко можем распарсить её
			std::string bearer{ auth_iter->value().begin(), auth_iter->value().begin() + 6 };
			std::string token{ auth_iter->value().begin() + 7, auth_iter->value().end() };

			if (bearer != "Bearer" || token.size() == 0) {
				// если нет строки Bearer, или она корявая, или токен пустой, то кидаем отбойник
				return unauthorized_response(std::move(req),
					"invalidToken"sv, "Authorization header is missing"sv);
			}

			Token req_token{ token }; // создаём быстро токен на основе запроса и ищем совпадение во внутреннем массиве
			if (!tokens_list_.count(req_token)) {
				// если заголовок Authorization содержит валидное значение токена, но в игре нет пользователя с таким токеном
				return unauthorized_response(std::move(req),
					"unknownToken"sv, "Player token has not been found"sv);
			}
			else {
				// если токен таки есть, тогда отправляемся в непосредственную имплементацию метода
				// если токен таки есть, тогда уже берем сессию, где он "висит" и запрашиваем инфу о всех подключенных игроках
				return player_list_response_impl(std::move(req), std::move(req_token));
			}
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error("GameHandler::player_list_response::error" + std::string(e.what()));
		}
	}
	// Возвращает ответ на запрос по присоединению к игре
	http_handler::Response GameHandler::join_game_response(http_handler::StringRequest&& req) {

		if (req.method_string() != http_handler::Method::POST) {
			// сюда вставить респонс о недопустимом типе
			return method_not_allowed_impl(std::move(req), http_handler::Method::POST);
		}

		try
		{
			// ищем тушку среди хеддеров запроса
			auto body_iter = req.find("Body");
			if (body_iter == req.end()) {
				// если нет тела запроса, тогда запрашиваем
				return bad_request_response(std::move(req), 
					"invalidArgument"sv, "Header body whit two arguments <userName> and <mapId> expected"sv);
			}
			// из тушки запроса получаем строку
			std::string body_string{ body_iter->value().begin(), body_iter->value().end() };
			// парсим тело запроса, все исключения в процессе будем ловить в catch_блоке
			json::value req_data = json_detail::ParseTextToBoostJson(body_string);

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
			else {
				// если карта есть и мы получили указатель
				// передаем управление основной имплементации
				return join_game_response_impl(std::move(req), std::move(req_data), map);
			}
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error("GameHandler::join_game_response::error" + std::string(e.what()));
		}
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
		response.body() = json_detail::GetErrorString(code, message);

		return response;
	}
	// Возвращает ответ, что запрос не прошёл валидацию
	http_handler::Response GameHandler::unauthorized_response(
		http_handler::StringRequest&& req, std::string_view code, std::string_view message) {
		http_handler::StringResponse response(http::status::unauthorized, req.version());
		response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
		response.set(http::field::cache_control, "no-cache");
		response.body() = json_detail::GetErrorString(code, message);

		return response;
	}

	const Token* GameHandler::get_unique_token(std::shared_ptr<GameSession> session) {

		// вариант получения с помощью контекста

		//const Token* result = nullptr;
		//boost::asio::post(strand_, 
		//	// получаем токен в стредне с помощью имплементации в приватной области класса
		//	[this, session, &result]() { result = this->get_unique_token_impl(session); });

		//return result;

		// вариант получения с помощью мьютекса

		std::lock_guard func_lock_(mutex_);
		return get_unique_token_impl(session);
	}

	const Token* GameHandler::get_unique_token_impl(std::shared_ptr<GameSession> session) {

		bool isUnique = true;         // создаём реверсивный флаг
		Token unique_token{ "" };       // создаём токен-болванку

		while (isUnique)
		{
			// генерируем новый токен
			unique_token = Token{ detail::GenerateToken32Hex() };
			// если сгенерированный токен уже есть, то флаг так и останется поднятым и цикл повторится
			isUnique = tokens_list_.count(unique_token);
		}

		auto insert = tokens_list_.insert({ unique_token,  session });
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

		if (tokens_list_.count(remove)) {
			tokens_list_.at(remove)->remove_player(&remove);
			return tokens_list_.erase(remove);
		}
		else {
			return false;
		}
	}


	// Возвращает ответ на запрос о списке игроков в данной сессии
	http_handler::Response GameHandler::player_list_response_impl(http_handler::StringRequest&& req, Token&& token) {

		// получаем сессию где на данный момент "висит" указанный токен
		std::shared_ptr<GameSession> session = tokens_list_.at(token);

		// подготавливаем и возвращаем ответ
		http_handler::StringResponse response(http::status::ok, req.version());
		response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
		response.set(http::field::cache_control, "no-cache");
		// заполняем тушку ответа с помощью жисонского метода
		response.body() = json_detail::GetSessionPlayersList(session->cbegin(), session->cend());

		return response;
	}
	// Возвращает ответ, о успешном добавлении игрока в игровую сессию
	http_handler::Response GameHandler::join_game_response_impl(http_handler::StringRequest&& req, json::value&& body, const model::Map* map) {
		
		Player* new_player = nullptr;        // заготовка под нового челика на сервере
		std::shared_ptr<GameSession> ref;    // заготовка под указатель на конкретную игровую сессию

		// смотрим есть ли на данный момент открытый игровой инстанс по данной карте
		if (instances_.count(map)) {
			// если инстанс есть, получаем его данные
			auto instance = instances_.at(map);
			// начинаем опрашивать все внутренние сессии в инстансе на предмет наличия свободного места
			// так как инстанс не создаётся без нужды, то хотя бы одна игровая сессия быть должна

			bool have_a_plance = false;      // вводим переменную для отслеживания наличия мест в текущих сессиях
			for (auto& item : instance) {
				// если нашли свободное место
				if (item->have_free_space()) {

					ref = item;              // записываем ссылку на шару сессии
					have_a_plance = true;    // ставим флаг, что место нашли
					break;                   // завершаем цикл за ненадобностью
				}
			}

			// если место было найдено, то ничего делать не надо - ref у нас есть, ниже по коду будет добавлен челик и создан ответ
			if (!have_a_plance) {
				// если же мест в текущих открытых сессиях НЕ найдено, ну вот нету, значит надо открыть новую
				ref = instances_.at(map)
					.emplace_back(std::make_shared<GameSession>(*this, map, 8));
			}
		}

		// если нет ни одной открытой сессии на данной карте
		else {
			// добавляем указатель и создаём инстанс со списком сессий
			instances_.insert(std::make_pair(map, GameInstance()));
			// так как у нас еще нет никакие сессий то тупо создаём новую внутри 
			// c максимумом, для примера, в 8 игроков (см. конструктор GameSession)
			// тут же получаем шару на неё, и передаем ей управление по вступлению в игру
			ref = instances_.at(map)
				.emplace_back(std::make_shared<GameSession>(*this, map, 8));
		}

		// добавляем челика на сервер и принимаем на него указатель
		new_player = ref->add_new_player(body.at("userName").as_string());

		// подготавливаем и возвращаем ответ
		http_handler::StringResponse response(http::status::ok, req.version());
		response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
		response.set(http::field::cache_control, "no-cache");
		response.body() = json_detail::GetJoinPlayerString(new_player);

		return response;
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