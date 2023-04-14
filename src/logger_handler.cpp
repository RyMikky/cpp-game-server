#include "logger_handler.h"

namespace logger_handler {

    namespace detail {

        void BaseFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {

            // згружаем их в общий жидомасон ответа
            json::object out_json {
                 {"timestamp", to_iso_extended_string(*rec[timestamp])},
                 {"data", *rec[additional_data]},
                 {"message", *rec[additional_message]}
            };

            // отдаём результирующий жидомасон
            strm << out_json;
        }

    } // namespace detail

    void LoggerHandler::LogShutdown() {
        BoostLogConsoleOutput(
            json::value{
                {"code"s, 0}
            }, "server exited"s);
    }

    void LoggerHandler::LogException(const std::exception& exc) {
        BoostLogConsoleOutput(
            json::value{
                {"code"s, EXIT_FAILURE},
                {"exception"s, exc.what()}
            }, "server exited"s);
    }

    void LoggerHandler::LogError(beast::error_code ec, std::string_view location) {

        BoostLogConsoleOutput(
            json::value{
                {"code"s, ec.value()},
                {"text"s, ec.message()},
                {"where"s, std::string(location)}
            }, "error"s);
    }

    void LoggerHandler::LogRequest(const http_handler::StringRequest& req, std::string_view remote_address) {

        BoostLogConsoleOutput(
            json::value{
                {"ip"s, std::string(remote_address)},
                {"URI"s, req.target()},
                {"method"s, req.method_string()}
            }, "request received"s);
    }

    void LoggerHandler::LogResponse (int code, std::string_view content, uint64_t time, std::string_view remote_address) {

        BoostLogConsoleOutput(
            json::value{
                {"ip"s, std::string(remote_address)},
                {"response_time"s, time},
                {"code"s, code},
                {"content_type", std::string(content)}
            }, "response sent"s);
    }

    void LoggerHandler::LogStartup(const net::ip::port_type& port, const net::ip::address& address) {
        BoostLogConsoleOutput(
            json::value{
                {"port"s, port},
                {"address"s, address.to_string()}
            }, "server started"s);
    }

    void LoggerHandler::BoostLogBaseSetup() {
        // активируем кастомные атрибуты
        logging::add_common_attributes();

        // Создаем бэкенд для синки, который будет использовать stream_
        auto cout_backend = boost::make_shared<logging::sinks::text_ostream_backend>();
        cout_backend->add_stream(boost::shared_ptr<std::ostream>(&stream_, boost::null_deleter()));
        cout_backend->auto_flush(true);

        // Создаем синку и привязываем ее к ядру логгера
        typedef logging::sinks::synchronous_sink<logging::sinks::text_ostream_backend> text_sink;
        boost::shared_ptr<text_sink> sink(new text_sink(cout_backend));
        sink->set_formatter(&detail::BaseFormatter);
        logging::core::get()->add_sink(sink);
    }

    void LoggerHandler::BoostLogConsoleOutput(const json::value& data, const std::string& message) {
        // блокируем мьютекс, чтобы не повторялись выводы
        std::lock_guard<std::mutex> lock(logger_mutex_);
        // производим запись с добавлением требуемых данных
        BOOST_LOG_TRIVIAL(info)
            << logging::add_value(additional_message, message)
            << logging::add_value(additional_data, data);
    }

} // namespace logger_handler