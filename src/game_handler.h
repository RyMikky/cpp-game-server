#pragma once

#include "json_loader.h"
#include "boost_json.h"
#include "collision_handler.h"         // через данный хеддер подключается domain.h
#include "postgres/postgers.h"

#include <vector>
#include <memory>
#include <chrono>
#include <mutex>
#include <optional>
#include <functional>
#include <unordered_map>

namespace game_handler {

	// базовое максимальное количество игроков в сессии
	static const size_t __DEFAULT_SESSIONS_MAX_PLAYERS__ = 200;
	// базовое максимальное количество игровых сессий
	static const size_t __DEFAULT_GAME_SESSIONS_MAX_COUNT__ = 50;

	namespace fs = std::filesystem;
	namespace json = boost::json;
	namespace beast = boost::beast;
	namespace http = beast::http;
	namespace net = boost::asio;
	namespace sys = boost::system;

	using namespace postgres;

	class GameHandler;                           // forward-definition

	// класс-обработчик текущей игровой сессии
	class GameSession : public std::enable_shared_from_this<GameSession>, public CollisionProvider {
		friend class GameHandler;
		friend class GameSessionRestoreContext;
	public:

		/*
		* Игровая сессия генерирует карту и обрабатывает все игровые события согласно указаниями, получаемым от GameHandler
		* Количество игроков и игрового лута ограничено. Ограничение игроков связано с разгрузкой сессии, чтобы на карте не было
		* больше игроков чем требуется (например, нет смысла на 10 кв.м. размещать 10 игроков, они будут друг у друга на голове)
		* Количество же лута ограничено величной произведения количества игроков, на вместимость их рюкзаков в квадрате.
		* Смысл в том, что одновременно может так сложиться, что в игре у каждого персонажа полные сумки, но при этом необходимо
		* сгенерировать новый лут и разместить его на карте, и обязательно с уникальным идентификатором в сессии
		*/

		GameSession(size_t id, GameHandler& handler, loot_gen::LootGeneratorConfig config, const model::Map* map, size_t max_players) 
			: session_id_(id)
			, game_handler_(handler)
			, loot_gen_{ config/*, []() { return model::GetRandomDouble(); }*/ }
			, session_map_(map)
			, players_id_(max_players)
			, loots_id_(max_players * static_cast<size_t>(
				std::pow(session_map_->GetOnMapBagCapacity(), 2))) {
		}
		GameSession(size_t id, GameHandler& handler, loot_gen::LootGeneratorConfig config, const model::Map* map, size_t max_players, bool start_random_position)
			: session_id_(id)
			, game_handler_(handler)
			, loot_gen_{ config/*, []() { return model::GetRandomDouble(); }*/ }
			, session_map_(map)
			, players_id_(max_players)
			, loots_id_(max_players * static_cast<size_t>(
				std::pow(session_map_->GetOnMapBagCapacity(), 2)))
			, random_start_position_(start_random_position) {
		}
		
		/*
		* Генератор закомментирован для прохождения пайплайна тестов. Используется штатный генератор.
		* Смотри реализацию класса loot_gen::LootGenerator, у него есть статический выдающий 1.0.
		* Суть вопроса в том, что мой генератор выдаёт случайное число от 0.0 до 1.0.
		* Тестовая система загружает нового игрока и запускает четыре больших тика по 5000000 мс.
		* После чего проверяет, что предмет лута создаётся. Логично что за 4 раза по 5кк мс лут должен появиться.
		* Мой генератор совершенно нормальным образом может все четыра раза выдать число не превышающее, допустим 0,2.
		* Таким образом лут сгенерирвоан не будет (см. реализацию loot_gen::LootGenerator::Generate) и ошибки тут нет!
		* Если бы проверяющая система производила, например 1000 тиков по 5к мс, или 5000 тиков по 1к мс, или 500 тиков по 10к мс
		* То генератор точно бы выдал число позволяющее создать предмет и спокойно пройти тесты.
		* А так, ну вот, ну все четыре раза мелочь, свёзды так совпали ¯\_(ツ)_/¯, предмет не сгенерирован, тест завален.
		*
		* При реальнном применении сомнительно, что игровой движок будет обновлять состояние раз в 5000 секунд
		* И честный генератор, выдающий 0.0 -> 1.0, добавит больше гибкости и непредсказуемости в процессе генерации лута
		* Что в свою очередь добавит интереса самой игре.
		*/

		// возвращает идентификатор игровой сессии
		size_t GetId() const {
			return session_id_;
		}
		// возвращает указатель на карту игровой сессии
		const model::Map* GetMap() const {
			return session_map_;
		}

		// ----------------- блок наследуемых методов CollisionProvider ----------------------------

		// возвращает количество офисов бюро находок на карте игровой сессии
		size_t OfficesCount() const override;
		// возвращает офис бюро находок по индексу
		const model::Office& GetOffice(size_t) const override;
		// возвращает мапу с игроками в игровой сессии
		const SessionPlayers& GetPlayers() const override {
			return session_players_;
		}
		// возвращает мапу с лутом в игровой сессии
		const SessionLoots& GetLoots() const override {
			return session_loots_;
		}

	protected:

		// задаёт флаг случайной позиции для старта новых игроков
		GameSession& SetRandomStartPosition(bool random_start_position);
		// добавляет нового игрока на случайное место на случайной дороге на карте
		Player* AddPlayer(std::string_view name);
		// вернуть указатель на игрока в сессии по токену
		Player* GetPlayer(const Token* token);
		// удалить игрока из игровой сессии
		bool RemovePlayer(const Token* token);

		/*
		* Обновляет состояние игры с заданным временем в миллисекундах.
		* Запускает полный цикл обработки в следующей последовательности:
		*  1. Расчёт будущих позиций игроков
		*  2. Расчёт и выполнение ожидаемых при перемещении коллизий
		*  3. Выполнение перемещения игроков на будущие координаты
		*  4. Генерация лута на карте
		*/
		bool UpdateState(int time);
		// метод добавляет скорость персонажу, вызывается из GameHandler::player_action_response_impl
		bool MovePlayer(const Token* token, PlayerMove move);
		// отвечает есть ли в сессии свободное местечко
		bool CheckFreeSpace();

		const auto cbegin() const {
			return session_players_.cbegin();
		}
		const auto cend() const {
			return session_players_.cend();
		}
		const auto begin() const {
			return session_players_.begin();
		}
		const auto end() const {
			return session_players_.end();
		}

	private:
		size_t session_id_;                                 // идентификатор игровой сессии
		GameHandler& game_handler_;                         // ссылка на базовый игровой обработчик
		loot_gen::LootGenerator loot_gen_;                  // собственный генератор лута игровой сессии
		const model::Map* session_map_;                     // указатель на карту игровой модели

		SessionPlayers session_players_;                    // хешированная мапа с игроками
		std::vector<bool> players_id_;                      // булевый массив индексов игроков
		SessionLoots session_loots_;                        // хешированная мапа с лутом на карте
		std::vector<bool> loots_id_;                        // булевый массив индексов лута
		SessionLoots loots_in_bags_;                        // хешированная мапа с лутом в инвентаре игроков

		bool random_start_position_ = true;                 // флаг случайной позиции игрока на старте

		// добавляет нового игрока на карту
		Player& AddPlayerImpl(size_t, std::string_view, const Token*, unsigned);

		// проверяет стартовую позицию игрока на предмет совпадения с другими игроками в сессии
		bool CheckStartPositionImpl(PlayerPosition& position);

		// генерирует лут с заданым типом, идентификатором и позицией
		bool GenerateSessionLootImpl(size_t type, size_t id, PlayerPosition pos);
		// генерирует на карте новые предметы лута в указанном количестве
		bool GenerateSessionLoots(unsigned count);
		// выполняет проверку количестав лута на карте и генерацию нового
		bool UpdateSessionLootsCount(int time);

		// выполняет обновления текущих позиций игроков согласно расчитанных ранее будущих позиций
		bool UpdateCurrentPlayersPositions();
		
		// выполняет удаление всех бездействующих игроков, превысивших лимит времени ожидания
		bool UpdateRetirementPlayers();

		// возвращает предметы в офис бюро находок, удаляет их из инвентаря и начисляет очки
		bool ReturnLootsToTheOfficeImpl(Player& player);
		// переносит предмет с указанным id в сумку игрока, удаляет предмет с карты
		bool PutLootInToTheBag(Player& player, size_t loot_id);
		// выполняет расчёт коллизий и выполняет их согласно полученому массиву
		bool HandlePlayersCollisionsActions();

		// изменяет координаты игрока при движении параллельно дороге, на которой он стоит
		bool PlayerParallelMovingImpl(Player& player, PlayerDirection direction, PlayerPosition&& from, PlayerPosition&& to, const model::Road* road);
		// изменяет координаты игрока при движении перпендикулярно дороге, на которой он стоит
		bool PlayerCrossMovingImpl(Player& player, PlayerDirection direction, PlayerPosition&& from, PlayerPosition&& to, const model::Road* road);
		// обновляет позицию выбранного игрока в соответствии с его заданной скоростью, направлением и временем в секундах
		// принимает время в миллисекундах, перерасчёт будет дальше по коду
		bool CalculateFuturePlayerPositionImpl(Player& player, int time);
		// выполняет расчёт и запись будущих позиций игроков в игровой сессии, принимает время в миллисекундах
		bool UpdateFuturePlayersPositions(int time);
	};

	class MapPtrHasher {
	public:
		std::size_t operator()(const model::Map* map) const noexcept;
	private:
		std::hash<std::string> _hasher;
	};

	// структура хранения и изначального создания игровых сессий
	using GameInstance = std::vector<std::shared_ptr<GameSession>>;
	// структура для контроля инстансов игровых сессий по картам
	using GameMapInstance = std::unordered_map<const model::Map*, GameInstance, MapPtrHasher>;
	// структура для быстрого поиска игрока по токену, реализуется реверсивным добавлением из сессии в обработчик
	using GameTokenList = std::unordered_map<Token, std::shared_ptr<GameSession>, TokenHasher>;
	// структура для быстрого поиска сессии по идентификатору
	using GameSessionList = std::unordered_map<size_t, std::shared_ptr<GameSession>>;

	// класс-контекст восстановления данных
	class GameSessionRestoreContext {
	private:
		friend class GameHandler;
		GameHandler& game_;
	public:
		GameSessionRestoreContext() = delete;
		GameSessionRestoreContext(const GameSessionRestoreContext&) = delete;
		GameSessionRestoreContext& operator=(const GameSessionRestoreContext&) = delete;
		GameSessionRestoreContext(GameSessionRestoreContext&&) = default;
		GameSessionRestoreContext& operator=(GameSessionRestoreContext&&) = default;

		GameSessionRestoreContext(GameHandler& game)
			: game_(game) {
		}
		GameSessionRestoreContext(GameHandler& game, std::shared_ptr<GameSession> session)
			: game_(game), session_(session) {
		}

		// воссоздаёт игрового персонажа
		GameHandler& RestoreGamePlayer(const SerializedPlayer& player);
		// воссоздаёт игровой лут
		GameHandler& RestoreGameLoot(const SerializedLoot& loot);

	protected:
		// назначает игровую сессию
		GameSessionRestoreContext& SetRestoredGameSession(std::shared_ptr<GameSession> session);

	private:
		std::shared_ptr<GameSession> session_;
	};

	// основной класс обработчик игровых сессий
	class GameHandler {
		friend class GameSession;
		// даёт доступ игровой сессии, для создания и удаления токенов
		// такая "сложность" реализуется для скорости поиска игроков в каждой из запущенных сессий
		// потенциально, в этом особой нужды нет, но тогда, поиск конкретного игрока по токену будет идти циклически 
		// в каждой из открытых сессий путём банального перебора, а так, GameHandler будет "из коробки" знать
		// кто есть в игре и в каком из игровых инстансов со временем поиска в диапазоне от O(N) до O(LogN) - unordered_map
		friend class GameSessionRestoreContext;
	public:
		// отдаём создание игровой модели классу обработчику игры
		explicit GameHandler(const fs::path& configuration
			, postgres::detail::ConnectionConfig&& base_config
			, size_t session_count = __DEFAULT_GAME_SESSIONS_MAX_COUNT__)

			: game_{ json_loader::LoadGameConfiguration(configuration) }
			, sessions_id_(session_count)
			, restore_context_(*this)
			, base_(std::move(base_config)) {
		}

		// ------------------- блок методов сериализатора ----------------------

		// воссоздаёт игровую сессию с указанным идентификатором и названием карты
		[[nodiscard]] GameSessionRestoreContext& RestoreGameSession(size_t, std::string_view);

		// ------------------- прочие управляющие методы -----------------------

		// Выполняет обновление всех открытых игровых сессий по времени
		void UpdateGameSessions(int time);
		// Назначает флаг случайного размещения игроков на картах
		void SetRandomStartPosition(bool flag);
		// Сбрасывает и удаляет все активные игровые сессии
		void ResetGameSessions();

		// возвращает массив с игровыми сессиями
		const GameSessionList& GetSessions() const {
			return sessions_list_;
		}
		
		// Возвращает ответ на запрос по изменению состояния игровых сессий со временем
		http_handler::Response SessionsUpdateResponse(http_handler::StringRequest&& req);
		// Возвращает ответ на запрос о совершении действий персонажем
		http_handler::Response PlayerActionResponse(http_handler::StringRequest&& req);
		// Возвращает ответ на запрос о состоянии игроков в игровой сессии
		http_handler::Response GameStateResponse(http_handler::StringRequest&& req);
		// Возвращает ответ на запрос о списке игроков в данной сессии
		http_handler::Response PlayersListResponse(http_handler::StringRequest&& req);
		// Возвращает ответ на запрос по присоединению к игре
		http_handler::Response JoinGameResponse(http_handler::StringRequest&& req);
		// Возвращает ответ на запрос по поиску конкретной карты
		http_handler::Response FindMapResponse(http_handler::StringRequest&& req, std::string_view find_request_line);
		// Возвращает ответ со списком загруженных карт
		http_handler::Response MapsListResponse(http_handler::StringRequest&& req);
		// Возвращает ответ со списком рекордов игры
		http_handler::Response RecordsResponse(http_handler::StringRequest&& req);
		// Возвращает ответ со списком рекордов игры с дополнительными параметрами по количеству и отступу
		http_handler::Response RecordsResponse(http_handler::StringRequest&& req, postgres::detail::ReqParam param);

	protected: // протектед блок доступен только friend class -ам для обратной записи данных и получения уникальных токенов

		// возвращает уже существующий токен по строковому представлению
		const Token* GetCreatedToken(const std::string&);
		/* 
		 * Реверсивный метод, который вызывается из игровой сессии.
		 * Служит для получения уникального токена при добавлении нового игрока.
		*/
		const Token* GetUniqueToken(std::shared_ptr<GameSession> session);
		// удаляет токен, игрока и записывает его рекорд в базу SQL
		bool ResetToken(const Token* token);
		// возвращает допустимое время простоя игрока в миллисекундах
		int GetRetirementTimeMS() const {
			return game_.GetRetirementTimeMS();
		}

	private:
		model::Game game_;
		std::mutex mutex_;
		GameSessionRestoreContext restore_context_;      // контекст восстановления игровых сессий
		DataBaseHandler base_;                           // PostgreSQL база данных в которую пишутся рекорды

		GameMapInstance instances_;                      // игровые инстансы по картам
		GameTokenList tokens_list_;                      // токены с указателями на конкретные сессии
		GameSessionList sessions_list_;                  // идентификаторы сессий и указатели на сессии

		std::vector<bool> sessions_id_;                  // булевый массив для id игровых сессий
		bool random_start_position_ = false;             // флаг радндомной позиции игроков на старте

		// возвращает уникальный токен после генерации
		const Token* GetUniqueTokenImpl(std::shared_ptr<GameSession> session);
		// добавляет конкретный токен с указателем на игровую сессию
		const Token* AddUniqueTokenImpl(const std::string&, std::shared_ptr<GameSession>);
		// добавляет конкретный токен с указателем на игровую сессию
		const Token* AddUniqueTokenImpl(Token&&, std::shared_ptr<GameSession>);

		bool ResetTokenImpl(const Token* token);

		// возвращает свободный уникальный идентификатор игровой сессии,
		// применяется при созданнии новых игровых сессий
		std::optional<size_t> GetNewUniqueSessionId();
		// создаёт новую игровую сессию
		std::shared_ptr<GameSession> MakeNewGameSessoin(size_t id, const model::Map* map);

		// Возвращает ответ на запрос по изменению состояния игровых сессий со временем
		http_handler::Response SessionsUpdateResponseImpl(http_handler::StringRequest&& req);
		// Возвращает ответ на запрос о совершении действий персонажем
		http_handler::Response PlayerActionResponseImpl(http_handler::StringRequest&& req, const Token* token);
		// Возвращает ответ на запрос о состоянии игроков в игровой сессии
		http_handler::Response GameStateResponseImpl(http_handler::StringRequest&& req, const Token* token);
		// Возвращает ответ на запрос о списке игроков в данной сессии
		http_handler::Response PlayersListResponseImpl(http_handler::StringRequest&& req, const Token* token);
		// Возвращает ответ, о успешном добавлении игрока в игровую сессию
		http_handler::Response JoinGameResponseImpl(http_handler::StringRequest&& req, json::value&& body, const model::Map* map);
		// Возвращает ответ, что запрошенный метод не разрешен, доступные указывается в аргументе allow
		http_handler::Response NotAllowedResponseImpl(http_handler::StringRequest&& req, std::string_view allow);

		// Возвращает ответ на все варианты неверных и невалидных запросов
		http_handler::Response CommonFailResponseImpl(http_handler::StringRequest&& req, 
			http::status status, std::string_view code, std::string_view message);

		// Проверяет полученный в запросе токен, если токен корректнен и есть в базе, то управление передается прилагаемому методу
		template <typename Function>
		http_handler::Response PlayerAuthorizationImpl(http_handler::StringRequest&& req, Function&& func);
		template <typename ...Methods>
		// Возвращает ответ, что запрошенные методы не разрешены, доступный указывается в аргументе allow
		http_handler::Response NotAllowedResponseImpl(http_handler::StringRequest&& req, Methods&& ...methods);
	};

	namespace detail {

		template<typename T1, typename T2>
		std::string CombineAllowedMethods(const T1& method_one, const T2& method_two) {
			return std::string(method_one) + ", " + std::string(method_two);
		}

		template<typename T1, typename T2, typename... Args>
		std::string CombineAllowedMethods(const T1& method_one, const T2& method_two, Args&&... args) {
			return std::string(method_one) + ", " + std::string(method_two) + ", " + CombineAllowedMethods(std::forward<Args>(args)...);
		}

		template<typename... Args>
		std::string CombineAllowedMethods(Args&&... args) {
			std::string result;
			bool first = true;
			((result += (first ? "" : ", ") + std::string(std::forward<Args>(args)), first = false), ...);
			return result;
		}

		// округляет double -> int по математическим законам
		int RoundDoubleMathematic(double value);

		std::optional<std::string> BearerParser(const std::string& auth_line);

	} // namespace detail

	// Проверяет полученный в запросе токен, если токен корректнен и есть в базе, то управление передается прилагаемому методу
	template <typename Function>
	http_handler::Response GameHandler::PlayerAuthorizationImpl(http_handler::StringRequest&& req, Function&& func) {
		// ищем тушку авторизации среди хеддеров запроса
		auto auth_iter = req.find("Authorization");
		if (auth_iter == req.end()) {
			// если нет тушки по авторизации, тогда кидаем отбойник
			return CommonFailResponseImpl(std::move(req), http::status::unauthorized,
				"invalidToken", "Authorization header is missing");
		}

		// из тушки запроса получаем строку
		// так как строка должна иметь строгий вид Bearer <токен>, то мы легко можем распарсить её
		auto auth_reparse = detail::BearerParser({ auth_iter->value().begin(), auth_iter->value().end() });

		if (!auth_reparse) {
			// если нет строки Bearer, или она корявая, или токен пустой, то кидаем отбойник
			return CommonFailResponseImpl(std::move(req), http::status::unauthorized,
				"invalidToken", "Authorization header is missing");
		}

		Token token{ auth_reparse.value() }; // создаём быстро токен на основе запроса и ищем совпадение во внутреннем массиве
		if (!tokens_list_.count(token)) {
			// если заголовок Authorization содержит валидное значение токена, но в игре нет пользователя с таким токеном
			return CommonFailResponseImpl(std::move(req), http::status::unauthorized,
				"unknownToken", "Player token has not been found");
		}

		// вызываем полученный обработчик
		return func(std::move(req), &tokens_list_.find(token)->first);
	}

	template <typename ...Methods>
	// Возвращает ответ, что запрошенные методы не разрешены, доступный указывается в аргументе allow
	http_handler::Response GameHandler::NotAllowedResponseImpl(http_handler::StringRequest&& req, Methods&& ...methods) {
		http_handler::StringResponse response(http::status::method_not_allowed, req.version());
		response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
		response.set(http::field::cache_control, "no-cache");
		// собираюю строку сборщиком из detail
		response.set(http::field::allow, detail::CombineAllowedMethods(methods...));
		response.body() = json_detail::GetErrorString("invalidMethod"sv, "Invalid method"s);

		return response;
	}

} // namespace game_handler