#include "logger_handler.h"

namespace logger_handler {

    void LogShutdown() {
        detail::BoostLogConsoleOutput(
            json::value{
                {"code"s, 0}
            }, "server exited"s);
    }

    void LogException(const std::exception& exc) {
        detail::BoostLogConsoleOutput(
            json::value{
                {"code"s, EXIT_FAILURE},
                {"exception"s, exc.what()}
            }, "server exited"s);
    }

    void LogError(beast::error_code ec, std::string_view location) {

        detail::BoostLogConsoleOutput(
            json::value{
                {"code"s, ec.value()},
                {"text"s, ec.message()},
                {"where"s, std::string(location)}
            }, "error"s);
    }

    void LogRequest(const http_handler::StringRequest& req, std::string_view remote_address) {

        detail::BoostLogConsoleOutput(
            json::value{
                {"ip"s, std::string(remote_address)},
                {"URI"s, req.target()},
                {"method"s, req.method_string()}
            }, "request received"s);
    }

    void LogResponse(int code, std::string_view content, uint64_t time, std::string_view remote_address) {

        detail::BoostLogConsoleOutput(
            json::value{
                {"ip"s, std::string(remote_address)},
                {"response_time"s, time},
                {"code"s, code},
                {"content_type", std::string(content)}
            }, "response sent"s);
    }

    void LogStartup(const net::ip::port_type& port, const net::ip::address& address) {
        detail::BoostLogConsoleOutput(
            json::value{
                {"port"s, port},
                {"address"s, address.to_string()}
            }, "server started"s);
    }

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

        void BoostLogBaseSetup(std::ostream& out) {
            // активируем кастомные атрибуты
            logging::add_common_attributes();

            // Создаем бэкенд для синки, который будет использовать stream_
            auto cout_backend = boost::make_shared<logging::sinks::text_ostream_backend>();
            cout_backend->add_stream(boost::shared_ptr<std::ostream>(&out, boost::null_deleter()));
            cout_backend->auto_flush(true);

            // Создаем синку и привязываем ее к ядру логгера
            typedef logging::sinks::synchronous_sink<logging::sinks::text_ostream_backend> text_sink;
            boost::shared_ptr<text_sink> sink(new text_sink(cout_backend));
            sink->set_formatter(&detail::BaseFormatter);
            logging::core::get()->add_sink(sink);
        }

        void BoostLogConsoleOutput(const json::value& data, const std::string& message) {
            // производим запись с добавлением требуемых данных
            BOOST_LOG_TRIVIAL(info)
                << logging::add_value(additional_message, message)
                << logging::add_value(additional_data, data);
        }

    } // namespace detail

} // namespace logger_handler