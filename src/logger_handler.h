/*
    Для корректной работы необходимо в мейне вызвать 
    $ detail::BoostLogBaseSetup(std::ostream& out)

    Все методы доступны по подключенному хеддеру.
    При необходимости можно предварительно настроить Formater
*/

#pragma once

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

namespace logger_handler {

    using namespace std::literals;

    namespace beast = boost::beast;
    namespace json = boost::json;

    namespace net = boost::asio;
    namespace sys = boost::system;

    namespace logging = boost::log;
    namespace sinks = boost::log::sinks;

    BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
    BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", json::value)
    BOOST_LOG_ATTRIBUTE_KEYWORD(additional_message, "AdditionalMessage", std::string)

    void LogShutdown();
    void LogException(const std::exception&);
    void LogError(beast::error_code ec, std::string_view location);
    void LogRequest(const http_handler::StringRequest& req, std::string_view remote_address);
    void LogResponse(int code, std::string_view content, uint64_t time, std::string_view remote_address);
    void LogStartup(const net::ip::port_type& port, const net::ip::address& address);

    namespace detail {

        void BaseFormatter(logging::record_view const& rec, logging::formatting_ostream& strm);
        void BoostLogBaseSetup(std::ostream& out);
        void BoostLogConsoleOutput(const json::value& data, const std::string& message);

    } // namespace detail

} // namespace logger_handler