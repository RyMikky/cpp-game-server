#pragma once

#include "domain.h"
//#include "model.h"
#include "json_loader.h"
#include "boost_json.h"

#include <vector>
#include <memory>
#include <filesystem>
#include <unordered_map>

namespace game_handler {

	namespace fs = std::filesystem;

	class GameHandler; // forward-defenition

	/*class Player {
	public:
		Player() = default;

		Player(const Player&) = delete;
		Player& operator=(const Player&) = delete;
		Player(Player&&) = default;
		Player& operator=(Player&&) = default;

		Player(uint16_t id, std::string_view name, const Token* token)
			: id_(id), name_(name), token_(token) {
		};

		uint16_t get_player_id() const {
			return id_;
		}
		std::string_view get_player_name() const {
			return name_;
		}
		std::string_view get_player_token() const {
			return **token_;
		}

	private:
		uint16_t id_ = 65535;
		std::string name_ = "dummy"s;
		const Token* token_ = nullptr;
	};

	using PlayerPtr = const Player*;*/

	class TokenHasher {
	public:
		std::size_t operator()(const Token& token) const noexcept;
	private:
		std::hash<std::string> _hasher;
	};

	class TokenPtrHasher {
	public:
		std::size_t operator()(const Token* token) const noexcept;
	private:
		std::hash<std::string> _hasher;
	};

	// класс-обработчик текущей игровой сессии
	class GameSession : public std::enable_shared_from_this<GameSession> {
	public:
		GameSession(GameHandler& handler, model::Map* map, size_t max_players) 
			: game_handler_(handler), session_game_map_(map), players_id_(max_players) {
		}

		bool add_new_player(std::string_view name);

		bool remove_player(const Token* token);
		bool remove_player(std::string_view name);
		bool remove_player(uint16_t id);

		Player* get_player_by_token(const Token* token);

		auto cbegin() {
			return session_players_.cbegin();
		}
		auto cend() {
			return session_players_.cend();
		}

	private:
		GameHandler& game_handler_;
		model::Map* session_game_map_;
		std::vector<bool> players_id_;
		std::unordered_map<const Token*, Player, TokenPtrHasher> session_players_;
	};

	class MapPtrHasher {
	public:
		std::size_t operator()(const model::Map* map) const noexcept;
	private:
		std::hash<std::string> _hasher;
	};

	//struct MapPtrHasher {
	//	size_t operator()(const model::Map* map) const {
	//		// Возвращает хеш значения, хранящегося внутри value
	//		std::string name = map->GetName();
	//		std::string id = *(map->GetId());
	//		return std::hash<std::string>(name) + std::hash<std::string>(id);
	//	}
	//};

	// структура хранения и изначального создания игровых сессий
	using GameInstance = std::vector<std::shared_ptr<GameSession>>;
	// структура для контроля инстансов игровых сессий по картам
	using GameMapInstance = std::unordered_map<model::Map*, GameInstance, MapPtrHasher>;
	// структура для быстрого поиска игрока по токену, реализуется реверсивным добавлением из сессии в обработчик
	using GameTokenList = std::unordered_map<Token, std::shared_ptr<GameSession>, TokenHasher>;

	class GameHandler {
		friend class GameSession;
		// даёт доступ игровой сессии, для создания и удаления токенов
		// такая "сложность" реализуется для скорости поиска игроков в каждой из запущенных сессий
		// потенциально, в этом особой нужды нет, но тогда, поиск конкретного игрока по токену будет идти циклически 
		// в каждой из открытыхз сессий путём банального перебора, а так, GameHandler будет "из коробки" знать
		// кто есть в игре и в каком из игровых инстансов со временем поиска в диапазоне от O(N) до O(LogN) - unordered_map
	public:
		// отдаём создание игровой модели класус обработчику игры
		explicit GameHandler(const fs::path& configuration) : io_context_(), strand_(io_context_.get_executor()){
			game_ = json_loader::LoadGame(configuration);
		}

		// Обеспечивает вход игрока в игровую сессию
		std::string enter_to_game_session(std::string_view name, std::string_view map);

	protected: // протектед блок доступен только friend class -у для обратной записи данных и получения уникальных токенов
		/* 
			Реверсивный метод, который вызывается из игровой сессии.
			Служит для получения уникального токена при добавлении нового игрока.
		*/
		const Token* get_unique_token(std::shared_ptr<GameSession> session);
		bool reset_token(std::string_view token);

	private:
		model::Game game_;

		// контекст и стренд необхордимы для работы в асинхронном режиме
		// так как обработчик "выдаёт" уникальные токены, то возможна ситуация, когда с двух параллельных потоков прилетит запрос
		// на создание уникального токена, получаем состояние гонки и потенциальную ошибку генерации уникального токена, потому
		// генерация и определение уникального токена будет выполняться в странде. Выше сказанное справедливо для всех concurence
		boost::asio::io_context io_context_;
		boost::asio::strand<boost::asio::io_context::executor_type> strand_;

		GameMapInstance instance_;
		GameTokenList token_list_;

		const Token* get_unique_token_impl(std::shared_ptr<GameSession> session);
		bool reset_token_impl(std::string_view token);
	};

} //namespace game_handler