#pragma once

#include <pqxx/pqxx>
#include <pqxx/zview.hxx>
#include <pqxx/connection>
#include <pqxx/transaction>

#include "tagged_uuid.h"

#include <map>
#include <mutex>
#include <vector>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <functional>
#include <condition_variable>

namespace postgres {

	using pqxx::operator"" _zv;

	constexpr int __RECORDS_LIMIT__ = 100;

	constexpr pqxx::zview __CREATE_GAME_TABLE__ = "create_game_records_table"_zv;
	constexpr pqxx::zview __CREATE_IDX_MULTI__ = "create_idx_multi"_zv;
	constexpr pqxx::zview __UPDATE_IDX_MULTI__ = "update_idx_multi"_zv;
	constexpr pqxx::zview __ADD_NEW_USER_RECORD__ = "add_new_user_record"_zv;
	constexpr pqxx::zview __GET_N_TOP_SCORES__ = "get_n_top_scores"_zv;
	constexpr pqxx::zview __GET_N_TOP_SCORES_FROM_OFFSET__ = "get_n_top_scores_from_offset"_zv;

	static const std::map<pqxx::zview, pqxx::zview> __PREPARED_DATABASE_FIRST_COMMANDS__ = {
		// создание базовой таблицы игрового сервера
		{"create_game_records_table"_zv, R"(CREATE TABLE IF NOT EXISTS retired_players 
				(id UUID CONSTRAINT id_constraint PRIMARY KEY, name varchar(100), score integer, play_time_ms integer);)"_zv},

		// создание мульти индекса для быстрого поиска
		{"create_idx_multi"_zv,
			R"(CREATE INDEX IF NOT EXISTS idx_multi ON retired_players (score DESC, play_time_ms, name);)"_zv}
	};

	static const std::map<pqxx::zview, pqxx::zview> __PREPARED_DATABASE_SECOND_COMMANDS__ = {

		// обновление мульти индекса для быстрого поиска
		{"update_idx_multi"_zv,
			R"(REINDEX INDEX idx_multi;;)"_zv},

		// добавить новый рекорд в таблицу
		{"add_new_user_record"_zv,
			R"(INSERT INTO retired_players (id, name, score, play_time_ms) VALUES ($1, $2, $3, $4);)"_zv},

		// выводит топ игроков в указанном количестве
		{"get_n_top_scores"_zv,
			R"(SELECT id::text, name::text, score, play_time_ms FROM retired_players ORDER BY score DESC, play_time_ms DESC, name ASC LIMIT $1)"_zv},

		// выводит топ игроков в указанном количестве, начиная с указанной позиции
		{"get_n_top_scores_from_offset"_zv,
			R"(SELECT id::text, name::text, score, play_time_ms FROM retired_players ORDER BY score DESC, play_time_ms DESC, name ASC LIMIT $1 OFFSET $2)"_zv}

	};

	namespace detail {

		struct ReqParam {
			std::optional<int> limit_;
			std::optional<int> offset_;
		};

		struct RecordTag {};
		using RecordId = util::TaggedUUID<RecordTag>;

		struct DBGameRecord {
			RecordId id_;
			std::string name_;
			unsigned score_;
			int time_ms_;
		};

		// базовая обёртка транзакции
		class Transaction {
		public:
			Transaction(pqxx::connection& connection)
				: transaction_{ connection } {
			}

			~Transaction() {
				transaction_.commit();
			}

			pqxx::work& GetTransaction() {
				return transaction_;
			}

		private:
			pqxx::work transaction_;
		};

		struct ConnectionConfig {
			std::string db_url_;
			size_t connection_count_;
		};

	} // namespace detail

} // namespace postgres
