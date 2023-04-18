#pragma once

#include "domain.h"
//#include "model.h"
#include "json_loader.h"
#include "boost_json.h"

#include <vector>
#include <memory>
#include <mutex>
#include <filesystem>
#include <numeric>
#include <unordered_map>

namespace game_handler {

	namespace fs = std::filesystem;
	namespace json = boost::json;
	namespace beast = boost::beast;
	namespace http = beast::http;
	namespace net = boost::asio;

	class GameHandler; // forward-defenition

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

	using SessionPlayers = std::unordered_map<const Token*, Player, TokenPtrHasher>;

	// класс-обработчик текущей игровой сессии
	class GameSession : public std::enable_shared_from_this<GameSession> {
	public:
		GameSession(GameHandler& handler, const model::Map* map, size_t max_players) 
			: game_handler_(handler), session_game_map_(map), players_id_(max_players) {
		}

		// отвечает есть ли в сессии свободное местечко
		bool have_free_space();

		Player* add_new_player(std::string_view name);

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
		const model::Map* session_game_map_;
		std::vector<bool> players_id_;
		SessionPlayers session_players_;
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
		// в каждой из открытыхз сессий путём банального перебора, а так, GameHandler будет "из коробки" знать
		// кто есть в игре и в каком из игровых инстансов со временем поиска в диапазоне от O(N) до O(LogN) - unordered_map
	public:
		// отдаём создание игровой модели класус обработчику игры
		explicit GameHandler(const fs::path& configuration) : io_context_(), strand_(io_context_.get_executor()){
			game_simple_ = json_loader::LoadGame(configuration);
		}

		// Обеспечивает вход игрока в игровую сессию
		std::string enter_to_game_session(std::string_view name, std::string_view map);

		// Возвращает ответ на запрос о списке игроков в данной сессии
		http_handler::Response player_list_response(http_handler::StringRequest&& req);
		// Возвращает ответ на запрос по присоединению к игре
		http_handler::Response join_game_response(http_handler::StringRequest&& req);
		// Возвращает ответ на запрос по поиску конкретной карты
		http_handler::Response find_map_response(http_handler::StringRequest&& req, std::string_view find_request_line);
		// Возвращает ответ со списком загруженных карт
		http_handler::Response map_list_response(http_handler::StringRequest&& req);
		// Возвращает ответ, что запрос некорректный
		http_handler::Response bad_request_response(http_handler::StringRequest&& req, std::string_view code, std::string_view message);
		// Возвращает ответ, что запрос не прошёл валидацию
		http_handler::Response unauthorized_response(http_handler::StringRequest&& req, std::string_view code, std::string_view message);

	protected: // протектед блок доступен только friend class -у для обратной записи данных и получения уникальных токенов
		/* 
			Реверсивный метод, который вызывается из игровой сессии.
			Служит для получения уникального токена при добавлении нового игрока.
		*/
		const Token* get_unique_token(std::shared_ptr<GameSession> session);
		bool reset_token(std::string_view token);

	private:
		model::Game game_simple_;

		// контекст и стренд необхордимы для работы в асинхронном режиме
		// так как обработчик "выдаёт" уникальные токены, то возможна ситуация, когда с двух параллельных потоков прилетит запрос
		// на создание уникального токена, получаем состояние гонки и потенциальную ошибку генерации уникального токена, потому
		// генерация и определение уникального токена будет выполняться в странде. Выше сказанное справедливо для всех concurence
		// сюда же требуется добавить количество потоков для обработки GameHandler (сделать то же самое, что в мейне RunWorker)
		net::io_context io_context_;                                  // КОНТЕКСТ ПОКА ЧТО НИГДЕ НЕ АКТИВИРОВАН! ЭТО НА БУДУЩЕЕ
		net::strand<net::io_context::executor_type> strand_;
		// в зависимости от проектируемых нагрузок и оборудования для запуска системы, уже по месту надо определять использовать
		// ли контекст, и если контекст, то в сколько потоков, собственных или общих? или же использовать мьютекс.
		// на данном этапе контекст не даст прироста производительности потому пока опустим это и будем использовать мьютекс
		std::mutex mutex_;

		GameMapInstance instances_;
		GameTokenList tokens_list_;

		const Token* get_unique_token_impl(std::shared_ptr<GameSession> session);
		bool reset_token_impl(std::string_view token);

		// Возвращает ответ на запрос о списке игроков в данной сессии
		http_handler::Response player_list_response_impl(http_handler::StringRequest&& req, Token&& token);
		// Возвращает ответ, о успешном добавлении игрока в игровую сессию
		http_handler::Response join_game_response_impl(http_handler::StringRequest&& req, json::value&& body, const model::Map* map);
		// Возвращает ответ, что упомянутая карта не найдена
		http_handler::Response map_not_found_response_impl(http_handler::StringRequest&& req);
		// Возвращает ответ, что запрошенный метод не разрешен, доступные указывается в аргументе allow
		http_handler::Response method_not_allowed_impl(http_handler::StringRequest&& req, std::string_view allow);
	
		template <typename ...Methods>
		// Возвращает ответ, что запрошенные методы не разрешены, доступный указывается в аргументе allow
		http_handler::Response method_not_allowed_impl(http_handler::StringRequest&& req, Methods&& ...methods);
	};

	namespace detail {

		template<typename T1, typename T2>
		std::string combine_methods(const T1& method_one, const T2& method_two) {
			return std::string(method_one) + ", " + std::string(method_two);
		}

		template<typename T1, typename T2, typename... Args>
		std::string combine_methods(const T1& method_one, const T2& method_two, Args&&... args) {
			return std::string(method_one) + ", " + std::string(method_two) + ", " + combine_words(std::forward<Args>(args)...);
		}

		template<typename... Args>
		std::string combine_methods(Args&&... args) {
			std::string result;
			bool first = true;
			((result += (first ? "" : ", ") + std::string(std::forward<Args>(args)), first = false), ...);
			return result;
		}

	} // namespace detail

	template <typename ...Methods>
	// Возвращает ответ, что запрошенные методы не разрешены, доступный указывается в аргументе allow
	http_handler::Response GameHandler::method_not_allowed_impl(http_handler::StringRequest&& req, Methods&& ...methods) {
		http_handler::StringResponse response(http::status::method_not_allowed, req.version());
		response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
		response.set(http::field::cache_control, "no-cache");
		// собираюю строку сборщиком из detail
		response.set(http::field::allow, detail::combine_methods(methods...));
		response.body() = json_detail::GetErrorString("invalidMethod"sv, "Invalid method"s);

		return response;
	}

} // namespace game_handler