#include "postgers.h"

namespace postgres {

	ConnectionPool::ConnectionWrapper::~ConnectionWrapper() {
		if (conn_) {
			pool_->ReturnConnection(std::move(conn_));
		}
	}

	ConnectionPool::ConnectionWrapper ConnectionPool::GetConnection() {
		std::unique_lock lock{ mutex_ };
		// Блокируем текущий поток и ждём, пока cond_var_ не получит уведомление и не освободится
		// хотя бы одно соединение
		cond_var_.wait(lock, [this] {
			return used_connections_ < pool_.size();
			});
		// После выхода из цикла ожидания мьютекс остаётся захваченным

		return { std::move(pool_[used_connections_++]), *this };
	}

	void ConnectionPool::ReturnConnection(ConnectionPtr&& conn) {
		// Возвращаем соединение обратно в пул
		{
			std::lock_guard lock{ mutex_ };
			assert(used_connections_ != 0);
			pool_[--used_connections_] = std::move(conn);
		}
		// Уведомляем один из ожидающих потоков об изменении состояния пула
		cond_var_.notify_one();
	}

	// добавляет новый рекорд в базу
	void DataBaseHandler::AddNewPlayerRecord(std::string_view name, unsigned score, int time_ms) {
		AddNewPlayerRecord(std::string(name), score, time_ms);
	}

	// добавляет новый рекорд в базу
	void DataBaseHandler::AddNewPlayerRecord(const std::string& name, unsigned score, int time_ms) {
		try
		{
			// берем свободное соединение
			auto conn = pool_.GetConnection();
			// открываем транзакцию
			Transaction work(*conn);
			// добавляем новый рекорд
			work.GetTransaction().exec_prepared(__ADD_NEW_USER_RECORD__
				, RecordId{}.New().ToString()
				, name
				, static_cast<int>(score)
				, time_ms);

			// пересчитываем индексы
			work.GetTransaction().exec_prepared(__UPDATE_IDX_MULTI__);

			// при завершении работы метода
			// деструктор транзакции произведет коммит
			// соединение вернется в пул доступных подключений
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error("DataBaseHandler::AddNewPlayerRecord::ERROR::" + std::string(e.what()));
		}
	}

	// возвращает топ рекордов с отступом от наивысшего вниз по списку
	std::optional<std::vector<DBGameRecord>> DataBaseHandler::GetGameRecords(ReqParam param) {
		return GetGameRecords(param.limit_.value_or(__RECORDS_LIMIT__), param.offset_.value_or(0));
	}

	// возвращает топ рекордов с отступом от наивысшего вниз по списку
	std::optional<std::vector<DBGameRecord>> DataBaseHandler::GetGameRecords(int limit, int offset) {
		try
		{
			if (limit > __RECORDS_LIMIT__) {
				return std::nullopt; // если установлено очень больше количество элементов, то возвращаем нуль
			}

			// берем свободное соединение
			auto conn = pool_.GetConnection();
			// открываем транзакцию
			Transaction work(*conn);
			// создаём таблицу если её нет

			std::vector<DBGameRecord> result;
			pqxx::result t_result = work.GetTransaction().exec_prepared(__GET_N_TOP_SCORES_FROM_OFFSET__, limit, offset);
			for (const auto& row : t_result) {

				std::string row_id = row["id"].as<std::string>();
				std::string row_name = row["name"].as<std::string>();
				int row_score = row["score"].as<int>();
				int row_time = row["play_time_ms"].as<int>();

				result.push_back(DBGameRecord{ RecordId{}.FromString(row_id)
					, row_name
					, static_cast<unsigned>(row_score)
					, row_time });
			}

			return result;

			// при завершении работы метода
			// деструктор транзакции произведет коммит
			// соединение вернется в пул доступных подключений
		}
		catch (const std::exception& e)
		{
			std::cerr << e.what() << std::endl;
			throw std::runtime_error("DataBaseHandler::GetGameRecords::ERROR::" + std::string(e.what()));
		}
	}

	// конЬструирует лямбду с подготовленными запросами к базе
	DataBaseHandler::ConnectionFactory DataBaseHandler::PrepareConnectionFactory() {
		auto factory = [this]() -> std::shared_ptr<pqxx::connection> {
			auto conn = std::make_shared<pqxx::connection>(this->config_.db_url_);
			for (const auto& [tag, command] : __PREPARED_DATABASE_FIRST_COMMANDS__) {
				conn->prepare(tag, command);
			}

			// после активации первых
			pqxx::work work{ *conn };
			work.exec_prepared(__CREATE_GAME_TABLE__);
			work.exec_prepared(__CREATE_IDX_MULTI__);
			work.commit();

			for (const auto& [tag, command] : __PREPARED_DATABASE_SECOND_COMMANDS__) {
				conn->prepare(tag, command);
			}
			return conn; };

		return std::move(factory);
	}

	// первичное соединение с базой, создание таблицы, если её нет
	void DataBaseHandler::FirstDataBaseConnection() {
		try
		{
			// берем свободное соединение
			auto conn = pool_.GetConnection();
			// открываем транзакцию
			Transaction work(*conn);
			// создаём таблицу если её нет
			work.GetTransaction().exec_prepared(__CREATE_GAME_TABLE__);
			// конфигурируем индексы
			work.GetTransaction().exec_prepared(__CREATE_IDX_MULTI__);

			// при завершении работы метода
			// деструктор транзакции произведет коммит
			// соединение вернется в пул доступных подключений
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error("DataBaseHandler::FirstDataBaseConnection::ERROR::" + std::string(e.what()));
		}
	}

} // namespace postgres