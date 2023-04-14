#pragma once

//#ifdef BOOST_USE_WINAPI_VERSION
//    #ifdef _WIN32
//#define BOOST_USE_WINAPI_VERSION 0x0601
//    #endif
//#endif

#include "domain.h"
#include "boost_json.h"

#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/date_time.hpp>

#include <iostream>
#include <mutex>

namespace logger_handler {

    using namespace std::literals;
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace json = boost::json;

    namespace net = boost::asio;
    namespace sys = boost::system;
    using tcp = net::ip::tcp;

    namespace logging = boost::log;
    namespace sinks = boost::log::sinks;
    namespace keywords = boost::log::keywords;
    namespace expr = boost::log::expressions;
    namespace attrs = boost::log::attributes;
    namespace pt = boost::posix_time;

    BOOST_LOG_ATTRIBUTE_KEYWORD(line_id, "LineID", unsigned int)
    BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
    BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", json::value)
    BOOST_LOG_ATTRIBUTE_KEYWORD(additional_message, "AdditionalMessage", std::string)

    // TODO убрать класс оставить только методы, доступные по инклюду хеддера, как сделал с Boost::JSON

    namespace detail {

        void BaseFormatter(logging::record_view const& rec, logging::formatting_ostream& strm);

    } // namespace detail

    class LoggerHandler {
    public:
        LoggerHandler(std::ostream& ostream) : stream_(ostream) {
            BoostLogBaseSetup();
        }

        LoggerHandler(const LoggerHandler&) = delete;
        LoggerHandler& operator=(const LoggerHandler&) = delete;

        ~LoggerHandler() {

            if (IsShutdown_ && !IsException_) {
                //LogShutdown();
            }
        }

        void LogShutdown();
        void LogException(const std::exception&);
        void LogError(beast::error_code ec, std::string_view location);
        void LogRequest(const http_handler::StringRequest& req, std::string_view remote_address);
        void LogResponse(int code, std::string_view content, uint64_t time, std::string_view remote_address);
        void LogStartup(const net::ip::port_type& port, const net::ip::address& address);

    private:
        std::ostream& stream_;
        std::mutex logger_mutex_;
        bool IsException_ = false;
        bool IsShutdown_ = true;

        void BoostLogBaseSetup();
        void BoostLogConsoleOutput(const json::value& data, const std::string& message);
    };

} // namespace logger_handler