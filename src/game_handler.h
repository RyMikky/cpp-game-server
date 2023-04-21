#pragma once

#include "domain.h"
#include "json_loader.h"
#include "boost_json.h"

#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <filesystem>
#include <numeric>
#include <optional>
#include <variant>
#include <execution>
#include <unordered_map>

namespace game_handler {

	namespace fs = std::filesystem;
	namespace json = boost::json;
	namespace beast = boost::beast;
	namespace http = beast::http;
	namespace net = boost::asio;

	class GameHandler; // forward-definition

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

		// чекает стартовую позицию на предмет совпадения с другими игроками в сессии
		const SessionPlayers& get_session_players() const {
			return session_players_;
		}

	private:
		GameHandler& game_handler_;
		const model::Map* session_game_map_;
		std::vector<bool> players_id_;
		SessionPlayers session_players_;

		bool start_position_check_impl(PlayerPosition& position);
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
	// вариант для проверки авторизации в одном методе
	using Authorization = std::variant<std::monostate, Token, http_handler::Response>;

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
			: game_simple_(json_loader::LoadGame(configuration)) {
		}

		// Возвращает ответ на запрос о состоянии игроков в игровой сессии
		http_handler::Response game_state_response(http_handler::StringRequest&& req);
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
		//boost::future<const Token*> get_unique_token(std::shared_ptr<GameSession>  session);
		bool reset_token(std::string_view token);

	private:
		model::Game game_simple_;
		std::mutex mutex_;

		GameMapInstance instances_;
		GameTokenList tokens_list_;

		const Token* get_unique_token_impl(std::shared_ptr<GameSession> session);
		bool reset_token_impl(std::string_view token);

		// Возвращает ответ на запрос о состоянии игроков в игровой сессии
		http_handler::Response game_state_response_impl(http_handler::StringRequest&& req, Token&& token);
		// Возвращает ответ на запрос о списке игроков в данной сессии
		http_handler::Response player_list_response_impl(http_handler::StringRequest&& req, Token&& token);
		// Возвращает ответ, о успешном добавлении игрока в игровую сессию
		http_handler::Response join_game_response_impl(http_handler::StringRequest&& req, json::value&& body, const model::Map* map);
		// Возвращает ответ, что упомянутая карта не найдена
		http_handler::Response map_not_found_response_impl(http_handler::StringRequest&& req);
		// Возвращает ответ, что запрошенный метод не разрешен, доступные указывается в аргументе allow
		http_handler::Response method_not_allowed_impl(http_handler::StringRequest&& req, std::string_view allow);

		// метод проверяющий совпадение токена с предоставленным, если токен корректнен и есть в базе, то возвращается токен
		Authorization authorization_token_impl_old(http_handler::StringRequest& req);

		// метод проверяющий совпадение токена с предоставленным, если токен корректнен и есть в базе, то возвращается токен
		template <typename Function>
		http_handler::Response authorization_token_impl(http_handler::StringRequest&& req, Function&& func);
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

		std::optional<std::string> BearerParser(std::string&& auth_line);

	} // namespace detail

	template <typename Function>
	http_handler::Response GameHandler::authorization_token_impl(http_handler::StringRequest&& req, Function&& func) {
		// ищем тушку авторизации среди хеддеров запроса
		auto auth_iter = req.find("Authorization");
		if (auth_iter == req.end()) {
			// если нет тушки по авторизации, тогда кидаем отбойник
			return unauthorized_response(std::move(req),
				"invalidToken"sv, "Authorization header is missing"sv);
		}

		// из тушки запроса получаем строку
		// так как строка должна иметь строгий вид Bearer <токен>, то мы легко можем распарсить её
		auto auth_reparse = detail::BearerParser({ auth_iter->value().begin(), auth_iter->value().end() });

		if (!auth_reparse) {
			// если нет строки Bearer, или она корявая, или токен пустой, то кидаем отбойник
			return unauthorized_response(std::move(req),
				"invalidToken"sv, "Authorization header is missing"sv);
		}

		Token token{ auth_reparse.value() }; // создаём быстро токен на основе запроса и ищем совпадение во внутреннем массиве
		if (!tokens_list_.count(token)) {
			// если заголовок Authorization содержит валидное значение токена, но в игре нет пользователя с таким токеном
			return unauthorized_response(std::move(req),
				"unknownToken"sv, "Player token has not been found"sv);
		}

		// вызываем полченный обработчик
		return func(std::move(req), std::move(token));
	}

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