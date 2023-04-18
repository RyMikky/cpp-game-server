#include "game_handler.h"
#include <boost/asio.hpp>
#include <algorithm>

namespace game_handler {

	namespace fs = std::filesystem;
	namespace json = boost::json;

	size_t TokenHasher::operator()(const Token& token) const noexcept {
		// ���������� ��� ��������, ����������� ������ token
		return _hasher(*token);
	}

	size_t TokenPtrHasher::operator()(const Token* token) const noexcept {
		// ���������� ��� ��������, ����������� ������ token
		return _hasher(*(*token));
	}

	// -------------------------- class GameSession --------------------------

	// �������� ���� �� � ������ ��������� ��������
	bool GameSession::have_free_space() {
		// ������� ���� �� ����� � ������� ������� ������
		auto id = std::find(players_id_.begin(), players_id_.end(), false);

		return id != players_id_.end();
	}

	Player* GameSession::add_new_player(std::string_view name) {
		// ������� ���� �� ����� � ������� ������� ������
		auto id = std::find(players_id_.begin(), players_id_.end(), false);
		if (id != players_id_.end()) {
			// ���� ����� ����, �� ����������� ���������� �����
			const Token* player_token = game_handler_.get_unique_token(shared_from_this());
			// ������ ������ � ������� ������� ������
			session_players_[player_token] = 
				std::move(Player{ uint16_t(std::distance(players_id_.begin(), id)), name, player_token });
			// ������ ������� � ������� �������
			*id = true;

			// ���������� ������ �� ������
			return get_player_by_token(player_token);
		}
		else {
			return nullptr;
		}
	}

	bool GameSession::remove_player(const Token* token) {
		if (!session_players_.count(token)) {
			return false;
		}
		else {
			// ����������� id �������� ������ �� ������
			players_id_[session_players_.at(token).get_player_id()] = false;
			// ������� ������ � ������ ������ �� ����������
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
		// ���������� ��� ��������, ����������� ������ map
		return _hasher(map->GetName()) + _hasher(*(map->GetId()));
	}

	// ������������ ���� ������ � ������� ������
	std::string enter_to_game_session(std::string_view name, std::string_view map) {
		return {};
	}

	// ���������� ����� �� ������ � ������ ������� � ������ ������
	http_handler::Response GameHandler::player_list_response(http_handler::StringRequest&& req) {

		if (req.method_string() != http_handler::Method::GET && req.method_string() != http_handler::Method::HEAD) {
			// ���� � ��� �� ��� � �� ��� ������, �� ������ ��������
			return method_not_allowed_impl(std::move(req), http_handler::Method::GET, http_handler::Method::HEAD);
		}

		try
		{
			// ���� ����� ����������� ����� �������� �������
			auto auth_iter = req.find("Authorization");
			if (auth_iter == req.end()) {
				// ���� ��� ����� �� �����������, ����� ������ ��������
				return unauthorized_response(std::move(req),
					"invalidToken"sv, "Authorization header is missing"sv);
			}

			// �� ����� ������� �������� ������
			// ��� ��� ������ ������ ����� ������� ��� Bearer <�����>, �� �� ����� ����� ���������� �
			std::string bearer{ auth_iter->value().begin(), auth_iter->value().begin() + 6 };
			std::string token{ auth_iter->value().begin() + 7, auth_iter->value().end() };

			if (bearer != "Bearer" || token.size() == 0) {
				// ���� ��� ������ Bearer, ��� ��� �������, ��� ����� ������, �� ������ ��������
				return unauthorized_response(std::move(req),
					"invalidToken"sv, "Authorization header is missing"sv);
			}

			Token req_token{ token }; // ������ ������ ����� �� ������ ������� � ���� ���������� �� ���������� �������
			if (!tokens_list_.count(req_token)) {
				// ���� ��������� Authorization �������� �������� �������� ������, �� � ���� ��� ������������ � ����� �������
				return unauthorized_response(std::move(req),
					"unknownToken"sv, "Player token has not been found"sv);
			}
			else {
				// ���� ����� ���� ����, ����� ������������ � ���������������� ������������� ������
				// ���� ����� ���� ����, ����� ��� ����� ������, ��� �� "�����" � ����������� ���� � ���� ������������ �������
				return player_list_response_impl(std::move(req), std::move(req_token));
			}
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error("GameHandler::player_list_response::error" + std::string(e.what()));
		}
	}
	// ���������� ����� �� ������ �� ������������� � ����
	http_handler::Response GameHandler::join_game_response(http_handler::StringRequest&& req) {

		if (req.method_string() != http_handler::Method::POST) {
			// ���� �������� ������� � ������������ ����
			return method_not_allowed_impl(std::move(req), http_handler::Method::POST);
		}

		try
		{
			// ���� ����� ����� �������� �������
			auto body_iter = req.find("Body");
			if (body_iter == req.end()) {
				// ���� ��� ���� �������, ����� �����������
				return bad_request_response(std::move(req), 
					"invalidArgument"sv, "Header body whit two arguments <userName> and <mapId> expected"sv);
			}
			// �� ����� ������� �������� ������
			std::string body_string{ body_iter->value().begin(), body_iter->value().end() };
			// ������ ���� �������, ��� ���������� � �������� ����� ������ � catch_�����
			json::value req_data = json_detail::ParseTextToBoostJson(body_string);

			// ���� � ����� ������ ��� ����� "userName" ��� "mapId"
			if (!req_data.as_object().count("userName") || !req_data.as_object().count("mapId")) {
				return bad_request_response(std::move(req), "invalidArgument"sv, "Two arguments <userName> and <mapId> expected"sv);
			}

			// ���� � "userName" �������
			if (req_data.as_object().at("userName") == "") {
				return bad_request_response(std::move(req), "invalidArgument"sv, "Invalid name"sv);
			}

			// ���� ����������� �����
			auto map = game_simple_.FindMap(
				model::Map::Id{ std::string(
					req_data.as_object().at("mapId").as_string()) });

			if (map == nullptr) {
				// ���� ����� �� �������, �� ������ ��������
				return map_not_found_response_impl(std::move(req));
			}
			else {
				// ���� ����� ���� � �� �������� ���������
				// �������� ���������� �������� �������������
				return join_game_response_impl(std::move(req), std::move(req_data), map);
			}
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error("GameHandler::join_game_response::error" + std::string(e.what()));
		}
	}
	// ���������� ����� �� ������ �� ������ ���������� �����
	http_handler::Response GameHandler::find_map_response(http_handler::StringRequest&& req, std::string_view find_request_line) {

		// ���� ����������� �����
		auto map = game_simple_.FindMap(model::Map::Id{ std::string(find_request_line) });

		if (map == nullptr) {
			// ���� ����� �� �������, �� ������ ��������
			return map_not_found_response_impl(std::move(req));
		}
		else {

			http_handler::StringResponse response(http::status::ok, req.version());
			response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
			response.set(http::field::cache_control, "no-cache");

			// ��������� ���� ������ �� �������������� ����� �� ����������� ���� ����� ���������� �����
			response.body() = json_detail::GetMapInfo(map);

			return response;
		}

	}
	// ���������� ����� �� ������� ����������� ����
	http_handler::Response GameHandler::map_list_response(http_handler::StringRequest&& req) {
		http_handler::StringResponse response(http::status::ok, req.version());
		response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
		response.set(http::field::cache_control, "no-cache");
		response.body() = json_detail::GetMapList(game_simple_.GetMaps());

		return response;
	}
	// ���������� �����, ��� ������ ������������
	http_handler::Response GameHandler::bad_request_response(
		http_handler::StringRequest&& req, std::string_view code, std::string_view message) {
		http_handler::StringResponse response(http::status::bad_request, req.version());
		response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
		response.set(http::field::cache_control, "no-cache");
		response.body() = json_detail::GetErrorString(code, message);

		return response;
	}
	// ���������� �����, ��� ������ �� ������ ���������
	http_handler::Response GameHandler::unauthorized_response(
		http_handler::StringRequest&& req, std::string_view code, std::string_view message) {
		http_handler::StringResponse response(http::status::unauthorized, req.version());
		response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
		response.set(http::field::cache_control, "no-cache");
		response.body() = json_detail::GetErrorString(code, message);

		return response;
	}

	const Token* GameHandler::get_unique_token(std::shared_ptr<GameSession> session) {

		// ������� ��������� � ������� ���������

		//const Token* result = nullptr;
		//boost::asio::post(strand_, 
		//	// �������� ����� � ������� � ������� ������������� � ��������� ������� ������
		//	[this, session, &result]() { result = this->get_unique_token_impl(session); });

		//return result;

		// ������� ��������� � ������� ��������

		std::lock_guard func_lock_(mutex_);
		return get_unique_token_impl(session);
	}

	const Token* GameHandler::get_unique_token_impl(std::shared_ptr<GameSession> session) {

		bool isUnique = true;         // ������ ����������� ����
		Token unique_token{ "" };       // ������ �����-��������

		while (isUnique)
		{
			// ���������� ����� �����
			unique_token = Token{ detail::GenerateToken32Hex() };
			// ���� ��������������� ����� ��� ����, �� ���� ��� � ��������� �������� � ���� ����������
			isUnique = tokens_list_.count(unique_token);
		}

		auto insert = tokens_list_.insert({ unique_token,  session });
		return &(insert.first->first);
	}

	bool GameHandler::reset_token(std::string_view token) {

		bool result = false;
		boost::asio::post(strand_,
			// ������� � ������� ��������� �������������
			[this, token, &result]() { result = reset_token_impl(token); });
		return result;
	}

	bool GameHandler::reset_token_impl(std::string_view token) {
		Token remove{ std::string(token) };

		// TODO ��������, ������ ����� ��������� �������� �� �����, ���� ���������� + �������� � GameSession

		if (tokens_list_.count(remove)) {
			tokens_list_.at(remove)->remove_player(&remove);
			return tokens_list_.erase(remove);
		}
		else {
			return false;
		}
	}


	// ���������� ����� �� ������ � ������ ������� � ������ ������
	http_handler::Response GameHandler::player_list_response_impl(http_handler::StringRequest&& req, Token&& token) {

		// �������� ������ ��� �� ������ ������ "�����" ��������� �����
		std::shared_ptr<GameSession> session = tokens_list_.at(token);

		// �������������� � ���������� �����
		http_handler::StringResponse response(http::status::ok, req.version());
		response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
		response.set(http::field::cache_control, "no-cache");
		// ��������� ����� ������ � ������� ���������� ������
		response.body() = json_detail::GetSessionPlayersList(session->cbegin(), session->cend());

		return response;
	}
	// ���������� �����, � �������� ���������� ������ � ������� ������
	http_handler::Response GameHandler::join_game_response_impl(http_handler::StringRequest&& req, json::value&& body, const model::Map* map) {
		
		Player* new_player = nullptr;        // ��������� ��� ������ ������ �� �������
		std::shared_ptr<GameSession> ref;    // ��������� ��� ��������� �� ���������� ������� ������

		// ������� ���� �� �� ������ ������ �������� ������� ������� �� ������ �����
		if (instances_.count(map)) {
			// ���� ������� ����, �������� ��� ������
			auto instance = instances_.at(map);
			// �������� ���������� ��� ���������� ������ � �������� �� ������� ������� ���������� �����
			// ��� ��� ������� �� �������� ��� �����, �� ���� �� ���� ������� ������ ���� ������

			bool have_a_plance = false;      // ������ ���������� ��� ������������ ������� ���� � ������� �������
			for (auto& item : instance) {
				// ���� ����� ��������� �����
				if (item->have_free_space()) {

					ref = item;              // ���������� ������ �� ���� ������
					have_a_plance = true;    // ������ ����, ��� ����� �����
					break;                   // ��������� ���� �� �������������
				}
			}

			// ���� ����� ���� �������, �� ������ ������ �� ���� - ref � ��� ����, ���� �� ���� ����� �������� ����� � ������ �����
			if (!have_a_plance) {
				// ���� �� ���� � ������� �������� ������� �� �������, �� ��� ����, ������ ���� ������� �����
				ref = instances_.at(map)
					.emplace_back(std::make_shared<GameSession>(*this, map, 8));
			}
		}

		// ���� ��� �� ����� �������� ������ �� ������ �����
		else {
			// ��������� ��������� � ������ ������� �� ������� ������
			instances_.insert(std::make_pair(map, GameInstance()));
			// ��� ��� � ��� ��� ��� ������� ������ �� ���� ������ ����� ������ 
			// c ����������, ��� �������, � 8 ������� (��. ����������� GameSession)
			// ��� �� �������� ���� �� ��, � �������� �� ���������� �� ���������� � ����
			ref = instances_.at(map)
				.emplace_back(std::make_shared<GameSession>(*this, map, 8));
		}

		// ��������� ������ �� ������ � ��������� �� ���� ���������
		new_player = ref->add_new_player(body.at("userName").as_string());

		// �������������� � ���������� �����
		http_handler::StringResponse response(http::status::ok, req.version());
		response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
		response.set(http::field::cache_control, "no-cache");
		response.body() = json_detail::GetJoinPlayerString(new_player);

		return response;
	}
	// ���������� �����, ��� ���������� ����� �� �������
	http_handler::Response GameHandler::map_not_found_response_impl(http_handler::StringRequest&& req) {
		http_handler::StringResponse response(http::status::not_found, req.version());
		response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
		response.set(http::field::cache_control, "no-cache");
		response.body() = json_detail::GetErrorString("mapNotFound"sv, "Map not found"sv);

		return response;
	}
	// ���������� �����, ��� ����������� ����� �� ��������, ��������� ����������� � ��������� allow
	http_handler::Response GameHandler::method_not_allowed_impl(http_handler::StringRequest&& req, std::string_view allow) {
		http_handler::StringResponse response(http::status::method_not_allowed, req.version());
		response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
		response.set(http::field::cache_control, "no-cache");
		response.set(http::field::allow, allow);
		response.body() = json_detail::GetErrorString("invalidMethod"sv, ("Only "s + std::string(allow) + " method is expected"s));

		return response;
	}
	
} //namespace game_handler