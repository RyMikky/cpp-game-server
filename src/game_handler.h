#pragma once

#include "domain.h"
#include "json_loader.h"
#include "boost_json.h"

#include <vector>
#include <memory>
#include <chrono>
#include <mutex>
#include <optional>
#include <functional>
#include <unordered_map>

namespace game_handler {

	namespace fs = std::filesystem;
	namespace json = boost::json;
	namespace beast = boost::beast;
	namespace http = beast::http;
	namespace net = boost::asio;
	namespace sys = boost::system;

	class GameHandler; // forward-definition

	class GameTimer : public std::enable_shared_from_this<GameTimer> {
		using Timer = net::steady_timer;
		using Function = std::function<void(std::chrono::milliseconds delta)>;
	public:
		GameTimer(http_handler::Strand& strand, std::chrono::milliseconds period, Function&& function)
			: api_strand_(strand), period_(period), function_(std::move(function)) {
		}

		GameTimer(const GameTimer&) = delete;
		GameTimer& operator=(const GameTimer&) = delete;
		GameTimer(GameTimer&&) = default;
		GameTimer& operator=(GameTimer&&) = default;

		// запускает выполнение таймера
		GameTimer& Start();
		// останавливает выполнение таймера
		GameTimer& Stop();
		// назначает новый временной интервал таймера
		GameTimer& SetPeriod(std::chrono::milliseconds period);
		// назначение новой функции для выполнения по таймеру
		GameTimer& SetFunction(Function&& function);

	private:
		http_handler::Strand& api_strand_;
		std::chrono::milliseconds period_;
		Function function_;
		Timer timer_{ api_strand_, period_ };

		bool execution_ = false;
		// основная имплементация выполнения таймера
		void ExecutionImpl(sys::error_code ec);
	};

	// класс-обработчик текущей игровой сессии
	class GameSession : public std::enable_shared_from_this<GameSession> {
		friend class GameHandler;
	public:
		GameSession(GameHandler& handler, const model::Map* map, size_t max_players) 
			: game_handler_(handler), session_game_map_(map), players_id_(max_players) {
		}
		GameSession(GameHandler& handler, const model::Map* map, size_t max_players, bool start_random_position)
			: game_handler_(handler), session_game_map_(map), players_id_(max_players), random_start_position_(start_random_position) {
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
		// обновляет состояние игры с заданным временем в миллисекундах
		bool UpdateState(int time);
		// метод добавляет скорость персонажу, вызывается из GameHandler::player_action_response_impl
		bool MovePlayer(const Token* token, PlayerMove move);
		// отвечает есть ли в сессии свободное местечко
		bool CheckFreeSpace();
		
		// чекает стартовую позицию на предмет совпадения с другими игроками в сессии
		const SessionPlayers& GetPlayers() const {
			return session_players_;
		}

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
		GameHandler& game_handler_;
		const model::Map* session_game_map_;
		std::vector<bool> players_id_;
		SessionPlayers session_players_;

		bool random_start_position_ = true;

		// изменяет координаты игрока при движении параллельно дороге, на которой он стоит
		bool ParallelMovingImpl(Player& player, PlayerDirection direction, PlayerPosition&& from, PlayerPosition&& to, const model::Road* road);
		// изменяет координаты игрока при движении перпендикулярно дороге, на которой он стоит
		bool CrossMovingImpl(Player& player, PlayerDirection direction, PlayerPosition&& from, PlayerPosition&& to, const model::Road* road);

		bool UpdatePlayerPosition(Player& player, double time);
		bool CheckStartPositionImpl(PlayerPosition& position);
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

	class GameHandler {
		friend class GameSession;
		// даёт доступ игровой сессии, для создания и удаления токенов
		// такая "сложность" реализуется для скорости поиска игроков в каждой из запущенных сессий
		// потенциально, в этом особой нужды нет, но тогда, поиск конкретного игрока по токену будет идти циклически 
		// в каждой из открытых сессий путём банального перебора, а так, GameHandler будет "из коробки" знать
		// кто есть в игре и в каком из игровых инстансов со временем поиска в диапазоне от O(N) до O(LogN) - unordered_map
	public:
		// отдаём создание игровой модели классу обработчику игры
		explicit GameHandler(const fs::path& configuration)
			: game_simple_(json_loader::LoadGameConfiguration(configuration)) {
		}

		// Выполняет обновление всех открытых игровых сессий по времени
		void UpdateGameSessions(int time);
		// Назначает флаг случайного размещения игроков на картах
		void SetRandomStartPosition(bool flag);
		// Сбрасывает и удаляет все активные игровые сессии
		void ResetGameSessions();
		
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

	protected: // протектед блок доступен только friend class -у для обратной записи данных и получения уникальных токенов
		/* 
			Реверсивный метод, который вызывается из игровой сессии.
			Служит для получения уникального токена при добавлении нового игрока.
		*/
		const Token* GetUniqueToken(std::shared_ptr<GameSession> session);
		// удаляет токен из базы
		bool ResetToken(std::string_view token);

	private:
		model::Game game_simple_;
		std::mutex mutex_;

		GameMapInstance instances_;
		GameTokenList tokens_list_;

		bool random_start_position_ = false;             // флаг радндомной позиции игроков на старте

		const Token* GetUniqueTokenImpl(std::shared_ptr<GameSession> session);
		bool ResetTokenImpl(std::string_view token);

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