#pragma once

#include "common.h"

namespace postgres {

    using namespace detail;

    class ConnectionPool {
        using PoolType = ConnectionPool;
        using ConnectionPtr = std::shared_ptr<pqxx::connection>;

    public:
        class ConnectionWrapper {
        public:
            ConnectionWrapper(std::shared_ptr<pqxx::connection>&& conn, PoolType& pool) noexcept
                : conn_{ std::move(conn) }
                , pool_{ &pool } {
            }

            ConnectionWrapper(const ConnectionWrapper&) = delete;
            ConnectionWrapper& operator=(const ConnectionWrapper&) = delete;

            ConnectionWrapper(ConnectionWrapper&&) = default;
            ConnectionWrapper& operator=(ConnectionWrapper&&) = default;

            pqxx::connection& operator*() const& noexcept {
                return *conn_;
            }
            pqxx::connection& operator*() const&& = delete;

            pqxx::connection* operator->() const& noexcept {
                return conn_.get();
            }

            ~ConnectionWrapper();

        private:
            std::shared_ptr<pqxx::connection> conn_;
            PoolType* pool_;
        };

        // ConnectionFactory is a functional object returning std::shared_ptr<pqxx::connection>
        template <typename ConnectionFactory>
        ConnectionPool(size_t capacity, ConnectionFactory&& connection_factory) {
            pool_.reserve(capacity);
            for (size_t i = 0; i < capacity; ++i) {
                pool_.emplace_back(connection_factory());
            }
        }

        ConnectionWrapper GetConnection();

    private:
        void ReturnConnection(ConnectionPtr&& conn);

        std::mutex mutex_;
        std::condition_variable cond_var_;
        std::vector<ConnectionPtr> pool_;
        size_t used_connections_ = 0;
    };

	class DataBaseHandler : public std::enable_shared_from_this<DataBaseHandler> {
    private:
        ConnectionConfig config_;
        using ConnectionFactory = std::function<std::shared_ptr<pqxx::connection>()>;
	public:
        DataBaseHandler() = delete;
        DataBaseHandler(const DataBaseHandler&) = delete;
        DataBaseHandler& operator=(const DataBaseHandler&) = delete;
        DataBaseHandler(DataBaseHandler&&) = default;
        DataBaseHandler& operator=(DataBaseHandler&&) = default;

        explicit DataBaseHandler(ConnectionConfig&& config)
            : config_(std::move(config)), pool_(config_.connection_count_
                , std::move(PrepareConnectionFactory())) {
            FirstDataBaseConnection();
        }

        // добавляет новый рекорд в базу
        void AddNewPlayerRecord(std::string_view name, unsigned score, int time_ms);
        // добавляет новый рекорд в базу
        void AddNewPlayerRecord(const std::string& name, unsigned score, int time_ms);
        // возвращает топ рекордов с отступом от наивысшего вниз по списку
        std::optional<std::vector<DBGameRecord>> GetGameRecords(ReqParam param);
        // возвращает топ рекордов с отступом от наивысшего вниз по списку
        std::optional<std::vector<DBGameRecord>> GetGameRecords(int limit = __RECORDS_LIMIT__, int offset = 0);
        

	private:
        ConnectionPool pool_;

        // конЬструирует лямбду с подготовленными запросами к базе
        ConnectionFactory PrepareConnectionFactory();
        // первичное соединение с базой, создание таблицы, если её нет
        void FirstDataBaseConnection();
	};

} // namespace postgres