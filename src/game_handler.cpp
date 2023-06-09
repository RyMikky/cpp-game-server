﻿#include "game_handler.h"
#include <boost/asio.hpp>
#include <algorithm>

namespace game_handler {

	namespace fs = std::filesystem;
	namespace json = boost::json;

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
			// берем свободный уникальный иденнтификатор
			size_t unique_id = static_cast<size_t>(std::distance(players_id_.begin(), id));

			// заготовка под позицию установки нового игрока
			PlayerPosition position{ session_map_->GetFirstRoadStartPosition() };

			// если активирован флаг получения случайной позиции на старте иначе будет старт на стартовой точке первой дороги
			if (random_start_position_) {
				// чтобы получить случайную позицию на какой-нибудь дороге на карте начнём цикл поиска места
				bool find_place = true;       // реверсивный флаг, который сделаем false, когда найдём место
				while (find_place)
				{
					// спрашиваем у карты случайную точку на какой-нибудь дороге
					position = session_map_->GetRandomPosition();
					// проверяем на совпадение позиции с уже имеющимися игроками и инвертируем вывод метода
					find_place = !CheckStartPositionImpl(position);
				}
			}
			
			AddPlayerImpl(unique_id, name, player_token, session_map_->GetOnMapBagCapacity())
				.SetCurrentPosition(std::move(position))         // назначаем стартовую позицию
				.SetDirection(PlayerDirection::NORTH)                   // назначаем дефолтное направление взгляда
				.SetSpeed({ 0, 0 });                              // назначаем стартовую скорость

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
			players_id_[session_players_.at(token).GetId()] = false;
			// удаляем запись о игроке вместе со структурой
			session_players_.erase(token);
			return true;
		}
	}

	/*
	* Обновляет состояние игры с заданным временем в миллисекундах.
	* Запускает полный цикл обработки в следующей последовательности:
	*  1. Расчёт будущих позиций игроков
	*  2. Расчёт и выполнение ожидаемых при перемещении коллизий
	*  3. Выполнение перемещения игроков на будущие координаты
	*  4. Генерация лута на карте
	*/
	bool GameSession::UpdateState(int time) {
		try
		{
			// 1. Расчёт будущих позиций игроков, так как задаётся время в миллисекундах
			// то мы должны перевести данные в дабл в секунды, так как скорость считается в м/с
			UpdateFuturePlayersPositions(static_cast<double>(time) / __MS_IN_ONE_SECOND__);

			// 2. Расчёт и выполнение ожидаемых при перемещении коллизий
			// В процессе исполнения будут расчитаны коллизии, выполнены действия по подбору и сдаче предметов лута
			HandlePlayersCollisionsActions();

			// 3. Выполнение перемещения игроков на расчитаные в пункте 1 будущие координаты
			UpdateCurrentPlayersPositions();

			// 4. Генерация лута на карте
			UpdateSessionLootsCount(time);

			//// запрашиваем количество лута для генерации
			//auto new_loot_count = loot_gen_.Generate(
			//	std::chrono::milliseconds(time), 
			//	static_cast<unsigned>(session_loots_.size()), 
			//	static_cast<unsigned>(session_players_.size()));

			//// при успешной генерации исключений не будет, и генерация вернет true
			//return (new_loot_count > 0) ? GenerateSessionLootImpl(new_loot_count) : true;
			return true;
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error("GameSession::UpdateState::Error::\n" + std::string(e.what()));
		}
	}

	// метод добавляет скорость персонажу, вызывается из GameHandler::player_action_response_impl
	bool GameSession::MovePlayer(const Token* token, PlayerMove move) {

		switch (move)
		{
		case game_handler::PlayerMove::UP: // верх скорость {0, -speed}
			GetPlayer(token)->SetSpeed(0, -session_map_->GetOnMapSpeed()).SetDirection(PlayerDirection::NORTH);
			return true;

		case game_handler::PlayerMove::DOWN: // вниз скорость {0, speed}
			GetPlayer(token)->SetSpeed(0, session_map_->GetOnMapSpeed()).SetDirection(PlayerDirection::SOUTH);
			return true;

		case game_handler::PlayerMove::LEFT: // влево скорость {-speed, 0}
			GetPlayer(token)->SetSpeed(-session_map_->GetOnMapSpeed(), 0).SetDirection(PlayerDirection::WEST);
			return true;

		case game_handler::PlayerMove::RIGHT: // влево скорость {speed, 0}
			GetPlayer(token)->SetSpeed(session_map_->GetOnMapSpeed(), 0).SetDirection(PlayerDirection::EAST);
			return true;

		case game_handler::PlayerMove::STAY:
			GetPlayer(token)->SetSpeed(0, 0);
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

	// ----------------- блок наследуемых методов CollisionProvider ----------------------------

	// возвращает количество офисов бюро находок на карте игровой сессии
	size_t GameSession::OfficesCount() const {
		return session_map_->GetOffices().size();
	}

	// возвращает офис бюро находок по индексу
	const model::Office& GameSession::GetOffice(size_t index) const {
		if (index < OfficesCount()) {
			return session_map_->GetOffices()[index];
		}
		else {
			throw std::out_of_range("game_handler::GameSession::GetOffice(index)::Error::Index is out of range");
		}
	}

	// добавляет нового игрока на карту
	Player& GameSession::AddPlayerImpl(size_t id, std::string_view name, const Token* token, unsigned capacity) {
		// добавляем игрока в базу
		session_players_[token] = std::move(Player{ id,name, token, capacity });
		players_id_[id] = true;                            // поднимаем флаг на массиве индексов
		return session_players_.at(token);                // возвращаем созданного игрока
	}

	// проверяет стартовую позицию игрока на предмет совпадения с другими игроками в сессии
	bool GameSession::CheckStartPositionImpl(PlayerPosition& position) {

		for (const auto& [token, player] : session_players_) {
			// если нашли поцизию совпадающую с запрошенной то выходим с false
			if (position == player.GetCurrentPosition()) {
				return false;
			}
		}
		return true;
	}

	// генерирует лут с заданым типом, идентификатором и позицией
	bool GameSession::GenerateSessionLootImpl(size_t type, size_t id, PlayerPosition pos) {
		session_loots_.emplace(id, std::move(
			GameLoot{ session_map_->GetLootType(type), type, id, pos }));
		loots_id_[id] = true;

		return true;
	}
	// генерирует на карте новые предметы лута в указанном количестве
	bool GameSession::GenerateSessionLoots(unsigned count) {

		try
		{
			for (unsigned i = 0; i != count; ++i) {

				// смотрим есть ли место в текущей игровой сессии для единиц лута
				auto id = std::find(loots_id_.begin(), loots_id_.end(), false);

				if (id != loots_id_.end()) {
					// вся генерация выполняется только в том случае, если есть место для размещения
					// количество лута ограничено, см. конструктор игровой сессии

					// получаем количество типов лута карты
					int loot_types_count = static_cast<int>(session_map_->GetLootTypesCount());

					if (loot_types_count != 0) {
						// дальше генерируем если вообще есть какой-то лут, который может быть сгенерирован

						// получаем случайный индекс из массива типов лута
						size_t type = static_cast<size_t>(model::GetRandomInteger(0, loot_types_count - 1));
						// получаем числовое обозначение id для нового предмета
						size_t unique_id = static_cast<size_t>(std::distance(loots_id_.begin(), id));
						// получаем случайную позицию на карте и сразу делаем из неё позицию в сессии
						PlayerPosition position{ session_map_->GetRandomPosition() };
						// чтобы не размещать вот по целочисленной позиции, прибавляем случайное число от минус дельты до плюс дельты дороги
						// ВРЕМЕННО ОТКЛЮЧЕНО, чтобы упростить тестирование сбора предметов
						//position.AddRandomPlusMinusDelta(__ROAD_DELTA__);

						// отправляем на генерацию
						GenerateSessionLootImpl(type, unique_id, position);

						//// на основе индекса создаём новый игровой лут
						//GameLoot loot{ session_map_->GetLootType(type), type, unique_id, {} };
						//// получаем случайную точку на дорогах карты, точки игровой модели обрабатываются целочислеными значениями
						//model::Point position = session_map_->GetRandomPosition();

						//loot.pos_.x_ = (position.x + model::GetRandomDoubleRoundOne(-__ROAD_DELTA__, __ROAD_DELTA__));
						//loot.pos_.y_ = (position.y + model::GetRandomDoubleRoundOne(-__ROAD_DELTA__, __ROAD_DELTA__));

						/*loot.pos_.x_ = position.x;
						loot.pos_.y_ = position.y;*/

						//// добавляем в список лута в текущей игровой сессии
						//// уникальным индексом будет как и в случае с игроком, индекс в булевом массиве
						//session_loots_.emplace(unique_id, std::move(loot));

						//// поднинмаем флаг в булевом массиве по занятой позиции
						//*id = true;
					}
				}
			}

			return true;
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error("GameSession::GenerateLoot(unsigned)::Error\n" + std::string(e.what()));
		}
	}

	// выполняет проверку количестав лута на карте и генерацию нового
	bool GameSession::UpdateSessionLootsCount(int time) {

		// запрашиваем количество лута для генерации
		// после обработки коллизий на карте может быть мало предметов
		// или могут зайти новые игроки в игру
		auto new_loot_count = loot_gen_.Generate(
			std::chrono::milliseconds(time),
			static_cast<unsigned>(session_loots_.size()),
			static_cast<unsigned>(session_players_.size()));

		// вызываем генерацию нового лута
		return GenerateSessionLoots(new_loot_count);
	}

	// выполняет обновления текущих позиций игроков согласно расчитанных ранее будущих позиций
	bool GameSession::UpdateCurrentPlayersPositions() {

		for (auto& player : session_players_) {
			// просто обновляем позицию для всех игроков
			player.second.UpdateCurrentPosition();
		}

		return true;
	}

	// переносит предмет в сумку игрока, удаляет предмет с карты
	bool GameSession::PutLootInToTheBag(Player& player, size_t loot_id) {
		
		if (session_loots_.count(loot_id)) {

			if (loots_in_bags_.count(loot_id)) {
				// предмет никак не может находиться в сумке, его еще пока в сумку не запихнули, или не очистили после сдачи в бюро
				throw std::runtime_error("game_handler::GameSession::PutLootInToTheBag::Error::Loot already in somebody bag");
			}

			if (!player.CheckFreeBagSpace()) {
				// еще раз на всякий пожарный проверяем, что место есть, а то далее будет переннос предмета в другую мапу
				return false;
			}

			// переносим предмет из основной мапы лута на карте в мапу лута в сумках
			loots_in_bags_.emplace(std::pair{ loot_id, std::move(session_loots_.at(loot_id)) });
			// удаляем запись из основной мапы лута на карте
			session_loots_.erase(loot_id);

			// добавляем игроку указанный предмет в сумку
			return player.AddLoot(loot_id, &loots_in_bags_.at(loot_id));
		}

		return false;
	}

	// возвращает предметы в офис бюро находок, удаляет их из инвентаря и начисляет очки
	bool GameSession::ReturnLootsToTheOfficeImpl(Player& player) {

		// начинаем перебирать все что хранится в сумке игрока
		for (size_t i = 0; i != player.GetBagSize(); ++i) {

			// "возвращаем" лут в "офис" бюро находок 
			auto loot = player.ReturnLoot(i);
			
			// удаляем запись о луте из мапы лута в инвентаре игроков
			loots_in_bags_.erase(loot.index_);

			// снимаем флаг с булевого вектора лута, чтобы можно было снова создать элемент с таким id
			loots_id_[loot.index_] = false;
		}

		return true;
	}

	// выполняет расчёт коллизий и выполняет их согласно полученому массиву
	bool GameSession::HandlePlayersCollisionsActions() {

		// Для начала выполняем поиск коллизий
		auto events = FindCollisionEvents(*this);

		// Выполняем перебор найденых событий с вызовом соответствующих обработчиков
		for (const auto& event : events) {

			if (event.type == CollisionEventType::GATHERING) {

				if (session_players_.count(event.player_token)) {
					if (session_loots_.count(event.object_id)) {
						// если токен валиден, и предмет на карте, то отдаем работу соответствующему обработчику
						PutLootInToTheBag(session_players_.at(event.player_token), event.object_id);
					}
					
				}
				else {
					throw std::invalid_argument("game_handler::GameSession::UpdatePlayersCollisionsActions::Gathering::Error::Invalid player token");
				}

			}

			else if (event.type == CollisionEventType::RETURN) {

				if (session_players_.count(event.player_token)) {
					// если токен валиден, то отдаем работу соответствующему обработчику
					ReturnLootsToTheOfficeImpl(session_players_.at(event.player_token));
				}
				else {
					throw std::invalid_argument("game_handler::GameSession::UpdatePlayersCollisionsActions::Return::Error::Invalid player token");
				}
			}
		}

		return true;
	}

	// изменяет координаты игрока при движении параллельно дороге, на которой он стоит
	bool GameSession::PlayerParallelMovingImpl(Player& player, PlayerDirection direction,
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
		player_keep_moving ? player.SetFuturePosition(std::move(to)) :
			player.SetFuturePosition(std::move(to)).SetSpeed(0, 0);

		return true;
	}

	// изменяет координаты игрока при движении перпендикулярно дороге, на которой он стоит
	bool GameSession::PlayerCrossMovingImpl(Player& player, PlayerDirection direction,
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
		player_keep_moving ? player.SetFuturePosition(std::move(to)) :
			player.SetFuturePosition(std::move(to)).SetSpeed(0, 0);

		return true;
	}

	bool GameSession::CalculateFuturePlayerPositionImpl(Player& player, double time) {

		// записываем вектор ожидаемого приращения по положению персонажа
		PlayerPosition delta_pos{ player.GetCurrentPosition().x_ + (player.GetSpeed().xV_ * time),
			player.GetCurrentPosition().y_ + (player.GetSpeed().yV_ * time) };
		// чтобы лишнего не считать, проверяем есть ли у нас какое-то приращение в принципе
		if (delta_pos.x_ == 0 && delta_pos.y_ == 0) {
			return true;           // если приращения нет, то сразу выходим и не продолжаем
		}

		const model::Road* road = nullptr;    // готовим заготовку под "дорогу"
		// записываем во временную переменную, чтобы не делать лишних вызовов
		PlayerDirection direction = player.GetDirection();
		//PlayerDirection direction = player.get_speed_direction();
		PlayerPosition position = player.GetCurrentPosition();
		// округляем позицию до уровня логики model::Map
		model::Point point{ detail::RoundDoubleMathematic(position.x_), detail::RoundDoubleMathematic(position.y_) };

		// в зависимости от нашего направления запрашиваем дорогу
		// если игрок смотрит влево или вправо, полагаем, что будет движение по горизонтальной дороге
		if (direction == PlayerDirection::WEST || direction == PlayerDirection::EAST) {
			road = session_map_->GetHorizontalRoad(point);     // зарос или вернет дорогу, или nullptr
		}
		// если игрок смотрит вниз или вверх, полагаем, что будет движение по вертикальной дороге
		else if (direction == PlayerDirection::NORTH || direction == PlayerDirection::SOUTH) {
			road = session_map_->GetVerticalRoad(point);       // зарос или вернет дорогу, или nullptr
		}

		// тут важный момент, если мы стоим на требуемой для движения дороге,
		// то в принципе мы в состоянии спокойно двигаться проверив выход за границы дороги
		if (road) {
			// отдаём обработку методу перемещения параллельно дороге
			return PlayerParallelMovingImpl(player, direction, std::move(position), std::move(delta_pos), road);
		}
		// если же мы не стоим на требуемой - стоим на дороге перпендикулярной оси движения
		else {
			// отдаём обработку методу перемещения перпендикулярно дороге
			return PlayerCrossMovingImpl(player, direction, std::move(position), std::move(delta_pos), road);
		}
	}

	// выполняет расчёт и запись будущих позиций игроков в игровой сессии
	bool GameSession::UpdateFuturePlayersPositions(double time) {

		for (auto& [token, player] : session_players_) {
			// вызываем метод расчёта будущей позиции игрока
			// это еще НЕ перемещеие, это намерения о совершаемом в будущем перемещении
			CalculateFuturePlayerPositionImpl(player, time);
		}
		return true;
	}

	

	// -------------------------- class GameHandler --------------------------

	size_t MapPtrHasher::operator()(const model::Map* map) const noexcept {
		// Возвращает хеш значения, хранящегося внутри map
		return _hasher(map->GetName()) + _hasher(*(map->GetId()));
	}

	// создаёт игровую сессию с указанным идентификатором и названием карты
	GameSessionRestoreContext& GameHandler::RestoreGameSession(size_t session_id, std::string_view map_id) {

		// ищем запрошенную карту
		auto map = game_.FindMap(
			model::Map::Id{ std::string(map_id)});

		if (map == nullptr) {
			// если карта не найдена, то кидаем исключение
			throw std::invalid_argument("GameHandler::RestoreSessionFromBackUp::Error::No map with id {" + std::string(map_id) + "}");
		}

		if (!instances_.count(map)) {
			// если нет инстанса под данную карту добавляем указатель и создаём инстанс со списком сессий
			instances_.insert(std::make_pair(map, GameInstance()));
		}

		// создаём сессию, возвращаемое зачение не используем, так как вся работа будет идти через главный обработчик
		return restore_context_.SetRestoredGameSession(MakeNewGameSessoin(session_id, map));
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
			sessions_list_.clear();
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
				auto map = game_.FindMap(
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

		if (req.method_string() != http_handler::Method::GET && req.method_string() != http_handler::Method::HEAD) {
			// сюда вставить респонс о недопустимом типе
			return NotAllowedResponseImpl(std::move(req), http_handler::Method::GET, http_handler::Method::HEAD);
		}

		// ищем запрошенную карту
		auto map = game_.FindMap(model::Map::Id{ std::string(find_request_line) });

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
		std::string body_str = json_detail::GetMapsList(game_.GetMaps());
		response.set(http::field::content_length, std::to_string(body_str.size()));
		response.body() = body_str;

		return response;
	}

	

	// возвращает уже существующий токен по строковому представлению
	const Token* GameHandler::GetCreatedToken(const std::string& token) {
		if (tokens_list_.count(Token{ token })) {
			return &(tokens_list_.find(Token{ token })->first);
		}
		return nullptr;
	}

	const Token* GameHandler::GetUniqueToken(std::shared_ptr<GameSession> session) {
		std::lock_guard func_lock(mutex_);
		return GetUniqueTokenImpl(session);
	}

	// возвращает уникальный токен после генерации
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

		return AddUniqueTokenImpl(std::move(unique_token), session);
	}

	// добавляет конкретный токен с указателем на игровую сессию
	const Token* GameHandler::AddUniqueTokenImpl(const std::string& unique_token, std::shared_ptr<GameSession> session) {
		auto insert = tokens_list_.insert({ Token{unique_token},  session });
		return &(insert.first->first);
	}

	// добавляет конкретный токен с указателем на игровую сессию
	const Token* GameHandler::AddUniqueTokenImpl(Token&& unique_token, std::shared_ptr<GameSession> session) {
		auto insert = tokens_list_.insert({ std::move(unique_token),  session });
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

	// возвращает свободный уникальный идентификатор игровой сессии,
	// применяется при созданнии новых игровых сессий
	// Внимание! Метод не ставит флаг true в массиве!
	std::optional<size_t> GameHandler::GetNewUniqueSessionId() {

		// ищем свободный номер для игровой сессии
		auto id = std::find(sessions_id_.begin(), sessions_id_.end(), false);

		if (id == sessions_id_.end()) {
			// если ничего не нашли то возвращаем пустышку
			return std::nullopt;
		}

		return static_cast<size_t>((std::distance(sessions_id_.begin(), id)));
	}
	// создаёт новую игровую сессию по заданной карте с назначеным id
	std::shared_ptr<GameSession> GameHandler::MakeNewGameSessoin(size_t id, const model::Map* map) {

		auto ref = instances_.at(map)
			.emplace_back(std::make_shared<GameSession>(id, *this,
				game_.GetLootGenConfig(), map, __DEFAULT_SESSIONS_MAX_PLAYERS__, random_start_position_));

		sessions_list_.emplace(id, ref);                   // сохраняем данные в массиве быстрого поиска 
		sessions_id_[id] = true;                           // закрываем флаг, чтобы иметь идентификатор

		return ref;
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
		std::string body_str = json_detail::GetSessionStateList(session->GetPlayers(), session->GetLoots());
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
				std::lock_guard id_lock_(mutex_);                                          // закроем мьютекс на всякий пожарный
				auto new_session_id = GetNewUniqueSessionId();            // забираем себе новый id для сессии

				if (new_session_id.has_value()) {
					// если удалось получить идентификатор то создаём новую сессию
					ref = MakeNewGameSessoin(new_session_id.value(), map);
					//ref = instances_.at(map)
					//	.emplace_back(std::make_shared<GameSession>(new_session_id.value(), *this, 
					//		game_.GetLootGenConfig(), map, __DEFAULT_SESSIONS_MAX_PLAYERS__, random_start_position_));

					//sessions_list_.emplace(new_session_id.value(), ref);     // сохраняем данные в массиве быстрого поиска 
					//sessions_id_[new_session_id.value()] = true;                           // закрываем флаг, чтобы иметь идентификатор
				}
				else {
					// если местоф нет, то скажем - увы и ах
					return CommonFailResponseImpl(std::move(req), 
						http::status::service_unavailable, "noPlace", "GameServer has no free place");
				}
			}
		}

		// если нет ни одной открытой сессии на данной карте
		else {
			std::lock_guard id_lock_(mutex_);                                          // закроем мьютекс на всякий пожарный
			auto new_session_id = GetNewUniqueSessionId();            // забираем себе новый id для сессии

			if (new_session_id.has_value()) {
				// добавляем указатель и создаём инстанс со списком сессий
				instances_.insert(std::make_pair(map, GameInstance()));
				// создаём новую сессию
				ref = MakeNewGameSessoin(new_session_id.value(), map);

				//// так как у нас еще нет никаких сессий то тупо создаём новую внутри 
				//// c максимумом, для примера, в 200 игроков (см. конструктор GameSession)
				//// тут же получаем шару на неё, и передаем ей управление по вступлению в игру
				//ref = instances_.at(map)
				//	.emplace_back(std::make_shared<GameSession>(new_session_id.value(), *this, 
				//		game_.GetLootGenConfig(),map, __DEFAULT_SESSIONS_MAX_PLAYERS__, random_start_position_));

				//sessions_list_.emplace(new_session_id.value(), ref);     // сохраняем данные в массиве быстрого поиска 
				//sessions_id_[new_session_id.value()] = true;                           // закрываем флаг, чтобы иметь идентификатор
			}
			else {
				// если местоф нет, то скажем - увы и ах
				return CommonFailResponseImpl(std::move(req),
					http::status::service_unavailable, "noPlace", "GameServer has no free place");
			}
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


	// назначает игровую сессию
	GameSessionRestoreContext& GameSessionRestoreContext::SetRestoredGameSession(std::shared_ptr<GameSession> session) {
		session_ = session;
		return *this;
	}

	// воссоздаёт игрового персонажа
	GameHandler& GameSessionRestoreContext::RestoreGamePlayer(const SerializedPlayer& player) {
		
		// восстанавливаем уникальный токен игрока
		auto token = game_.AddUniqueTokenImpl(player.GetToken(), session_);
		// воссоздаём игрока из полученных данных и токена
		session_->AddPlayerImpl(player.GetId(), player.GetName(), token, player.GetBagCapacity())
			.SetCurrentPosition(std::move(player.GetCurrentPosition()))
			.SetFuturePosition(std::move(player.GetFuturePosition()))
			.SetDirection(std::move(player.GetDirection()))
			.SetSpeed(std::move(player.GetSpeed()))
			.SetScore(player.GetScore());

		for (size_t i = 0; i != player.GetLootCount(); ++i) {
			// запрашиваем восстанновление лута в инвентаре
			RestoreGameLoot(player.GetLootByIndex(i));
		}

		return game_;
	}

	// воссоздаёт игровой лут
	GameHandler& GameSessionRestoreContext::RestoreGameLoot(const SerializedLoot& loot) {
		// запрашиваем генерацию лута по заданым параметрам
		session_->GenerateSessionLootImpl(loot.GetType(), loot.GetId(), loot.GetPosition());
		// если лут лежал у игрока в инветаре
		if (loot.GetToken() != "onMap") {
			// запрашиваем размещение лута в интвентаре конкретного игрока
			// к моменту запроса на восстаовлеие лута, игроки должны быть восстановлены
			session_->PutLootInToTheBag(
				*(session_->GetPlayer(game_.GetCreatedToken(loot.GetToken()))), loot.GetId()
			);
		}
		
		return game_;
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