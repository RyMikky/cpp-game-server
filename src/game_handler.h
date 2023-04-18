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

	// �����-���������� ������� ������� ������
	class GameSession : public std::enable_shared_from_this<GameSession> {
	public:
		GameSession(GameHandler& handler, const model::Map* map, size_t max_players) 
			: game_handler_(handler), session_game_map_(map), players_id_(max_players) {
		}

		// �������� ���� �� � ������ ��������� ��������
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

	// ��������� �������� � ������������ �������� ������� ������
	using GameInstance = std::vector<std::shared_ptr<GameSession>>;
	// ��������� ��� �������� ��������� ������� ������ �� ������
	using GameMapInstance = std::unordered_map<const model::Map*, GameInstance, MapPtrHasher>;
	// ��������� ��� �������� ������ ������ �� ������, ����������� ����������� ����������� �� ������ � ����������
	using GameTokenList = std::unordered_map<Token, std::shared_ptr<GameSession>, TokenHasher>;

	class GameHandler {
		friend class GameSession;
		// ��� ������ ������� ������, ��� �������� � �������� �������
		// ����� "���������" ����������� ��� �������� ������ ������� � ������ �� ���������� ������
		// ������������, � ���� ������ ����� ���, �� �����, ����� ����������� ������ �� ������ ����� ���� ���������� 
		// � ������ �� ��������� ������ ���� ���������� ��������, � ���, GameHandler ����� "�� �������" �����
		// ��� ���� � ���� � � ����� �� ������� ��������� �� �������� ������ � ��������� �� O(N) �� O(LogN) - unordered_map
	public:
		// ����� �������� ������� ������ ������ ����������� ����
		explicit GameHandler(const fs::path& configuration) : io_context_(), strand_(io_context_.get_executor()){
			game_simple_ = json_loader::LoadGame(configuration);
		}

		// ������������ ���� ������ � ������� ������
		std::string enter_to_game_session(std::string_view name, std::string_view map);

		// ���������� ����� �� ������ � ������ ������� � ������ ������
		http_handler::Response player_list_response(http_handler::StringRequest&& req);
		// ���������� ����� �� ������ �� ������������� � ����
		http_handler::Response join_game_response(http_handler::StringRequest&& req);
		// ���������� ����� �� ������ �� ������ ���������� �����
		http_handler::Response find_map_response(http_handler::StringRequest&& req, std::string_view find_request_line);
		// ���������� ����� �� ������� ����������� ����
		http_handler::Response map_list_response(http_handler::StringRequest&& req);
		// ���������� �����, ��� ������ ������������
		http_handler::Response bad_request_response(http_handler::StringRequest&& req, std::string_view code, std::string_view message);
		// ���������� �����, ��� ������ �� ������ ���������
		http_handler::Response unauthorized_response(http_handler::StringRequest&& req, std::string_view code, std::string_view message);

	protected: // ��������� ���� �������� ������ friend class -� ��� �������� ������ ������ � ��������� ���������� �������
		/* 
			����������� �����, ������� ���������� �� ������� ������.
			������ ��� ��������� ����������� ������ ��� ���������� ������ ������.
		*/
		const Token* get_unique_token(std::shared_ptr<GameSession> session);
		bool reset_token(std::string_view token);

	private:
		model::Game game_simple_;

		// �������� � ������ ����������� ��� ������ � ����������� ������
		// ��� ��� ���������� "�����" ���������� ������, �� �������� ��������, ����� � ���� ������������ ������� �������� ������
		// �� �������� ����������� ������, �������� ��������� ����� � ������������� ������ ��������� ����������� ������, ������
		// ��������� � ����������� ����������� ������ ����� ����������� � �������. ���� ��������� ����������� ��� ���� concurence
		// ���� �� ��������� �������� ���������� ������� ��� ��������� GameHandler (������� �� �� �����, ��� � ����� RunWorker)
		net::io_context io_context_;                                  // �������� ���� ��� ����� �� �����������! ��� �� �������
		net::strand<net::io_context::executor_type> strand_;
		// � ����������� �� ������������� �������� � ������������ ��� ������� �������, ��� �� ����� ���� ���������� ������������
		// �� ��������, � ���� ��������, �� � ������� �������, ����������� ��� �����? ��� �� ������������ �������.
		// �� ������ ����� �������� �� ���� �������� ������������������ ������ ���� ������� ��� � ����� ������������ �������
		std::mutex mutex_;

		GameMapInstance instances_;
		GameTokenList tokens_list_;

		const Token* get_unique_token_impl(std::shared_ptr<GameSession> session);
		bool reset_token_impl(std::string_view token);

		// ���������� ����� �� ������ � ������ ������� � ������ ������
		http_handler::Response player_list_response_impl(http_handler::StringRequest&& req, Token&& token);
		// ���������� �����, � �������� ���������� ������ � ������� ������
		http_handler::Response join_game_response_impl(http_handler::StringRequest&& req, json::value&& body, const model::Map* map);
		// ���������� �����, ��� ���������� ����� �� �������
		http_handler::Response map_not_found_response_impl(http_handler::StringRequest&& req);
		// ���������� �����, ��� ����������� ����� �� ��������, ��������� ����������� � ��������� allow
		http_handler::Response method_not_allowed_impl(http_handler::StringRequest&& req, std::string_view allow);
	
		template <typename ...Methods>
		// ���������� �����, ��� ����������� ������ �� ���������, ��������� ����������� � ��������� allow
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
	// ���������� �����, ��� ����������� ������ �� ���������, ��������� ����������� � ��������� allow
	http_handler::Response GameHandler::method_not_allowed_impl(http_handler::StringRequest&& req, Methods&& ...methods) {
		http_handler::StringResponse response(http::status::method_not_allowed, req.version());
		response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
		response.set(http::field::cache_control, "no-cache");
		// �������� ������ ��������� �� detail
		response.set(http::field::allow, detail::combine_methods(methods...));
		response.body() = json_detail::GetErrorString("invalidMethod"sv, "Invalid method"s);

		return response;
	}

} // namespace game_handler