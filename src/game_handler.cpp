#include "game_handler.h"
#include <boost/asio.hpp>
#include <algorithm>

namespace game_handler {

	namespace fs = std::filesystem;
	namespace json = boost::json;

	// -------------------------- class GameSession --------------------------
	

	// задаёт флаг случайной позиции для старта новых игроков
	GameSession& GameSession::set_start_random_position(bool start_random_position) {
		start_random_position_ = start_random_position;
		return *this;
	}
	// добавляет нового игрока на случайное место на случайной дороге на карте
	Player* GameSession::add_new_player(std::string_view name) {
		// смотрим есть ли место в текущей игровой сессии
		auto id = std::find(players_id_.begin(), players_id_.end(), false);
		if (id != players_id_.end()) {
			// если место есть, то запрашиваем уникальный токен
			const Token* player_token = game_handler_.get_unique_token(shared_from_this());

			PlayerPosition position{0.0, 0.0};                 // заготовка под позицию установки нового игрока
			// если активирован флаг получения случайной позиции на старте иначе будет старт на точке {0.0, 0.0}
			if (start_random_position_) {
				// чтобы получить случайную позицию на какой-нибудь дороге на карте начнём цикл поиска места
				bool findPlase = true;        // реверсивный флаг, который сделаем false, когда найдём место
				while (findPlase)
				{
					// спрашиваем у карты случайную точку на какой-нибудь дороге
					model::Point point = session_game_map_->get_random_road_position();
					// переводим точку в позицию
					position.x_ = point.x;
					position.y_ = point.y;
					// проверяем на совпадение позиции с уже имеющимися игроками и инвертируем вывод метода
					findPlase = !start_position_check_impl(position);
				}
			}
			
			// создаём игрока в текущей игровой сессии
			session_players_[player_token] = std::move(
				Player{ uint16_t(std::distance(players_id_.begin(), id)), name, player_token }
					.set_position(std::move(position))               // назначаем стартовую позицию
					.set_direction(PlayerDirection::NORTH)                  // назначаем дефолтное направление взгляда
					.set_speed({ 0, 0 }));                            // назначаем стартовую скорость
					
			// делаем пометку в булевом массиве
			*id = true;

			// возвращаем игрока по токену
			return get_player_by_token(player_token);
		}
		else {
			return nullptr;
		}
	}
	// вернуть указатель на игрока в сессии по токену
	Player* GameSession::get_player_by_token(const Token* token) {
		if (session_players_.count(token)) {
			return &session_players_.at(token);
		}
		return nullptr;
	}

	// удалить игрока из игровой сессии
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
	// обновляет состояние игры с заданным временем в миллисекундах
	bool GameSession::update_state(int time) {
		try
		{
			// просто перебираем всех игроков в сессии
			for (auto& [token, player] : session_players_) {
				// обновляет позицию выбранного игрока в соответствии с его заданной скоростью, направлением и временем в секундах
				set_player_new_position(player, (double)time / 1000);
			}

			return true;
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error("GameSession::update_state::Error::" + std::string(e.what()));
		}
	}
	// метод добавляет скорость персонажу, вызывается из GameHandler::player_action_response_impl
	bool GameSession::move_player(const Token* token, PlayerMove move) {

		switch (move)
		{
		case game_handler::PlayerMove::UP: // верх скорость {0, -speed}
			get_player_by_token(token)->set_speed(0, -session_game_map_->get_dog_speed()).set_direction(PlayerDirection::NORTH);
			return true;

		case game_handler::PlayerMove::DOWN: // вниз скорость {0, speed}
			get_player_by_token(token)->set_speed(0, session_game_map_->get_dog_speed()).set_direction(PlayerDirection::SOUTH);
			return true;

		case game_handler::PlayerMove::LEFT: // влево скорость {-speed, 0}
			get_player_by_token(token)->set_speed(-session_game_map_->get_dog_speed(), 0).set_direction(PlayerDirection::WEST);
			return true;

		case game_handler::PlayerMove::RIGHT: // влево скорость {speed, 0}
			get_player_by_token(token)->set_speed(session_game_map_->get_dog_speed(), 0).set_direction(PlayerDirection::EAST);
			return true;

		case game_handler::PlayerMove::STAY:
			get_player_by_token(token)->set_speed(0, 0);
			return true;

		case game_handler::PlayerMove::error:
			throw std::runtime_error("GameSession::move_player::PlayerMove::error");

		default:
			return false;
		}
	}
	// отвечает есть ли в сессии свободное местечко
	bool GameSession::have_free_space() {
		// смотрим есть ли место в текущей игровой сессии
		auto id = std::find(players_id_.begin(), players_id_.end(), false);

		return id != players_id_.end();
	}

	// изменяет координаты игрока при движении параллельно дороге, на которой он стоит
	bool GameSession::player_parallel_moving_impl(Player& player, PlayerDirection direction, 
		PlayerPosition&& from, PlayerPosition&& to, const model::Road* road) {
		
		bool playerKeepMoving = true;                   // флаг продолжения движения игрока 
		double limit_dy_ = 0u;                          // заготовка под лимит по оси Y
		double limit_dx_ = 0u;                          // заготовка под лимит по оси X

		switch (direction)
		{
		// в данном кейсе координата PlayerPosition to.y_ должна быть меньше PlayerPosition from.y_
		case game_handler::PlayerDirection::NORTH:
			// необходимо убедиться, что мы не поднимемся выше допустимого лимита
			// берем наименьшую координату вертикальной дороги по оси Y и вычитаем дельту
			limit_dy_ = static_cast<double>(std::min(road->GetStart().y, road->GetEnd().y)) - __ROAD_DELTA__;
			if (to.y_ <= limit_dy_) {
				// если приращение меньше максимально допустимого (максимальный отступ от оси дороги вверх)
				to.y_ = limit_dy_;                      // то просто меняем, приращение по вертикальной оси
				playerKeepMoving = false;               // снимаем флаг продолжения движения
			}

			break;
		// в данном кейсе координата PlayerPosition to.y_ должна быть больше PlayerPosition from.y_
		case game_handler::PlayerDirection::SOUTH:
			// необходимо убедиться, что мы не опускаемся ниже допустимого лимита
			// берем наивысшую координату вертикальной дороги по оси Y и прибавляем дельту
			limit_dy_ = static_cast<double>(std::max(road->GetStart().y, road->GetEnd().y)) + __ROAD_DELTA__;
			if (to.y_ >= limit_dy_) {
				// если приращение больше максимально допустимого (максимальный отступ от оси дороги вниз)
				to.y_ = limit_dy_;                      // то просто меняем, приращение по вертикальной оси
				playerKeepMoving = false;               // снимаем флаг продолжения движения
			}

			break;
		// в данном кейсе координата PlayerPosition to.x_ должна быть меньше PlayerPosition from.x_
		case game_handler::PlayerDirection::WEST:
			// необходимо убедиться, что мы не смещаемся левее допустимого лимита
			// берем наименьшую координату горизонтальной дороги по оси X и вычитаем дельту
			limit_dx_ = static_cast<double>(std::min(road->GetStart().x, road->GetEnd().x)) - __ROAD_DELTA__;
			if (to.y_ <= limit_dx_) {
				// если приращение меньше максимально допустимого (максимальный отступ от оси дороги влево)
				to.y_ = limit_dx_;                      // то просто меняем, приращение по горизонтальной оси
				playerKeepMoving = false;               // снимаем флаг продолжения движения
			}

			break;
		// в данном кейсе координата PlayerPosition to.x_ должна быть больше PlayerPosition from.x_
		case game_handler::PlayerDirection::EAST:
			// необходимо убедиться, что мы не смещаемся правее допустимого лимита
			// берем наибольшую координату горизонтальной дороги по оси X и прибавляем дельту
			limit_dx_ = static_cast<double>(std::max(road->GetStart().x, road->GetEnd().x)) + __ROAD_DELTA__;
			if (to.y_ >= limit_dx_) {
				// если приращение больше максимально допустимого (максимальный отступ от оси дороги влево)
				to.y_ = limit_dx_;                      // то просто меняем, приращение по горизонтальной оси
				playerKeepMoving = false;               // снимаем флаг продолжения движения
			}

			break;

		default:
			return false;
		}

		// записываем новые координаты и тормозим если необходимо
		playerKeepMoving ? player.set_position(std::move(to)) :
			player.set_position(std::move(to)).set_speed(0, 0);

		return true;
	}
	// изменяет координаты игрока при движении перпендикулярно дороге, на которой он стоит
	bool GameSession::player_cross_moving_impl(Player& player, PlayerDirection direction, 
		PlayerPosition&& from, PlayerPosition&& to, const model::Road* road) {
		
		bool playerKeepMoving = true;                   // флаг продолжения движения игрока 
		double limit_dy_ = 0u;                          // заготовка под лимит по оси Y
		double limit_dx_ = 0u;                          // заготовка под лимит по оси X
		
		switch (direction)
		{
		// в данном кейсе координата PlayerPosition to.y_ должна быть меньше PlayerPosition from.y_
		case game_handler::PlayerDirection::NORTH:
			// округляем y_ позиции игрока (from), до целого и вычитаем дельту отступа от центра дороги
			limit_dy_ = static_cast<double>(detail::double_round(from.y_)) - __ROAD_DELTA__;
			if (to.y_ <= limit_dy_) {
				// если приращение меньше максимально допустимого (максимальный отступ от оси дороги вверх)
				to.y_ = limit_dy_;                      // то просто меняем, приращение по вертикальной оси
				playerKeepMoving = false;               // снимаем флаг продолжения движения
			}
			break;

		// в данном кейсе координата PlayerPosition to.y_ должна быть больше PlayerPosition from.y_
		case game_handler::PlayerDirection::SOUTH:
			// округляем y_ позиции игрока (from), до целого и прибавляем дельту отступа от центра дороги
			limit_dy_ = static_cast<double>(detail::double_round(from.y_)) + __ROAD_DELTA__;
			if (to.y_ >= limit_dy_) {
				// если приращение больше максимально допустимого (максимальный отступ от оси дороги вниз)
				to.y_ = limit_dy_;                     // то просто меняем, приращение по вертикальной оси
				playerKeepMoving = false;              // снимаем флаг продолжения движения
			}
			break;

		// в данном кейсе координата PlayerPosition to.x_ должна быть меньше PlayerPosition from.x_
		case game_handler::PlayerDirection::WEST:
			// округляем x_ позиции игрока (from), до целого и вычитаем дельту отступа от центра дороги
			limit_dx_ = static_cast<double>(detail::double_round(from.x_)) - __ROAD_DELTA__;
			if (to.x_ <= limit_dx_) {
				// если приращение меньше максимально допустимого (максимальный отступ от оси дороги влево)
				to.x_ = limit_dx_;                     // то просто меняем, приращение по вертикальной оси
				playerKeepMoving = false;              // снимаем флаг продолжения движения
			}
			break;

		// в данном кейсе координата PlayerPosition to.x_ должна быть больше PlayerPosition from.x_
		case game_handler::PlayerDirection::EAST:
			// округляем x_ позиции игрока (from), до целого и прибавляем дельту отступа от центра дороги
			limit_dx_ = static_cast<double>(detail::double_round(from.x_)) + __ROAD_DELTA__;
			if (to.x_ >= limit_dx_) {
				// если приращение больше максимально допустимого (максимальный отступ от оси дороги влево)
				to.x_ = limit_dx_;                     // то просто меняем, приращение по вертикальной оси
				playerKeepMoving = false;              // снимаем флаг продолжения движения
			}
			break;

		default:
			return false;
		}

		// записываем новые координаты и тормозим если необходимо
		playerKeepMoving ? player.set_position(std::move(to)) :
			player.set_position(std::move(to)).set_speed(0, 0);

		return true;
	}

	bool GameSession::set_player_new_position(Player& player, double time) {

		// записываем вектор ожидаемого приращения по положению персонажа
		PlayerPosition delta_pos{ player.get_speed().xV_ * time, player.get_speed().yV_ * time, };
		// чтобы лишнего не считать, проверяем есть ли у нас какое-то приращение в принципе
		if (delta_pos.x_ == 0 && delta_pos.y_ == 0) {
			return true;           // если приращения нет, то сразу выходим и не продолжаем
		}

		const model::Road* road = nullptr;    // готовим заготовку под "дорогу"
		// записываем во временную переменную, чтобы не делать лишних вызовов
		PlayerDirection direction = player.get_direction();
		PlayerPosition position = player.get_position();
		// округляем позицию до уровня логики model::Map
		model::Point point{ detail::double_round(position.x_), detail::double_round(position.y_) };
		
		// в зависимости от нашего направления запрашиваем дорогу
		// если игрок смотрит влево или вправо, полагаем, что будет движение по горизонтальной дороге
		if (direction == PlayerDirection::WEST || direction == PlayerDirection::EAST) {	
			road = session_game_map_->stay_on_horizontal_road(point);     // зарос или вернет дорогу, или nullptr
		}
		// если игрок смотрит вниз или вверх, полагаем, что будет движение по вертикальной дороге
		else if (direction == PlayerDirection::NORTH || direction == PlayerDirection::SOUTH) {
			road = session_game_map_->stay_on_vertical_road(point);       // зарос или вернет дорогу, или nullptr
		}

		// тут важный момент, если мы стоим на требуемой для движения дороге,
		// то в принципе мы в состоянии спокойно двигаться проверив выход за границы дороги
		if (road) {
			// отдаём обработку методу перемещения параллельно дороге
			return player_parallel_moving_impl(player, direction, std::move(position), std::move(delta_pos), road);
		}
		// если же мы не стоим на требуемой - стоим на дороге перпендикулярной оси движения
		else {
			// отдаём обработку методу перемещения перпендикулярно дороге
			return player_cross_moving_impl(player, direction, std::move(position), std::move(delta_pos), road);
		}
	}
	// чекает стартовую позицию на предмет совпадения с другими игроками в сессии
	bool GameSession::start_position_check_impl(PlayerPosition& position) {

		for (const auto& item : session_players_) {
			// если нашли поцизию совпадающую с запрошенной то выходим с false
			if (position == item.second.get_position()) {
				return false;
			}
		}
		return true;
	}

	// -------------------------- class GameHandler --------------------------

	size_t MapPtrHasher::operator()(const model::Map* map) const noexcept {
		// Возвращает хеш значения, хранящегося внутри map
		return _hasher(map->GetName()) + _hasher(*(map->GetId()));
	}

	// Возвращает ответ на запрос по установке флага случайного стартового расположения
	http_handler::Response GameHandler::game_start_position_response(http_handler::StringRequest&& req) {
		if (req.method_string() != http_handler::Method::POST) {
			// если у нас не POST-запрос, то кидаем отбойник
			return method_not_allowed_impl(std::move(req), http_handler::Method::POST);
		}

		try
		{
			// парсим тело запроса, все исключения в процессе будем ловить в catch_блоке
			json::value req_data = json_detail::parse_text_to_json(req.body());
			std::string flag = req_data.at("randomPosition").as_string().data();

			if (flag == "false") {
				start_random_position_ = false;
			}
			else if (flag == "true") {
				start_random_position_ = true;
			}

			// запускаем обновление всех игровых сессий во всех игровых инстансах за O(N*K), 
			// где N - количество открытых инстансов, K - количество открытых игровых сессий в инстансе 
			for (auto& instance : instances_) {
				// берем сессии из инстанса
				for (auto& session : instance.second) {
					// обновляем каждую сессию
					session->set_start_random_position(start_random_position_);
				}
			}

			http_handler::StringResponse response(http::status::ok, req.version());
			response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
			response.set(http::field::cache_control, "no-cache");

			if (start_random_position_) {
				response.body() = json_detail::get_debug_argument("startRandomPosition", "true");
			}
			else {
				response.body() = json_detail::get_debug_argument("startRandomPosition", "false");
			}

			return response;
		}
		catch (const std::exception&)
		{
			return bad_request_response(std::move(req), "invalidArgument"sv, "Debug game start position request parse error"sv);
		}
	}
	// Возвращает ответ на запрос по удалению всех игровых сессий из обработчика
	http_handler::Response GameHandler::game_sessions_reset_response(http_handler::StringRequest&& req) {
		if (req.method_string() != http_handler::Method::POST) {
			// если у нас не POST-запрос, то кидаем отбойник
			return method_not_allowed_impl(std::move(req), http_handler::Method::POST);
		}

		try
		{
			instances_.clear();            // понадеемся на умное удаление в шаред поинтерах
			tokens_list_.clear();          // как только все шары самоуничтожатся, сессии прекратят существовать

			http_handler::StringResponse response(http::status::ok, req.version());

			response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
			response.set(http::field::cache_control, "no-cache");
			response.body() = json_detail::get_debug_argument("gameDataStatus", "dataIsClear");

			return response;
		}
		catch (const std::exception&)
		{
			return bad_request_response(std::move(req), "FatalError"sv, "Data Reset request is fail"sv);
		}
	}

	// Возвращает ответ на запрос по изменению состояния игровой сессии со временем
	http_handler::Response GameHandler::session_time_update_response(http_handler::StringRequest&& req) {
		if (req.method_string() != http_handler::Method::POST) {
			// если у нас не POST-запрос, то кидаем отбойник
			return method_not_allowed_impl(std::move(req), http_handler::Method::POST);
		}

		// ищем тушку авторизации среди хеддеров запроса
		auto content_type = req.find("Content-Type");
		if (content_type == req.end() || content_type->value() != "application/json") {
			// если нет тушки по авторизации, тогда кидаем отбойник
			return bad_request_response(std::move(req),
				"invalidArgument"sv, "Invalid content type"sv);
		}

		if (req.body().size() == 0) {
			// если нет тела запроса, тогда запрашиваем
			return bad_request_response(std::move(req),
				"invalidArgument"sv, "Request body whit argument <timeDelta> expected"sv);
		}

		try
		{
			return session_time_update_response_impl(std::move(req));
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error("GameHandler::player_action_response::error" + std::string(e.what()));
		}
	}
	// Возвращает ответ на запрос о совершении действий персонажем
	http_handler::Response GameHandler::player_action_response(http_handler::StringRequest&& req) {
		if (req.method_string() != http_handler::Method::POST) {
			// если у нас не POST-запрос, то кидаем отбойник
			return method_not_allowed_impl(std::move(req), http_handler::Method::POST);
		}

		// ищем тушку авторизации среди хеддеров запроса
		auto content_type = req.find("Content-Type");
		if (content_type == req.end() || content_type->value() != "application/json") {
			// если нет тушки по авторизации, тогда кидаем отбойник
			return bad_request_response(std::move(req),
				"invalidArgument"sv, "Invalid content type"sv);
		}

		if (req.body().size() == 0) {
			// если нет тела запроса, тогда запрашиваем
			return bad_request_response(std::move(req),
				"invalidArgument"sv, "Request body whit argument <move> expected"sv);
		}

		try
		{
			// в случае успешной авторизации, лямбда вызовет нужный обработчик
			return authorization_token_impl(std::move(req),
				[this](http_handler::StringRequest&& req, const Token* token) {
					return this->player_action_response_impl(std::move(req), token);
				});
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error("GameHandler::player_action_response::error" + std::string(e.what()));
		}
	}
	// Возвращает ответ на запрос о состоянии игроков в игровой сессии
	http_handler::Response GameHandler::game_state_response(http_handler::StringRequest&& req) {

		if (req.method_string() != http_handler::Method::GET && req.method_string() != http_handler::Method::HEAD) {
			// если у нас ни гет и ни хед запрос, то кидаем отбойник
			return method_not_allowed_impl(std::move(req), http_handler::Method::GET, http_handler::Method::HEAD);
		}

		try
		{
			// в случае успешной авторизации, лямбда вызовет нужный обработчик
			return authorization_token_impl(std::move(req),
				[this](http_handler::StringRequest&& req, const Token* token) {
					return this->game_state_response_impl(std::move(req), token);
				});
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error("GameHandler::game_state_response::error" + std::string(e.what()));
		}
	}
	// Возвращает ответ на запрос о списке игроков в данной сессии
	http_handler::Response GameHandler::player_list_response(http_handler::StringRequest&& req) {

		if (req.method_string() != http_handler::Method::GET && req.method_string() != http_handler::Method::HEAD) {
			// если у нас ни гет и ни хед запрос, то кидаем отбойник
			return method_not_allowed_impl(std::move(req), http_handler::Method::GET, http_handler::Method::HEAD);
		}

		try
		{
			// в случае успешной авторизации, лямбда вызовет нужный обработчик
			return authorization_token_impl(std::move(req), 
				[this](http_handler::StringRequest&& req, const Token* token) {
					return this->player_list_response_impl(std::move(req), token);
				});
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
			if (req.body().size() == 0) {
				// если нет тела запроса, тогда запрашиваем
				return bad_request_response(std::move(req),
					"invalidArgument"sv, "Header body whit two arguments <userName> and <mapId> expected"sv);
			}

			// парсим тело запроса, все исключения в процессе будем ловить в catch_блоке
			json::value req_data = json_detail::parse_text_to_json(req.body());
			
			{
				// если в блоке вообще нет графы "userName" или "mapId"
				if (!req_data.as_object().count("userName") || !req_data.as_object().count("mapId")) {
					return bad_request_response(std::move(req), "invalidArgument"sv, "Two arguments <userName> and <mapId> expected"sv);
				}

				// если в "userName" пустота
				if (req_data.as_object().at("userName") == "") {
					return bad_request_response(std::move(req), "invalidArgument"sv, "Invalid name"sv);
				}

				// ищем запрошенную карту
				auto map = game_simple_.find_map(
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
		}
		catch (const std::exception&)
		{
			return bad_request_response(std::move(req), "invalidArgument"sv, "Join game request parse error"sv);
		}
	}
	// Возвращает ответ на запрос по поиску конкретной карты
	http_handler::Response GameHandler::find_map_response(http_handler::StringRequest&& req, std::string_view find_request_line) {

		// ищем запрошенную карту
		auto map = game_simple_.find_map(model::Map::Id{ std::string(find_request_line) });

		if (map == nullptr) {
			// если карта не найдена, то кидаем отбойник
			return map_not_found_response_impl(std::move(req));
		}
		else {

			http_handler::StringResponse response(http::status::ok, req.version());
			response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
			response.set(http::field::cache_control, "no-cache");

			// загружаем тело ответа из жидомасонского блока по полученному выше блоку параметров карты
			response.body() = json_detail::get_map_info(map);

			return response;
		}

	}
	// Возвращает ответ со списком загруженных карт
	http_handler::Response GameHandler::map_list_response(http_handler::StringRequest&& req) {
		http_handler::StringResponse response(http::status::ok, req.version());
		response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
		response.set(http::field::cache_control, "no-cache");
		response.body() = json_detail::get_map_list(game_simple_.get_maps());

		return response;
	}
	// Возвращает ответ, что запрос некорректный
	http_handler::Response GameHandler::bad_request_response(
		http_handler::StringRequest&& req, std::string_view code, std::string_view message) {
		http_handler::StringResponse response(http::status::bad_request, req.version());
		response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
		response.set(http::field::cache_control, "no-cache");
		response.body() = json_detail::get_error_string(code, message);

		return response;
	}
	// Возвращает ответ, что запрос не прошёл валидацию
	http_handler::Response GameHandler::unauthorized_response(
		http_handler::StringRequest&& req, std::string_view code, std::string_view message) {
		http_handler::StringResponse response(http::status::unauthorized, req.version());
		response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
		response.set(http::field::cache_control, "no-cache");
		response.body() = json_detail::get_error_string(code, message);

		return response;
	}

	const Token* GameHandler::get_unique_token(std::shared_ptr<GameSession> session) {
		std::lock_guard func_lock_(mutex_);
		return get_unique_token_impl(session);
	}
	const Token* GameHandler::get_unique_token_impl(std::shared_ptr<GameSession> session) {

		bool isUnique = true;         // создаём реверсивный флаг
		Token unique_token{ "" };       // создаём токен-болванку

		while (isUnique)
		{
			// генерируем новый токен
			unique_token = Token{ detail::generate_token_32_hex() };
			// если сгенерированный токен уже есть, то флаг так и останется поднятым и цикл повторится
			isUnique = tokens_list_.count(unique_token);
		}

		auto insert = tokens_list_.insert({ unique_token,  session });
		return &(insert.first->first);
	}

	bool GameHandler::reset_token(std::string_view token) {
		std::lock_guard func_lock_(mutex_);
		return reset_token_impl(token);
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

	// Возвращает ответ на запрос по изменению состояния игровой сессии со временем
	http_handler::Response GameHandler::session_time_update_response_impl(http_handler::StringRequest&& req) {
		try
		{
			// пробуем записать строку запроса в json-блок
			json::object body = json_detail::parse_text_to_json(req.body()).as_object();

			if (!body.count("timeDelta") || (!body.at("timeDelta").is_number() && !body.at("timeDelta").is_string())) {
				// если в теле запроса отсутствует поле "timeDelta", или его значение не валидно
				return bad_request_response(std::move(req),
					"invalidArgument"sv, "Failed to parse tick request JSON"sv);
			}

			int time = 0;

			if (body.at("timeDelta").is_number()) {
				time = static_cast<int>(body.at("timeDelta").is_number());
			}
			else if (body.at("timeDelta").is_string()) {
				time = std::stoi(std::string(body.at("timeDelta").as_string()));
			}

			// запускаем обновление всех игровых сессий во всех игровых инстансах за O(N*K), 
			// где N - количество открытых инстансов, K - количество открытых игровых сессий в инстансе 
			for (auto& instance : instances_) {
				// берем сессии из инстанса
				for (auto& session : instance.second) {
					// обновляем каждую сессию
					session->update_state(time);
				}
			}

			// подготавливаем и возвращаем ответ о успехе операции
			http_handler::StringResponse response(http::status::ok, req.version());
			response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
			response.set(http::field::cache_control, "no-cache");
			response.body() = "{}";

			return response;
		}
		catch (const std::exception&)
		{
			return bad_request_response(std::move(req), "invalidArgument"sv, "Failed to parse tick request JSON"sv);
		}
	}
	// Возвращает ответ на запрос о состоянии игроков в игровой сессии
	http_handler::Response GameHandler::player_action_response_impl(http_handler::StringRequest&& req, const Token* token) {

		try
		{
			// пробуем записать строку запроса в json-блок
			json::object body = json_detail::parse_text_to_json(req.body()).as_object();

			if (!body.count("move") || !detail::check_player_move(body.at("move").as_string())) {
				// если в теле запроса отсутствует поле "Move", или его значение не валидно
				return bad_request_response(std::move(req),
					"invalidArgument"sv, "Failed to parse action"sv);
			}

			// получаем сессию где на данный момент "висит" указанный токен
			std::shared_ptr<GameSession> session = tokens_list_.at(*token);
			// запрашиваем сессию изменить скорость персонажа
			session->move_player(token, detail::parse_player_move(body.at("move").as_string()));

			// подготавливаем и возвращаем ответ о успехе операции
			http_handler::StringResponse response(http::status::ok, req.version());
			response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
			response.set(http::field::cache_control, "no-cache");
			response.body() = "{}";

			return response;
		}
		catch (const std::exception&)
		{
			return bad_request_response(std::move(req), "invalidArgument"sv, "Failed to parse action"sv);
		}
	}
	// Возвращает ответ на запрос о состоянии игроков в игровой сессии
	http_handler::Response GameHandler::game_state_response_impl(http_handler::StringRequest&& req, const Token* token) {
		
		// получаем сессию где на данный момент "висит" указанный токен
		std::shared_ptr<GameSession> session = tokens_list_.at(*token);

		// подготавливаем и возвращаем ответ
		http_handler::StringResponse response(http::status::ok, req.version());
		response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
		response.set(http::field::cache_control, "no-cache");
		// заполняем тушку ответа с помощью жисонского метода
		response.body() = json_detail::get_session_state_list(session->get_session_players());

		return response;
	}
	// Возвращает ответ на запрос о списке игроков в данной сессии
	http_handler::Response GameHandler::player_list_response_impl(http_handler::StringRequest&& req, const Token* token) {

		// получаем сессию где на данный момент "висит" указанный токен
		std::shared_ptr<GameSession> session = tokens_list_.at(*token);

		// подготавливаем и возвращаем ответ
		http_handler::StringResponse response(http::status::ok, req.version());
		response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
		response.set(http::field::cache_control, "no-cache");
		// заполняем тушку ответа с помощью жисонского метода
		response.body() = json_detail::get_session_players_list(session->get_session_players());

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
					.emplace_back(std::make_shared<GameSession>(*this, map, 200, start_random_position_));
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
				.emplace_back(std::make_shared<GameSession>(*this, map, 200, start_random_position_));
		}

		// добавляем челика на сервер и принимаем на него указатель
		new_player = ref->add_new_player(body.at("userName").as_string());

		// подготавливаем и возвращаем ответ
		http_handler::StringResponse response(http::status::ok, req.version());
		response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
		response.set(http::field::cache_control, "no-cache");
		response.body() = json_detail::get_session_join_player(new_player);

		return response;
	}
	// Возвращает ответ, что упомянутая карта не найдена
	http_handler::Response GameHandler::map_not_found_response_impl(http_handler::StringRequest&& req) {
		http_handler::StringResponse response(http::status::not_found, req.version());
		response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
		response.set(http::field::cache_control, "no-cache");
		response.body() = json_detail::get_error_string("mapNotFound"sv, "Map not found"sv);

		return response;
	}
	// Возвращает ответ, что запрошенный метод не ражрешен, доступный указывается в аргументе allow
	http_handler::Response GameHandler::method_not_allowed_impl(http_handler::StringRequest&& req, std::string_view allow) {
		http_handler::StringResponse response(http::status::method_not_allowed, req.version());
		response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
		response.set(http::field::cache_control, "no-cache");
		response.set(http::field::allow, allow);
		response.body() = json_detail::get_error_string("invalidMethod"sv, ("Only "s + std::string(allow) + " method is expected"s));

		return response;
	}
	
	namespace detail {

		// округляет double -> int по математическим законам
		int double_round(double value) {
			return static_cast<int>((value + ((value >= 0) ? 0.5 : -0.5)));
		}

		std::optional<std::string> BearerParser(std::string&& auth_line) {

			if (auth_line.size() != 39) {
				// если в строке меньше чем "Bearer " плюс 32 символа токена
				return std::nullopt;
			}
			else {

				std::string bearer{ auth_line.begin(), auth_line.begin() + 7 };
				if (bearer != "Bearer ") {
					// начальная строка должна быть такой как в условии
					return std::nullopt;
				}
				else {

					std::string token{ auth_line.begin() + 7, auth_line.end() };

					if (token.size() != 32) {
						return std::nullopt;
					}
					else {
						return token;
					}

					//return std::string{ auth_line.begin() + 7, auth_line.end() };
				}
			}
		}

	} // namespace detail

} //namespace game_handler