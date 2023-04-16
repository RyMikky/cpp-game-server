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

	// �����-���������� ������� ������� ������
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
	//		// ���������� ��� ��������, ����������� ������ value
	//		std::string name = map->GetName();
	//		std::string id = *(map->GetId());
	//		return std::hash<std::string>(name) + std::hash<std::string>(id);
	//	}
	//};

	// ��������� �������� � ������������ �������� ������� ������
	using GameInstance = std::vector<std::shared_ptr<GameSession>>;
	// ��������� ��� �������� ��������� ������� ������ �� ������
	using GameMapInstance = std::unordered_map<model::Map*, GameInstance, MapPtrHasher>;
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
			game_ = json_loader::LoadGame(configuration);
		}

		// ������������ ���� ������ � ������� ������
		std::string enter_to_game_session(std::string_view name, std::string_view map);

	protected: // ��������� ���� �������� ������ friend class -� ��� �������� ������ ������ � ��������� ���������� �������
		/* 
			����������� �����, ������� ���������� �� ������� ������.
			������ ��� ��������� ����������� ������ ��� ���������� ������ ������.
		*/
		const Token* get_unique_token(std::shared_ptr<GameSession> session);
		bool reset_token(std::string_view token);

	private:
		model::Game game_;

		// �������� � ������ ����������� ��� ������ � ����������� ������
		// ��� ��� ���������� "�����" ���������� ������, �� �������� ��������, ����� � ���� ������������ ������� �������� ������
		// �� �������� ����������� ������, �������� ��������� ����� � ������������� ������ ��������� ����������� ������, ������
		// ��������� � ����������� ����������� ������ ����� ����������� � �������. ���� ��������� ����������� ��� ���� concurence
		boost::asio::io_context io_context_;
		boost::asio::strand<boost::asio::io_context::executor_type> strand_;

		GameMapInstance instance_;
		GameTokenList token_list_;

		const Token* get_unique_token_impl(std::shared_ptr<GameSession> session);
		bool reset_token_impl(std::string_view token);
	};

} //namespace game_handler