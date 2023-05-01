#include "game_handler.h"
#include <boost/asio.hpp>
#include <algorithm>

namespace game_handler {

	namespace fs = std::filesystem;
	namespace json = boost::json;

	// -------------------------- class GameTimer ----------------------------

	// запускает выполнение таймера
	GameTimer& GameTimer::Start() {
		if (!execution_) {
			execution_ = true;
			sys::error_code error_code;
			ExecutionImpl(error_code);
		}
		return *this;
	}

	// останавливает выполнение таймера
	GameTimer& GameTimer::Stop() {
		if (execution_) {
			execution_ = false;
		}
		return *this;
	}

	// назначает новый временной интервал таймера
	GameTimer& GameTimer::SetPeriod(std::chrono::milliseconds period) {
		if (!execution_) {
			period_ = period;
			return *this;
		}

		throw std::runtime_error("GameTimer::SetPeriod::Error::Timer execution flag is \"true\"");
	}

	// назначение новой функции для выполнения по таймеру
	GameTimer& GameTimer::SetFunction(Function&& function) {
		if (!execution_) {
			function_ = std::move(function);
			return *this;

		}
		throw std::runtime_error("GameTimer::SetFunction::Error::Timer execution flag is \"true\"");
	}

	// основная имплементация выполнения таймера
	void GameTimer::ExecutionImpl(sys::error_code ec) {
		// выполнение начинается только после установки флага о начале выполнения
		if (execution_) {
			try
			{
				timer_.async_wait(
					net::bind_executor(api_strand_, [self = shared_from_this()](sys::error_code ec) {
						// вызываем выполнение требуемой функции
						self->function_(self->period_);
						// переносим таймер на время периода повторения
						self->timer_.expires_from_now(self->period_);
						// снова вызываем метод выполнения
						self->ExecutionImpl(ec);
					}));
			}
			catch (const std::exception& e)
			{
				throw std::runtime_error("GameTimer::ExecutionImpl::Error " + std::string(e.what()));
			}
		}
		else {
			timer_.cancel();            // если флаг выполнения снят, то останавливаем таймер
		}
	}

	// -------------------------- class GameSession --------------------------
	
	// задаёт флаг случайной позиции для старта новых игроков
	GameSession& GameSession::SetRandomStartPosition(bool start_random_position) {
		random_start_position_ = start_random_position;
		return *this;
	}

	// добавляет нового игрока на случайное место на случайной дороге на карте
	Player* GameSession::AddPlayer(std::string_view name) {
		// смотрим есть ли место в текущей игровой сессии
		auto id = std::find(players_id_.begin(), players_id_.end(), false);
		if (id != players_id_.end()) {
			// если место есть, то запрашиваем уникальный токен
			const Token* player_token = game_handler_.GetUniqueToken(shared_from_this());
			// запрашиваем стартовую точку первой дороги
			model::Point point = session_game_map_->GetFirstRoadStartPosition();

			// заготовка под позицию установки нового игрока
			PlayerPosition position{ static_cast<double>(point.x), static_cast<double>(point.y) };

			// если активирован флаг получения случайной позиции на старте иначе будет старт на стартовой точке первой дороги
			if (random_start_position_) {
				// чтобы получить случайную позицию на какой-нибудь дороге на карте начнём цикл поиска места
				bool find_place = true;       // реверсивный флаг, который сделаем false, когда найдём место
				while (find_place)
				{
					// спрашиваем у карты случайную точку на какой-нибудь дороге
					point = session_game_map_->GetRandomPosition();
					// переводим точку в позицию
					position.x_ = point.x;
					position.y_ = point.y;
					// проверяем на совпадение позиции с уже имеющимися игроками и инвертируем вывод метода
					find_place = !CheckStartPositionImpl(position);
				}
			}
			
			// создаём игрока в текущей игровой сессии
			session_players_[player_token] = std::move(
				Player{ uint16_t(std::distance(players_id_.begin(), id)), name, player_token }
					.SetPlayerPosition(std::move(position))               // назначаем стартовую позицию
					.SetPlayerDirection(PlayerDirection::NORTH)                  // назначаем дефолтное направление взгляда
					.SetPlayerSpeed({ 0, 0 }));                            // назначаем стартовую скорость
					
			// делаем пометку в булевом массиве
			*id = true;

			// возвращаем игрока по токену
			return GetPlayer(player_token);
		}
		else {
			return nullptr;
		}
	}

	// вернуть указатель на игрока в сессии по токену
	Player* GameSession::GetPlayer(const Token* token) {
		if (session_players_.count(token)) {
			return &session_players_.at(token);
		}
		return nullptr;
	}

	// удалить игрока из игровой сессии
	bool GameSession::RemovePlayer(const Token* token) {
		if (!session_players_.count(token)) {
			return false;
		}
		else {
			// освобождаем id текущего игрока по токену
			players_id_[session_players_.at(token).GetPlayerId()] = false;
			// удаляем запись о игроке вместе со структурой
			session_players_.erase(token);
			return true;
		}
	}

	// обновляет состояние игры с заданным временем в миллисекундах
	bool GameSession::UpdateState(int time) {
		try
		{
			// просто перебираем всех игроков в сессии
			for (auto& [token, player] : session_players_) {
				// обновляет позицию выбранного игрока в соответствии с его заданной скоростью, направлением и временем в секундах
				UpdatePlayerPosition(player, static_cast<double>(time) / __MS_IN_ONE_SECOND__);
			}

			return true;
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error("GameSession::UpdateState::Error::" + std::string(e.what()));
		}
	}

	// метод добавляет скорость персонажу, вызывается из GameHandler::player_action_response_impl
	bool GameSession::MovePlayer(const Token* token, PlayerMove move) {

		switch (move)
		{
		case game_handler::PlayerMove::UP: // верх скорость {0, -speed}
			GetPlayer(token)->SetPlayerSpeed(0, -session_game_map_->GetOnMapSpeed()).SetPlayerDirection(PlayerDirection::NORTH);
			return true;

		case game_handler::PlayerMove::DOWN: // вниз скорость {0, speed}
			GetPlayer(token)->SetPlayerSpeed(0, session_game_map_->GetOnMapSpeed()).SetPlayerDirection(PlayerDirection::SOUTH);
			return true;

		case game_handler::PlayerMove::LEFT: // влево скорость {-speed, 0}
			GetPlayer(token)->SetPlayerSpeed(-session_game_map_->GetOnMapSpeed(), 0).SetPlayerDirection(PlayerDirection::WEST);
			return true;

		case game_handler::PlayerMove::RIGHT: // влево скорость {speed, 0}
			GetPlayer(token)->SetPlayerSpeed(session_game_map_->GetOnMapSpeed(), 0).SetPlayerDirection(PlayerDirection::EAST);
			return true;

		case game_handler::PlayerMove::STAY:
			GetPlayer(token)->SetPlayerSpeed(0, 0);
			return true;

		case game_handler::PlayerMove::error:
			throw std::runtime_error("GameSession::MovePlayer::PlayerMove::error");

		default:
			return false;
		}
	}

	// отвечает есть ли в сессии свободное местечко
	bool GameSession::CheckFreeSpace() {
		// смотрим есть ли место в текущей игровой сессии
		auto id = std::find(players_id_.begin(), players_id_.end(), false);

		return id != players_id_.end();
	}

	// изменяет координаты игрока при движении параллельно дороге, на которой он стоит
	bool GameSession::ParallelMovingImpl(Player& player, PlayerDirection direction, 
		PlayerPosition&& from, PlayerPosition&& to, const model::Road* road) {
		
		bool player_keep_moving = true;                // флаг продолжения движения игрока 
		double limit_dy = 0u;                          // заготовка под лимит по оси Y
		double limit_dx = 0u;                          // заготовка под лимит по оси X

		switch (direction)
		{
		// в данном кейсе координата PlayerPosition to.y_ должна быть меньше PlayerPosition from.y_
		case game_handler::PlayerDirection::NORTH:
			// необходимо убедиться, что мы не поднимемся выше допустимого лимита
			// берем наименьшую координату вертикальной дороги по оси Y и вычитаем дельту
			limit_dy = static_cast<double>(std::min(road->GetStart().y, road->GetEnd().y)) - __ROAD_DELTA__;
			if (to.y_ <= limit_dy) {
				// если приращение меньше максимально допустимого (максимальный отступ от оси дороги вверх)
				to.y_ = limit_dy;                      // то просто меняем, приращение по вертикальной оси
				player_keep_moving = false;               // снимаем флаг продолжения движения
			}
			break;

		// в данном кейсе координата PlayerPosition to.y_ должна быть больше PlayerPosition from.y_
		case game_handler::PlayerDirection::SOUTH:
			// необходимо убедиться, что мы не опускаемся ниже допустимого лимита
			// берем наивысшую координату вертикальной дороги по оси Y и прибавляем дельту
			limit_dy = static_cast<double>(std::max(road->GetStart().y, road->GetEnd().y)) + __ROAD_DELTA__;
			if (to.y_ >= limit_dy) {
				// если приращение больше максимально допустимого (максимальный отступ от оси дороги вниз)
				to.y_ = limit_dy;                      // то просто меняем, приращение по вертикальной оси
				player_keep_moving = false;               // снимаем флаг продолжения движения
			}
			break;

		// в данном кейсе координата PlayerPosition to.x_ должна быть меньше PlayerPosition from.x_
		case game_handler::PlayerDirection::WEST:
			// необходимо убедиться, что мы не смещаемся левее допустимого лимита
			// берем наименьшую координату горизонтальной дороги по оси X и вычитаем дельту
			limit_dx = static_cast<double>(std::min(road->GetStart().x, road->GetEnd().x)) - __ROAD_DELTA__;
			if (to.x_ <= limit_dx) {
				// если приращение меньше максимально допустимого (максимальный отступ от оси дороги влево)
				to.x_ = limit_dx;                      // то просто меняем, приращение по горизонтальной оси
				player_keep_moving = false;               // снимаем флаг продолжения движения
			}
			break;

		// в данном кейсе координата PlayerPosition to.x_ должна быть больше PlayerPosition from.x_
		case game_handler::PlayerDirection::EAST:
			// необходимо убедиться, что мы не смещаемся правее допустимого лимита
			// берем наибольшую координату горизонтальной дороги по оси X и прибавляем дельту
			limit_dx = static_cast<double>(std::max(road->GetStart().x, road->GetEnd().x)) + __ROAD_DELTA__;
			if (to.x_ >= limit_dx) {
				// если приращение больше максимально допустимого (максимальный отступ от оси дороги влево)
				to.x_ = limit_dx;                      // то просто меняем, приращение по горизонтальной оси
				player_keep_moving = false;               // снимаем флаг продолжения движения
			}
			break;

		default:
			return false;
		}
		// записываем новые координаты и тормозим если необходимо
		player_keep_moving ? player.SetPlayerPosition(std::move(to)) :
			player.SetPlayerPosition(std::move(to)).SetPlayerSpeed(0, 0);

		return true;
	}

	// изменяет координаты игрока при движении перпендикулярно дороге, на которой он стоит
	bool GameSession::CrossMovingImpl(Player& player, PlayerDirection direction, 
		PlayerPosition&& from, PlayerPosition&& to, const model::Road* road) {
		
		bool player_keep_moving = true;                // флаг продолжения движения игрока 
		double limit_dy = 0u;                          // заготовка под лимит по оси Y
		double limit_dx = 0u;                          // заготовка под лимит по оси X
		
		switch (direction)
		{
		// в данном кейсе координата PlayerPosition to.y_ должна быть меньше PlayerPosition from.y_
		case game_handler::PlayerDirection::NORTH:
			// округляем y_ позиции игрока (from), до целого и вычитаем дельту отступа от центра дороги
			limit_dy = static_cast<double>(detail::RoundDoubleMathematic(from.y_)) - __ROAD_DELTA__;
			if (to.y_ <= limit_dy) {
				// если приращение меньше максимально допустимого (максимальный отступ от оси дороги вверх)
				to.y_ = limit_dy;                      // то просто меняем, приращение по вертикальной оси
				player_keep_moving = false;               // снимаем флаг продолжения движения
			}
			break;

		// в данном кейсе координата PlayerPosition to.y_ должна быть больше PlayerPosition from.y_
		case game_handler::PlayerDirection::SOUTH:
			// округляем y_ позиции игрока (from), до целого и прибавляем дельту отступа от центра дороги
			limit_dy = static_cast<double>(detail::RoundDoubleMathematic(from.y_)) + __ROAD_DELTA__;
			if (to.y_ >= limit_dy) {
				// если приращение больше максимально допустимого (максимальный отступ от оси дороги вниз)
				to.y_ = limit_dy;                     // то просто меняем, приращение по вертикальной оси
				player_keep_moving = false;              // снимаем флаг продолжения движения
			}
			break;

		// в данном кейсе координата PlayerPosition to.x_ должна быть меньше PlayerPosition from.x_
		case game_handler::PlayerDirection::WEST:
			// округляем x_ позиции игрока (from), до целого и вычитаем дельту отступа от центра дороги
			limit_dx = static_cast<double>(detail::RoundDoubleMathematic(from.x_)) - __ROAD_DELTA__;
			if (to.x_ <= limit_dx) {
				// если приращение меньше максимально допустимого (максимальный отступ от оси дороги влево)
				to.x_ = limit_dx;                     // то просто меняем, приращение по вертикальной оси
				player_keep_moving = false;              // снимаем флаг продолжения движения
			}
			break;

		// в данном кейсе координата PlayerPosition to.x_ должна быть больше PlayerPosition from.x_
		case game_handler::PlayerDirection::EAST:
			// округляем x_ позиции игрока (from), до целого и прибавляем дельту отступа от центра дороги
			limit_dx = static_cast<double>(detail::RoundDoubleMathematic(from.x_)) + __ROAD_DELTA__;
			if (to.x_ >= limit_dx) {
				// если приращение больше максимально допустимого (максимальный отступ от оси дороги влево)
				to.x_ = limit_dx;                     // то просто меняем, приращение по вертикальной оси
				player_keep_moving = false;              // снимаем флаг продолжения движения
			}
			break;

		default:
			return false;
		}
		// записываем новые координаты и тормозим если необходимо
		player_keep_moving ? player.SetPlayerPosition(std::move(to)) :
			player.SetPlayerPosition(std::move(to)).SetPlayerSpeed(0, 0);

		return true;
	}

	bool GameSession::UpdatePlayerPosition(Player& player, double time) {

		// записываем вектор ожидаемого приращения по положению персонажа
		PlayerPosition delta_pos{ player.GetPlayerPosition().x_ + (player.GetPlayerSpeed().xV_ * time), 
			player.GetPlayerPosition().y_ + (player.GetPlayerSpeed().yV_ * time) };
		// чтобы лишнего не считать, проверяем есть ли у нас какое-то приращение в принципе
		if (delta_pos.x_ == 0 && delta_pos.y_ == 0) {
			return true;           // если приращения нет, то сразу выходим и не продолжаем
		}

		const model::Road* road = nullptr;    // готовим заготовку под "дорогу"
		// записываем во временную переменную, чтобы не делать лишних вызовов
		PlayerDirection direction = player.GetPlayerDirection();
		//PlayerDirection direction = player.get_speed_direction();
		PlayerPosition position = player.GetPlayerPosition();
		// округляем позицию до уровня логики model::Map
		model::Point point{ detail::RoundDoubleMathematic(position.x_), detail::RoundDoubleMathematic(position.y_) };
		
		// в зависимости от нашего направления запрашиваем дорогу
		// если игрок смотрит влево или вправо, полагаем, что будет движение по горизонтальной дороге
		if (direction == PlayerDirection::WEST || direction == PlayerDirection::EAST) {	
			road = session_game_map_->GetHorizontalRoad(point);     // зарос или вернет дорогу, или nullptr
		}
		// если игрок смотрит вниз или вверх, полагаем, что будет движение по вертикальной дороге
		else if (direction == PlayerDirection::NORTH || direction == PlayerDirection::SOUTH) {
			road = session_game_map_->GetVerticalRoad(point);       // зарос или вернет дорогу, или nullptr
		}

		// тут важный момент, если мы стоим на требуемой для движения дороге,
		// то в принципе мы в состоянии спокойно двигаться проверив выход за границы дороги
		if (road) {
			// отдаём обработку методу перемещения параллельно дороге
			return ParallelMovingImpl(player, direction, std::move(position), std::move(delta_pos), road);
		}
		// если же мы не стоим на требуемой - стоим на дороге перпендикулярной оси движения
		else {
			// отдаём обработку методу перемещения перпендикулярно дороге
			return CrossMovingImpl(player, direction, std::move(position), std::move(delta_pos), road);
		}
	}

	// чекает стартовую позицию на предмет совпадения с другими игроками в сессии
	bool GameSession::CheckStartPositionImpl(PlayerPosition& position) {

		for (const auto& item : session_players_) {
			// если нашли поцизию совпадающую с запрошенной то выходим с false
			if (position == item.second.GetPlayerPosition()) {
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

	// Выполняет обновление всех открытых игровых сессий по времени
	void GameHandler::UpdateGameSessions(int time) {
		if (time > 0) {
			// запускаем обновление всех игровых сессий во всех игровых инстансах за O(N*K), 
			// где N - количество открытых инстансов, K - количество открытых игровых сессий в инстансе 
			for (auto& instance : instances_) {
				// берем сессии из инстанса
				for (auto& session : instance.second) {
					// обновляем каждую сессию
					session->UpdateState(time);
				}
			}
		}
		else {
			throw std::runtime_error("GameHandler::(void)::UpdateGameSessions::Error::Income time < {0} ms");
		}
	}

	// Назначает флаг случайного размещения игроков на картах
	void GameHandler::SetRandomStartPosition(bool flag) {

		if (random_start_position_ != flag) {
			// если переданный флаг отличается от установленного
			random_start_position_ = flag;

			// запускаем обновление всех игровых сессий во всех игровых инстансах за O(N*K), 
			// где N - количество открытых инстансов, K - количество открытых игровых сессий в инстансе 
			for (auto& instance : instances_) {
				// берем сессии из инстанса
				for (auto& session : instance.second) {
					// обновляем каждую сессию
					session->SetRandomStartPosition(random_start_position_);
				}
			}
		}
	}
	// Сбрасывает и удаляет все активные игровые сессии
	void GameHandler::ResetGameSessions() {
		try
		{
			instances_.clear();            // понадеемся на умное удаление в шаред поинтерах
			tokens_list_.clear();          // как только все шары самоуничтожатся, сессии прекратят существовать
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error("GameHandler::ResetGameSessions::Error::" + std::string(e.what()));
		}
	}

	// Возвращает ответ на запрос по изменению состояния игровой сессии со временем
	http_handler::Response GameHandler::SessionsUpdateResponse(http_handler::StringRequest&& req) {
		if (req.method_string() != http_handler::Method::POST) {
			// если у нас не POST-запрос, то кидаем отбойник
			return NotAllowedResponseImpl(std::move(req), http_handler::Method::POST);
		}

		// ищем тушку авторизации среди хеддеров запроса
		auto content_type = req.find("Content-Type");
		if (content_type == req.end() || content_type->value() != "application/json") {
			// если нет тушки по авторизации, тогда кидаем отбойник
			return CommonFailResponseImpl(std::move(req), http::status::bad_request,
				"invalidArgument", "Invalid content type");
		}

		if (req.body().size() == 0) {
			// если нет тела запроса, тогда запрашиваем
			return CommonFailResponseImpl(std::move(req), http::status::bad_request,
				"invalidArgument", "Request body whit argument <timeDelta> expected");
		}

		try
		{
			return SessionsUpdateResponseImpl(std::move(req));
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error("GameHandler::SessionsUpdateResponse::error" + std::string(e.what()));
		}
	}

	// Возвращает ответ на запрос о совершении действий персонажем
	http_handler::Response GameHandler::PlayerActionResponse(http_handler::StringRequest&& req) {
		if (req.method_string() != http_handler::Method::POST) {
			// если у нас не POST-запрос, то кидаем отбойник
			return NotAllowedResponseImpl(std::move(req), http_handler::Method::POST);
		}

		// ищем тушку авторизации среди хеддеров запроса
		auto content_type = req.find("Content-Type");
		if (content_type == req.end() || content_type->value() != "application/json") {
			// если нет тушки по авторизации, тогда кидаем отбойник
			return CommonFailResponseImpl(std::move(req), http::status::bad_request,
				"invalidArgument", "Invalid content type");
		}

		if (req.body().size() == 0) {
			// если нет тела запроса, тогда запрашиваем
			return CommonFailResponseImpl(std::move(req), http::status::bad_request,
				"invalidArgument", "Request body whit argument <move> expected");
		}

		try
		{
			// в случае успешной авторизации, лямбда вызовет нужный обработчик
			return PlayerAuthorizationImpl(std::move(req),
				[this](http_handler::StringRequest&& req, const Token* token) {
					return this->PlayerActionResponseImpl(std::move(req), token);
				});
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error("GameHandler::PlayerActionResponse::error" + std::string(e.what()));
		}
	}

	// Возвращает ответ на запрос о состоянии игроков в игровой сессии
	http_handler::Response GameHandler::GameStateResponse(http_handler::StringRequest&& req) {

		if (req.method_string() != http_handler::Method::GET && req.method_string() != http_handler::Method::HEAD) {
			// если у нас ни гет и ни хед запрос, то кидаем отбойник
			return NotAllowedResponseImpl(std::move(req), http_handler::Method::GET, http_handler::Method::HEAD);
		}

		try
		{
			// в случае успешной авторизации, лямбда вызовет нужный обработчик
			return PlayerAuthorizationImpl(std::move(req),
				[this](http_handler::StringRequest&& req, const Token* token) {
					return this->GameStateResponseImpl(std::move(req), token);
				});
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error("GameHandler::GameStateResponse::error" + std::string(e.what()));
		}
	}

	// Возвращает ответ на запрос о списке игроков в данной сессии
	http_handler::Response GameHandler::PlayersListResponse(http_handler::StringRequest&& req) {

		if (req.method_string() != http_handler::Method::GET && req.method_string() != http_handler::Method::HEAD) {
			// если у нас ни гет и ни хед запрос, то кидаем отбойник
			return NotAllowedResponseImpl(std::move(req), http_handler::Method::GET, http_handler::Method::HEAD);
		}

		try
		{
			// в случае успешной авторизации, лямбда вызовет нужный обработчик
			return PlayerAuthorizationImpl(std::move(req), 
				[this](http_handler::StringRequest&& req, const Token* token) {
					return this->PlayersListResponseImpl(std::move(req), token);
				});
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error("GameHandler::PlayersListResponse::error" + std::string(e.what()));
		}
	}

	// Возвращает ответ на запрос по присоединению к игре
	http_handler::Response GameHandler::JoinGameResponse(http_handler::StringRequest&& req) {

		if (req.method_string() != http_handler::Method::POST) {
			// сюда вставить респонс о недопустимом типе
			return NotAllowedResponseImpl(std::move(req), http_handler::Method::POST);
		}

		try
		{
			if (req.body().size() == 0) {
				// если нет тела запроса, тогда запрашиваем
				return CommonFailResponseImpl(std::move(req), http::status::bad_request,
					"invalidArgument", "Header body whit two arguments <userName> and <mapId> expected");
			}

			// парсим тело запроса, все исключения в процессе будем ловить в catch_блоке
			json::value req_data = json_detail::ParseTextToJSON(req.body());
			
			{
				// если в блоке вообще нет графы "userName" или "mapId"
				if (!req_data.as_object().count("userName") || !req_data.as_object().count("mapId")) {
					return CommonFailResponseImpl(std::move(req), http::status::bad_request,
						"invalidArgument", "Two arguments <userName> and <mapId> expected");
				}

				// если в "userName" пустота
				if (req_data.as_object().at("userName") == "") {
					return CommonFailResponseImpl(std::move(req), http::status::bad_request,
						"invalidArgument", "Invalid name");
				}

				// ищем запрошенную карту
				auto map = game_simple_.FindMap(
					model::Map::Id{ std::string(
						req_data.as_object().at("mapId").as_string()) });

				if (map == nullptr) {
					// если карта не найдена, то кидаем отбойник
					return CommonFailResponseImpl(std::move(req), http::status::not_found,
						"mapNotFound", "Map not found");
				}
				else {
					// если карта есть и мы получили указатель
					// передаем управление основной имплементации
					return JoinGameResponseImpl(std::move(req), std::move(req_data), map);
				}
			}
		}
		catch (const std::exception&)
		{
			return CommonFailResponseImpl(std::move(req), http::status::bad_request,
				"invalidArgument", "Join game request parse error");
		}
	}

	// Возвращает ответ на запрос по поиску конкретной карты
	http_handler::Response GameHandler::FindMapResponse(http_handler::StringRequest&& req, std::string_view find_request_line) {

		// ищем запрошенную карту
		auto map = game_simple_.FindMap(model::Map::Id{ std::string(find_request_line) });

		if (map == nullptr) {
			// если карта не найдена, то кидаем отбойник
			return CommonFailResponseImpl(std::move(req), http::status::not_found,
				"mapNotFound", "Map not found");
		}
		else {

			http_handler::StringResponse response(http::status::ok, req.version());
			response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
			response.set(http::field::cache_control, "no-cache");

			// заполняем тушку ответа с помощью жисонского метода
			std::string body_str = json_detail::GetMapInfo(map);
			response.set(http::field::content_length, std::to_string(body_str.size()));
			response.body() = body_str;

			return response;
		}
	}

	// Возвращает ответ со списком загруженных карт
	http_handler::Response GameHandler::MapsListResponse(http_handler::StringRequest&& req) {
		http_handler::StringResponse response(http::status::ok, req.version());
		response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
		response.set(http::field::cache_control, "no-cache");

		// заполняем тушку ответа с помощью жисонского метода
		std::string body_str = json_detail::GetMapsList(game_simple_.GetMaps());
		response.set(http::field::content_length, std::to_string(body_str.size()));
		response.body() = body_str;

		return response;
	}

	const Token* GameHandler::GetUniqueToken(std::shared_ptr<GameSession> session) {
		std::lock_guard func_lock(mutex_);
		return GetUniqueTokenImpl(session);
	}

	const Token* GameHandler::GetUniqueTokenImpl(std::shared_ptr<GameSession> session) {

		bool isUnique = true;           // создаём реверсивный флаг
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

	bool GameHandler::ResetToken(std::string_view token) {
		std::lock_guard func_lock(mutex_);
		return ResetTokenImpl(token);
	}

	bool GameHandler::ResetTokenImpl(std::string_view token) {
		Token remove{ std::string(token) };

		// TODO Заглушка, скорее всего корректно работать не будет, надо переделать + удаление в GameSession

		if (tokens_list_.count(remove)) {
			tokens_list_.at(remove)->RemovePlayer(&remove);
			return tokens_list_.erase(remove);
		}
		else {
			return false;
		}
	}

	// Возвращает ответ на запрос по изменению состояния игровой сессии со временем
	http_handler::Response GameHandler::SessionsUpdateResponseImpl(http_handler::StringRequest&& req) {
		try
		{
			// пробуем записать строку запроса в json-блок
			json::object body = json_detail::ParseTextToJSON(req.body()).as_object();

			if (!body.count("timeDelta") || (!body.at("timeDelta").is_number() && !body.at("timeDelta").is_string())) {
				// если в теле запроса отсутствует поле "timeDelta", или его значение не валидно
				return CommonFailResponseImpl(std::move(req), http::status::bad_request,
					"invalidArgument", "Failed to parse tick request JSON");
			}

			int time = 0;

			if (body.at("timeDelta").is_number()) {
				time = static_cast<int>(body.at("timeDelta").as_int64());
			}
			else if (body.at("timeDelta").is_string()) {
				time = std::stoi(std::string(body.at("timeDelta").as_string()));
			}

			if (time == 0) {
				// если задают ноль, то также выдаём badRequest
				return CommonFailResponseImpl(std::move(req), http::status::bad_request,
					"invalidArgument", "Failed to parse tick request JSON");
			}

			// запускаем обновление всех игровых сессий во всех игровых инстансах за O(N*K), 
			// где N - количество открытых инстансов, K - количество открытых игровых сессий в инстансе 
			for (auto& instance : instances_) {
				// берем сессии из инстанса
				for (auto& session : instance.second) {
					// обновляем каждую сессию
					session->UpdateState(time);
				}
			}

			// подготавливаем и возвращаем ответ о успехе операции
			http_handler::StringResponse response(http::status::ok, req.version());
			response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
			response.set(http::field::cache_control, "no-cache");
			response.set(http::field::content_length, "2");
			response.body() = "{}";

			return response;
		}
		catch (const std::exception&)
		{
			return CommonFailResponseImpl(std::move(req), http::status::bad_request,
				"invalidArgument", "Failed to parse tick request JSON");
		}
	}

	// Возвращает ответ на запрос о состоянии игроков в игровой сессии
	http_handler::Response GameHandler::PlayerActionResponseImpl(http_handler::StringRequest&& req, const Token* token) {

		try
		{
			// пробуем записать строку запроса в json-блок
			json::object body = json_detail::ParseTextToJSON(req.body()).as_object();

			if (!body.count("move") || !detail::CheckPlayerMove(body.at("move").as_string())) {
				return CommonFailResponseImpl(std::move(req), http::status::bad_request,
					"invalidArgument", "Failed to parse action");
			}

			// получаем сессию где на данный момент "висит" указанный токен
			std::shared_ptr<GameSession> session = tokens_list_.at(*token);
			// запрашиваем сессию изменить скорость персонажа
			session->MovePlayer(token, detail::ParsePlayerMove(body.at("move").as_string()));

			// подготавливаем и возвращаем ответ о успехе операции
			http_handler::StringResponse response(http::status::ok, req.version());
			response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
			response.set(http::field::cache_control, "no-cache");
			response.set(http::field::content_length, "2");
			response.body() = "{}";

			return response;
		}
		catch (const std::exception&)
		{
			return CommonFailResponseImpl(std::move(req), http::status::bad_request,
				"invalidArgument", "Failed to parse action");
		}
	}

	// Возвращает ответ на запрос о состоянии игроков в игровой сессии
	http_handler::Response GameHandler::GameStateResponseImpl(http_handler::StringRequest&& req, const Token* token) {
		
		// получаем сессию где на данный момент "висит" указанный токен
		std::shared_ptr<GameSession> session = tokens_list_.at(*token);

		// подготавливаем и возвращаем ответ
		http_handler::StringResponse response(http::status::ok, req.version());
		response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
		response.set(http::field::cache_control, "no-cache");

		// заполняем тушку ответа с помощью жисонского метода
		std::string body_str = json_detail::GetSessionStateList(session->GetPlayers());
		response.set(http::field::content_length, std::to_string(body_str.size()));
		response.body() = body_str;

		return response;
	}

	// Возвращает ответ на запрос о списке игроков в данной сессии
	http_handler::Response GameHandler::PlayersListResponseImpl(http_handler::StringRequest&& req, const Token* token) {

		// получаем сессию где на данный момент "висит" указанный токен
		std::shared_ptr<GameSession> session = tokens_list_.at(*token);

		// подготавливаем и возвращаем ответ
		http_handler::StringResponse response(http::status::ok, req.version());
		response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
		response.set(http::field::cache_control, "no-cache");
		// заполняем тушку ответа с помощью жисонского метода
		std::string body_str = json_detail::GetSessionPlayersList(session->GetPlayers());
		response.set(http::field::content_length, std::to_string(body_str.size()));
		response.body() = body_str;
		
		return response;
	}

	// Возвращает ответ, о успешном добавлении игрока в игровую сессию
	http_handler::Response GameHandler::JoinGameResponseImpl(http_handler::StringRequest&& req, json::value&& body, const model::Map* map) {
		
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
				if (item->CheckFreeSpace()) {

					ref = item;              // записываем ссылку на шару сессии
					have_a_plance = true;    // ставим флаг, что место нашли
					break;                   // завершаем цикл за ненадобностью
				}
			}

			// если место было найдено, то ничего делать не надо - ref у нас есть, ниже по коду будет добавлен челик и создан ответ
			if (!have_a_plance) {
				// если же мест в текущих открытых сессиях НЕ найдено, ну вот нету, значит надо открыть новую
				ref = instances_.at(map)
					.emplace_back(std::make_shared<GameSession>(*this, map, 200, random_start_position_));
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
				.emplace_back(std::make_shared<GameSession>(*this, map, 200, random_start_position_));
		}

		// добавляем челика на сервер и принимаем на него указатель
		new_player = ref->AddPlayer(body.at("userName").as_string());

		// подготавливаем и возвращаем ответ
		http_handler::StringResponse response(http::status::ok, req.version());
		response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
		response.set(http::field::cache_control, "no-cache");

		std::string body_str = json_detail::GetSessionPlayerJoin(new_player);
		response.set(http::field::content_length, std::to_string(body_str.size()));
		response.body() = body_str;

		return response;
	}

	// Возвращает ответ, что запрошенный метод не ражрешен, доступный указывается в аргументе allow
	http_handler::Response GameHandler::NotAllowedResponseImpl(http_handler::StringRequest&& req, std::string_view allow) {
		http_handler::StringResponse response(http::status::method_not_allowed, req.version());
		response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
		response.set(http::field::cache_control, "no-cache");
		response.set(http::field::allow, allow);
		response.body() = json_detail::GetErrorString("invalidMethod"sv, ("Only "s + std::string(allow) + " method is expected"s));

		return response;
	}

	// Возвращает ответ на все варианты неверных и невалидных запросов
	http_handler::Response GameHandler::CommonFailResponseImpl(http_handler::StringRequest&& req, 
		http::status status, std::string_view code, std::string_view message) {

		http_handler::StringResponse response(status, req.version());
		response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
		response.set(http::field::cache_control, "no-cache");

		// заполняем тушку ответа с помощью жисонского метода
		std::string body_str = json_detail::GetErrorString(code, message);
		response.set(http::field::content_length, std::to_string(body_str.size()));
		response.body() = body_str;
		response.prepare_payload();

		return response;
	}
	
	namespace detail {

		// округляет double -> int по математическим законам
		int RoundDoubleMathematic(double value) {
			return static_cast<int>((value + ((value >= 0) ? 0.5 : -0.5)));
		}

		std::optional<std::string> BearerParser(const std::string& auth_line) {

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
				}
			}
		}

	} // namespace detail

} //namespace game_handler