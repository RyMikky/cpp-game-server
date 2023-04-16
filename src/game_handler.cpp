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
		//// пользуюсь поиском чтобы получить иттератор с конкретной парой указатель/сессия
		//auto it = std::find(token_list_.begin(), token_list_.end(), remove);

		//if (it != token_list_.end()) {
		//	// запрашиваю удаление по найденному указателю
		//	token_list_.at(remove)->remove_player(&it->first);
		//	// удаляю запись из архива
		//	return token_list_.erase(remove);
		//}
		//else {
		//	return false;
		//}

		if (token_list_.count(remove)) {
			token_list_.at(remove)->remove_player(&remove);
			return token_list_.erase(remove);
		}
		else {
			return false;
		}
	}

} //namespace game_handler