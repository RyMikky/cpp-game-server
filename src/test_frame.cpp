#include "test_frame.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cassert>

namespace test {

    SimpleTest::SimpleTest(std::string_view address, std::string_view port) : resolver_(ioc_), stream_(ioc_) {
        try {
            endpoint_ = resolver_.resolve(address, port);

            RunAllTests();
        }
        catch (std::exception const& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    SimpleTest& SimpleTest::RunAllTests() {

        std::cerr << std::endl;
        std::cerr << "SimpleTest::RunAllTests()::Begin.................................Ok\n" << std::endl;

        TestApiMapsList();
        std::cerr << "TestApiMapsList()::Complete::Status..............................Ok\n" << std::endl;

        TestApiMapOne();
        std::cerr << "TestApiMapOne()::Complete::Status................................Ok\n" << std::endl;

        TestApiMapNotFound();
        std::cerr << "TestApiMapNotFound()::Complete::Status...........................Ok\n" << std::endl;

        TestApiBadRequest();
        std::cerr << "TestApiBadRequest()::Complete::Status............................Ok\n" << std::endl;

        TestApiGameLogin();
        std::cerr << "TestApiGameLogin()::Complete::Status.............................Ok\n" << std::endl;

        TestApiGameSecondLogin();
        std::cerr << "TestApiGameSecondLogin()::Complete::Status.......................Ok\n" << std::endl;

        TestApiGameLoginMissName();
        std::cerr << "TestApiGameLoginMissName()::Complete::Status.....................Ok\n" << std::endl;

        TestApiGameLoginInvalidName();
        std::cerr << "TestApiGameLoginInvalidName()::Complete::Status..................Ok\n" << std::endl;

        TestApiGameLoginMissMap();
        std::cerr << "TestApiGameLoginMissMap()::Complete::Status......................Ok\n" << std::endl;

        TestApiGameLoginMapNotFound();
        std::cerr << "TestApiGameLoginMapNotFound()::Complete::Status..................Ok\n" << std::endl;

        TestApiGameLoginInvalidMethod();
        std::cerr << "TestApiGameLoginInvalidMethod()::Complete::Status................Ok\n" << std::endl;

        TestAuthorizationSet();

        return *this;
    }

    void SimpleTest::TestApiMapsList() {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // создаем объект запроса
            http::request<http::string_body> req{ http::verb::get, "/api/v1/maps", 11 };
            req.set(http::field::host, endpoint_->endpoint().address().to_string());
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

            // отправляем запрос на сервер
            http::write(stream_, req);

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            {
                // проверяем совпадение типа контента
                auto const& content = res.find(http::field::content_type);
                if (content != res.end()) {
                    assert(content->value() == http_handler::ContentType::APP_JSON);
                }
                else {
                    assert(false);
                }

                // првоеряем совпадение кода ответа сервера
                assert(res.result_int() == 200);

                // проверяем совпадение строки ответа с заготовкой в файле
                // открываем файл с заготовкой ответа
                std::fstream file("../test/test_api_maps.txt", std::ios::in);

                // чтобы не заморачиваться с переносами строк и прочим битово читаем файл в поток
                std::stringstream buffer;
                buffer << file.rdbuf(); file.close();

                // загоняем ответ сервера из строки в JSON
                auto server_resp = json_detail::ParseTextToBoostJson(
                    boost::beast::buffers_to_string(res.body().data()));
                // загоняем буфер из файла в JSON
                auto file_resp = json_detail::ParseTextToBoostJson(buffer.str());
                
                // сравниваем полученные значения
                assert(server_resp == file_resp);
            }


            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
        }
        catch (std::exception const& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    void SimpleTest::TestApiMapOne() {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // создаем объект запроса
            http::request<http::string_body> req{ http::verb::get, "/api/v1/maps/map1", 11 };
            req.set(http::field::host, endpoint_->endpoint().address().to_string());
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

            // отправляем запрос на сервер
            http::write(stream_, req);

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            {
                // проверяем совпадение типа контента
                auto const& content = res.find(http::field::content_type);
                if (content != res.end()) {
                    assert(content->value() == http_handler::ContentType::APP_JSON);
                }
                else {
                    assert(false);
                }

                // првоеряем совпадение кода ответа сервера
                assert(res.result_int() == 200);

                // проверяем совпадение строки ответа с заготовкой в файле
                // открываем файл с заготовкой ответа
                std::fstream file("../test/test_api_map1.txt", std::ios::in);

                // чтобы не заморачиваться с переносами строк и прочим битово читаем файл в поток
                std::stringstream buffer;
                buffer << file.rdbuf(); file.close();

                // загоняем ответ сервера из строки в JSON
                auto server_resp = json_detail::ParseTextToBoostJson(
                    boost::beast::buffers_to_string(res.body().data()));
                // загоняем буфер из файла в JSON
                auto file_resp = json_detail::ParseTextToBoostJson(buffer.str());

                // сравниваем полученные значения
                assert(server_resp == file_resp);
            }

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
        }
        catch (std::exception const& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    void SimpleTest::TestApiMapNotFound() {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // создаем объект запроса
            http::request<http::string_body> req{ http::verb::get, "/api/v1/maps/map15", 11 };
            req.set(http::field::host, endpoint_->endpoint().address().to_string());
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

            // отправляем запрос на сервер
            http::write(stream_, req);

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            {
                // проверяем совпадение типа контента
                auto const& content = res.find(http::field::content_type);
                if (content != res.end()) {
                    assert(content->value() == http_handler::ContentType::APP_JSON);
                }
                else {
                    assert(false);
                }

                // првоеряем совпадение кода ответа сервера
                assert(res.result_int() == 404);

                // проверяем совпадение строки ответа с заготовкой в файле
                // открываем файл с заготовкой ответа
                std::fstream file("../test/test_api_map_not_found.txt", std::ios::in);

                // чтобы не заморачиваться с переносами строк и прочим битово читаем файл в поток
                std::stringstream buffer;
                buffer << file.rdbuf(); file.close();

                // загоняем ответ сервера из строки в JSON
                auto server_resp = json_detail::ParseTextToBoostJson(
                    boost::beast::buffers_to_string(res.body().data()));
                // загоняем буфер из файла в JSON
                auto file_resp = json_detail::ParseTextToBoostJson(buffer.str());

                // сравниваем полученные значения
                assert(server_resp == file_resp);
            }

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
        }
        catch (std::exception const& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    void SimpleTest::TestApiBadRequest() {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // создаем объект запроса
            http::request<http::string_body> req{ http::verb::get, "/api/v333/maps/map1", 11 };
            req.set(http::field::host, endpoint_->endpoint().address().to_string());
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

            // отправляем запрос на сервер
            http::write(stream_, req);

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            {
                // проверяем совпадение типа контента
                auto const& content = res.find(http::field::content_type);
                if (content != res.end()) {
                    assert(content->value() == http_handler::ContentType::APP_JSON);
                }
                else {
                    assert(false);
                }

                // првоеряем совпадение кода ответа сервера
                assert(res.result_int() == 400);

                // проверяем совпадение строки ответа с заготовкой в файле
                // открываем файл с заготовкой ответа
                std::fstream file("../test/test_api_bad_request.txt", std::ios::in);

                // чтобы не заморачиваться с переносами строк и прочим битово читаем файл в поток
                std::stringstream buffer;
                buffer << file.rdbuf(); file.close();

                // загоняем ответ сервера из строки в JSON
                auto server_resp = json_detail::ParseTextToBoostJson(
                    boost::beast::buffers_to_string(res.body().data()));
                // загоняем буфер из файла в JSON
                auto file_resp = json_detail::ParseTextToBoostJson(buffer.str());

                // сравниваем полученные значения
                assert(server_resp == file_resp);
            }

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
        }
        catch (std::exception const& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    void SimpleTest::TestApiGameLogin() {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // создаем объект запроса
            http::request<http::string_body> req{ http::verb::post, "/api/v1/game/join"sv, 11 };
            req.set(http::field::host, endpoint_->endpoint().address().to_string());
            req.set(http::field::content_type, "application/json"sv);
            req.set(http::field::body, "{\"userName\": \"Scooby Doo\", \"mapId\": \"map1\"} "sv);
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

            // отправляем запрос на сервер
            http::write(stream_, req);

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            {
                // проверяем совпадение типа контента
                auto const& content = res.find(http::field::content_type);
                if (content != res.end()) {
                    assert(content->value() == http_handler::ContentType::APP_JSON);
                }
                else {
                    assert(false);
                }

                // проверяем cache-control
                auto const& cache = res.find(http::field::cache_control);
                if (cache != res.end()) {
                    assert(cache->value() == "no-cache"sv);
                }
                else {
                    assert(false);
                }

                // првоеряем совпадение кода ответа сервера
                assert(res.result_int() == 200);

                // загоняем ответ сервера из строки в JSON
                auto server_resp = json_detail::ParseTextToBoostJson(
                    boost::beast::buffers_to_string(res.body().data()));

                // првоеряем в ответе наличие строк о токене и айди
                assert(server_resp.as_object().contains("authToken"));
                assert(server_resp.as_object().contains("playerId"));
                assert(server_resp.as_object().at("playerId") == 0);
            }

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
        }
        catch (std::exception const& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    void SimpleTest::TestApiGameSecondLogin() {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // создаем объект запроса
            http::request<http::string_body> req{ http::verb::post, "/api/v1/game/join"sv, 11 };
            req.set(http::field::host, endpoint_->endpoint().address().to_string());
            req.set(http::field::content_type, "application/json"sv);
            req.set(http::field::body, "{\"userName\": \"Whelma Shnizel\", \"mapId\": \"map1\"} "sv);
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

            // отправляем запрос на сервер
            http::write(stream_, req);

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            {
                // проверяем совпадение типа контента
                auto const& content = res.find(http::field::content_type);
                if (content != res.end()) {
                    assert(content->value() == http_handler::ContentType::APP_JSON);
                }
                else {
                    assert(false);
                }

                // проверяем cache-control
                auto const& cache = res.find(http::field::cache_control);
                if (cache != res.end()) {
                    assert(cache->value() == "no-cache"sv);
                }
                else {
                    assert(false);
                }

                // првоеряем совпадение кода ответа сервера
                assert(res.result_int() == 200);

                // загоняем ответ сервера из строки в JSON
                auto server_resp = json_detail::ParseTextToBoostJson(
                    boost::beast::buffers_to_string(res.body().data()));

                // првоеряем в ответе наличие строк о токене и айди
                assert(server_resp.as_object().contains("authToken"));
                assert(server_resp.as_object().contains("playerId"));
                assert(server_resp.as_object().at("playerId") == 1);
            }

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
        }
        catch (std::exception const& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    void SimpleTest::TestApiGameLoginMissName() {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // создаем объект запроса
            http::request<http::string_body> req{ http::verb::post, "/api/v1/game/join"sv, 11 };
            req.set(http::field::host, endpoint_->endpoint().address().to_string());
            req.set(http::field::content_type, "application/json"sv);
            req.set(http::field::body, "{\"mapId\": \"map1\"}"sv);
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

            // отправляем запрос на сервер
            http::write(stream_, req);

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            {
                // проверяем совпадение типа контента
                auto const& content = res.find(http::field::content_type);
                if (content != res.end()) {
                    assert(content->value() == http_handler::ContentType::APP_JSON);
                }
                else {
                    assert(false);
                }

                // проверяем cache-control
                auto const& cache = res.find(http::field::cache_control);
                if (cache != res.end()) {
                    assert(cache->value() == "no-cache"sv);
                }
                else {
                    assert(false);
                }

                // првоеряем совпадение кода ответа сервера
                assert(res.result_int() == 400);

                // проверяем совпадение строки ответа с заготовкой в файле
                // открываем файл с заготовкой ответа
                std::fstream file("../test/test_api_game_login_miss_name_or_map.txt", std::ios::in);

                // чтобы не заморачиваться с переносами строк и прочим битово читаем файл в поток
                std::stringstream buffer;
                buffer << file.rdbuf(); file.close();

                // загоняем ответ сервера из строки в JSON
                auto server_resp = json_detail::ParseTextToBoostJson(
                    boost::beast::buffers_to_string(res.body().data()));

                // загоняем буфер из файла в JSON
                auto file_resp = json_detail::ParseTextToBoostJson(buffer.str());

                // сравниваем полученные значения
                assert(server_resp == file_resp);
            }

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
        }
        catch (std::exception const& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    void SimpleTest::TestApiGameLoginInvalidName() {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // создаем объект запроса
            http::request<http::string_body> req{ http::verb::post, "/api/v1/game/join"sv, 11 };
            req.set(http::field::host, endpoint_->endpoint().address().to_string());
            req.set(http::field::content_type, "application/json"sv);
            req.set(http::field::body, "{\"userName\": \"\", \"mapId\": \"map1\"} "sv);
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

            // отправляем запрос на сервер
            http::write(stream_, req);

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            {
                // проверяем совпадение типа контента
                auto const& content = res.find(http::field::content_type);
                if (content != res.end()) {
                    assert(content->value() == http_handler::ContentType::APP_JSON);
                }
                else {
                    assert(false);
                }

                // проверяем cache-control
                auto const& cache = res.find(http::field::cache_control);
                if (cache != res.end()) {
                    assert(cache->value() == "no-cache"sv);
                }
                else {
                    assert(false);
                }

                // првоеряем совпадение кода ответа сервера
                assert(res.result_int() == 400);

                // проверяем совпадение строки ответа с заготовкой в файле
                // открываем файл с заготовкой ответа
                std::fstream file("../test/test_api_game_login_invalid_name.txt", std::ios::in);

                // чтобы не заморачиваться с переносами строк и прочим битово читаем файл в поток
                std::stringstream buffer;
                buffer << file.rdbuf(); file.close();

                // загоняем ответ сервера из строки в JSON
                auto server_resp = json_detail::ParseTextToBoostJson(
                    boost::beast::buffers_to_string(res.body().data()));
                // загоняем буфер из файла в JSON
                auto file_resp = json_detail::ParseTextToBoostJson(buffer.str());

                // сравниваем полученные значения
                assert(server_resp == file_resp);
            }

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
        }
        catch (std::exception const& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    void SimpleTest::TestApiGameLoginMissMap() {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // создаем объект запроса
            http::request<http::string_body> req{ http::verb::post, "/api/v1/game/join"sv, 11 };
            req.set(http::field::host, endpoint_->endpoint().address().to_string());
            req.set(http::field::content_type, "application/json"sv);
            req.set(http::field::body, "{\"userName\": \"Vasya\"} "sv);
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

            // отправляем запрос на сервер
            http::write(stream_, req);

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            {
                // проверяем совпадение типа контента
                auto const& content = res.find(http::field::content_type);
                if (content != res.end()) {
                    assert(content->value() == http_handler::ContentType::APP_JSON);
                }
                else {
                    assert(false);
                }

                // проверяем cache-control
                auto const& cache = res.find(http::field::cache_control);
                if (cache != res.end()) {
                    assert(cache->value() == "no-cache"sv);
                }
                else {
                    assert(false);
                }

                // првоеряем совпадение кода ответа сервера
                assert(res.result_int() == 400);

                // проверяем совпадение строки ответа с заготовкой в файле
                // открываем файл с заготовкой ответа
                std::fstream file("../test/test_api_game_login_miss_name_or_map.txt", std::ios::in);

                // чтобы не заморачиваться с переносами строк и прочим битово читаем файл в поток
                std::stringstream buffer;
                buffer << file.rdbuf(); file.close();

                // загоняем ответ сервера из строки в JSON
                auto server_resp = json_detail::ParseTextToBoostJson(
                    boost::beast::buffers_to_string(res.body().data()));
                // загоняем буфер из файла в JSON
                auto file_resp = json_detail::ParseTextToBoostJson(buffer.str());

                // сравниваем полученные значения
                assert(server_resp == file_resp);
            }

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
        }
        catch (std::exception const& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    void SimpleTest::TestApiGameLoginMapNotFound() {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // создаем объект запроса
            http::request<http::string_body> req{ http::verb::post, "/api/v1/game/join"sv, 11 };
            req.set(http::field::host, endpoint_->endpoint().address().to_string());
            req.set(http::field::content_type, "application/json"sv);
            req.set(http::field::body, "{\"userName\": \"Vasya\", \"mapId\": \"map47\"} "sv);
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

            // отправляем запрос на сервер
            http::write(stream_, req);

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            {
                // проверяем совпадение типа контента
                auto const& content = res.find(http::field::content_type);
                if (content != res.end()) {
                    assert(content->value() == http_handler::ContentType::APP_JSON);
                }
                else {
                    assert(false);
                }

                // проверяем cache-control
                auto const& cache = res.find(http::field::cache_control);
                if (cache != res.end()) {
                    assert(cache->value() == "no-cache"sv);
                }
                else {
                    assert(false);
                }

                // првоеряем совпадение кода ответа сервера
                assert(res.result_int() == 404);

                // проверяем совпадение строки ответа с заготовкой в файле
                // открываем файл с заготовкой ответа
                std::fstream file("../test/test_api_map_not_found.txt", std::ios::in);

                // чтобы не заморачиваться с переносами строк и прочим битово читаем файл в поток
                std::stringstream buffer;
                buffer << file.rdbuf(); file.close();

                // загоняем ответ сервера из строки в JSON
                auto server_resp = json_detail::ParseTextToBoostJson(
                    boost::beast::buffers_to_string(res.body().data()));
                // загоняем буфер из файла в JSON
                auto file_resp = json_detail::ParseTextToBoostJson(buffer.str());

                // сравниваем полученные значения
                assert(server_resp == file_resp);
            }

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
        }
        catch (std::exception const& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    void SimpleTest::TestApiGameLoginInvalidMethod() {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // создаем объект запроса
            http::request<http::string_body> req{ http::verb::get, "/api/v1/game/join"sv, 11 };
            req.set(http::field::host, endpoint_->endpoint().address().to_string());
            req.set(http::field::content_type, "application/json"sv);
            req.set(http::field::body, "{\"userName\": \"Scooby Doo\", \"mapId\": \"map1\"} "sv);
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

            // отправляем запрос на сервер
            http::write(stream_, req);

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            {
                // проверяем совпадение типа контента
                auto const& content = res.find(http::field::content_type);
                if (content != res.end()) {
                    assert(content->value() == http_handler::ContentType::APP_JSON);
                }
                else {
                    assert(false);
                }

                // проверяем cache-control
                auto const& cache = res.find(http::field::cache_control);
                if (cache != res.end()) {
                    assert(cache->value() == "no-cache"sv);
                }
                else {
                    assert(false);
                }

                // проверяем allow
                auto const& allow = res.find(http::field::allow);
                if (allow != res.end()) {
                    assert(allow->value() == "POST"sv);
                }
                else {
                    assert(false);
                }

                // првоеряем совпадение кода ответа сервера
                assert(res.result_int() == 405);

                // проверяем совпадение строки ответа с заготовкой в файле
                // открываем файл с заготовкой ответа
                std::fstream file("../test/test_api_game_login_invalid_method.txt", std::ios::in);

                // чтобы не заморачиваться с переносами строк и прочим битово читаем файл в поток
                std::stringstream buffer;
                buffer << file.rdbuf(); file.close();

                // загоняем ответ сервера из строки в JSON
                auto server_resp = json_detail::ParseTextToBoostJson(
                    boost::beast::buffers_to_string(res.body().data()));
                // загоняем буфер из файла в JSON
                auto file_resp = json_detail::ParseTextToBoostJson(buffer.str());

                // сравниваем полученные значения
                assert(server_resp == file_resp);
            }

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
        }
        catch (std::exception const& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }


    SimpleTest::AuthResp SimpleTest::TestApiGameThirdLogin() {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // создаем объект запроса
            http::request<http::string_body> req{ http::verb::post, "/api/v1/game/join"sv, 11 };
            req.set(http::field::host, endpoint_->endpoint().address().to_string());
            req.set(http::field::content_type, "application/json"sv);
            req.set(http::field::body, "{\"userName\": \"Mega Pups\", \"mapId\": \"map1\"} "sv);
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

            // отправляем запрос на сервер
            http::write(stream_, req);

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            boost::json::value server_resp;

            {
                // проверяем совпадение типа контента
                auto const& content = res.find(http::field::content_type);
                if (content != res.end()) {
                    assert(content->value() == http_handler::ContentType::APP_JSON);
                }
                else {
                    assert(false);
                }

                // проверяем cache-control
                auto const& cache = res.find(http::field::cache_control);
                if (cache != res.end()) {
                    assert(cache->value() == "no-cache"sv);
                }
                else {
                    assert(false);
                }

                // првоеряем совпадение кода ответа сервера
                assert(res.result_int() == 200);

                // загоняем ответ сервера из строки в JSON
                server_resp = json_detail::ParseTextToBoostJson(
                    boost::beast::buffers_to_string(res.body().data()));

                // првоеряем в ответе наличие строк о токене и айди
                assert(server_resp.as_object().contains("authToken"));
                assert(server_resp.as_object().contains("playerId"));
                assert(server_resp.as_object().at("playerId") == 2);
            }

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }

            // сделаем "грязные" преобразования в С-стиле, ай-ай-ай, Страуструб голову бы оторвал)
            return { (std::string)server_resp.as_object().at("authToken").as_string(), 
                (size_t)server_resp.as_object().at("playerId").as_int64() };
        }
        catch (std::exception const& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    void SimpleTest::TestApiGamePlayerList(SimpleTest::AuthResp data) {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // создаем объект запроса
            http::request<http::string_body> req{ http::verb::get, "/api/v1/game/players"sv, 11 };
            req.set(http::field::host, endpoint_->endpoint().address().to_string());
            req.set(http::field::content_type, "application/json"sv);
            req.set(http::field::authorization, "Bearer " + data.first);
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

            // отправляем запрос на сервер
            http::write(stream_, req);

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            {
                // проверяем совпадение типа контента
                auto const& content = res.find(http::field::content_type);
                if (content != res.end()) {
                    assert(content->value() == http_handler::ContentType::APP_JSON);
                }
                else {
                    assert(false);
                }

                // првоеряем совпадение кода ответа сервера
                assert(res.result_int() == 200);

                // проверяем совпадение строки ответа с заготовкой в файле
                // открываем файл с заготовкой ответа
                std::fstream file("../test/test_api_players_list.txt", std::ios::in);

                // чтобы не заморачиваться с переносами строк и прочим битово читаем файл в поток
                std::stringstream buffer;
                buffer << file.rdbuf(); file.close();

                // загоняем ответ сервера из строки в JSON
                auto server_resp = json_detail::ParseTextToBoostJson(
                    boost::beast::buffers_to_string(res.body().data()));
                // загоняем буфер из файла в JSON
                auto file_resp = json_detail::ParseTextToBoostJson(buffer.str());

                // сравниваем полученные значения
                assert(server_resp == file_resp);
            }


            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
        }
        catch (std::exception const& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    void SimpleTest::TestApiGameAuthorizationMissBody(AuthResp data){
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // создаем объект запроса
            http::request<http::string_body> req{ http::verb::get, "/api/v1/game/players"sv, 11 };
            req.set(http::field::host, endpoint_->endpoint().address().to_string());
            req.set(http::field::content_type, "application/json"sv);
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

            // отправляем запрос на сервер
            http::write(stream_, req);

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            {
                // проверяем совпадение типа контента
                auto const& content = res.find(http::field::content_type);
                if (content != res.end()) {
                    assert(content->value() == http_handler::ContentType::APP_JSON);
                }
                else {
                    assert(false);
                }

                // првоеряем совпадение кода ответа сервера
                assert(res.result_int() == 401);

                // проверяем совпадение строки ответа с заготовкой в файле
                // открываем файл с заготовкой ответа
                std::fstream file("../test/test_api_authorization_missing.txt", std::ios::in);

                // чтобы не заморачиваться с переносами строк и прочим битово читаем файл в поток
                std::stringstream buffer;
                buffer << file.rdbuf(); file.close();

                // загоняем ответ сервера из строки в JSON
                auto server_resp = json_detail::ParseTextToBoostJson(
                    boost::beast::buffers_to_string(res.body().data()));
                // загоняем буфер из файла в JSON
                auto file_resp = json_detail::ParseTextToBoostJson(buffer.str());

                // сравниваем полученные значения
                assert(server_resp == file_resp);
            }


            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
        }
        catch (std::exception const& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    void SimpleTest::TestApiGameAuthorizationMissing(AuthResp data){
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // создаем объект запроса
            http::request<http::string_body> req{ http::verb::get, "/api/v1/game/players"sv, 11 };
            req.set(http::field::host, endpoint_->endpoint().address().to_string());
            req.set(http::field::content_type, "application/json"sv);
            req.set(http::field::authorization, "Cearer " + data.first);
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

            // отправляем запрос на сервер
            http::write(stream_, req);

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            {
                // проверяем совпадение типа контента
                auto const& content = res.find(http::field::content_type);
                if (content != res.end()) {
                    assert(content->value() == http_handler::ContentType::APP_JSON);
                }
                else {
                    assert(false);
                }

                // првоеряем совпадение кода ответа сервера
                assert(res.result_int() == 401);

                // проверяем совпадение строки ответа с заготовкой в файле
                // открываем файл с заготовкой ответа
                std::fstream file("../test/test_api_authorization_missing.txt", std::ios::in);

                // чтобы не заморачиваться с переносами строк и прочим битово читаем файл в поток
                std::stringstream buffer;
                buffer << file.rdbuf(); file.close();

                // загоняем ответ сервера из строки в JSON
                auto server_resp = json_detail::ParseTextToBoostJson(
                    boost::beast::buffers_to_string(res.body().data()));
                // загоняем буфер из файла в JSON
                auto file_resp = json_detail::ParseTextToBoostJson(buffer.str());

                // сравниваем полученные значения
                assert(server_resp == file_resp);
            }


            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
        }
        catch (std::exception const& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    void SimpleTest::TestApiGameTokenNotFound(AuthResp data){
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // создаем объект запроса
            http::request<http::string_body> req{ http::verb::get, "/api/v1/game/players"sv, 11 };
            req.set(http::field::host, endpoint_->endpoint().address().to_string());
            req.set(http::field::content_type, "application/json"sv);
            req.set(http::field::authorization, "Bearer 68f75asfdsff");
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

            // отправляем запрос на сервер
            http::write(stream_, req);

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            {
                // проверяем совпадение типа контента
                auto const& content = res.find(http::field::content_type);
                if (content != res.end()) {
                    assert(content->value() == http_handler::ContentType::APP_JSON);
                }
                else {
                    assert(false);
                }

                // првоеряем совпадение кода ответа сервера
                assert(res.result_int() == 401);

                // проверяем совпадение строки ответа с заготовкой в файле
                // открываем файл с заготовкой ответа
                std::fstream file("../test/test_api_token_not_found.txt", std::ios::in);

                // чтобы не заморачиваться с переносами строк и прочим битово читаем файл в поток
                std::stringstream buffer;
                buffer << file.rdbuf(); file.close();

                // загоняем ответ сервера из строки в JSON
                auto server_resp = json_detail::ParseTextToBoostJson(
                    boost::beast::buffers_to_string(res.body().data()));
                // загоняем буфер из файла в JSON
                auto file_resp = json_detail::ParseTextToBoostJson(buffer.str());

                // сравниваем полученные значения
                assert(server_resp == file_resp);
            }


            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
        }
        catch (std::exception const& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    void SimpleTest::TestApiGamePlayerListInvalidMethod(SimpleTest::AuthResp data) {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // создаем объект запроса
            http::request<http::string_body> req{ http::verb::post, "/api/v1/game/players"sv, 11 };
            req.set(http::field::host, endpoint_->endpoint().address().to_string());
            req.set(http::field::content_type, "application/json"sv);
            req.set(http::field::authorization, "Bearer " + data.first);
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

            // отправляем запрос на сервер
            http::write(stream_, req);

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            {
                // проверяем совпадение типа контента
                auto const& content = res.find(http::field::content_type);
                if (content != res.end()) {
                    assert(content->value() == http_handler::ContentType::APP_JSON);
                }
                else {
                    assert(false);
                }

                // проверяем cache-control
                auto const& cache = res.find(http::field::cache_control);
                if (cache != res.end()) {
                    assert(cache->value() == "no-cache"sv);
                }
                else {
                    assert(false);
                }

                // проверяем allows
                auto const& allow = res.find(http::field::allow);
                if (allow != res.end()) {
                    assert(allow->value() == "GET, HEAD"sv);
                }
                else {
                    assert(false);
                }

                // првоеряем совпадение кода ответа сервера
                assert(res.result_int() == 405);

                // проверяем совпадение строки ответа с заготовкой в файле
                // открываем файл с заготовкой ответа
                std::fstream file("../test/test_api_players_list_invalid_method.txt", std::ios::in);

                // чтобы не заморачиваться с переносами строк и прочим битово читаем файл в поток
                std::stringstream buffer;
                buffer << file.rdbuf(); file.close();

                // загоняем ответ сервера из строки в JSON
                auto server_resp = json_detail::ParseTextToBoostJson(
                    boost::beast::buffers_to_string(res.body().data()));
                // загоняем буфер из файла в JSON
                auto file_resp = json_detail::ParseTextToBoostJson(buffer.str());
                // сравниваем полученные значения
                assert(server_resp == file_resp);
            }


            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
        }
        catch (std::exception const& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    void SimpleTest::TestAuthorizationSet() {

        SimpleTest::AuthResp data = SimpleTest::TestApiGameThirdLogin();
        std::cerr << "TestApiGameThirdLogin()::Complete::Status........................Ok\n" << std::endl;
        SimpleTest::TestApiGamePlayerList(data);
        std::cerr << "TestApiGamePlayerList()::Complete::Status........................Ok\n" << std::endl;
        SimpleTest::TestApiGameAuthorizationMissBody(data);
        std::cerr << "TestApiGameAuthorizationMissBody()::Complete::Status.............Ok\n" << std::endl;
        SimpleTest::TestApiGameAuthorizationMissing(data);
        std::cerr << "TestApiGameAuthorizationMissing()::Complete::Status..............Ok\n" << std::endl;
        SimpleTest::TestApiGameTokenNotFound(data);
        std::cerr << "TestApiGameTokenNotFound()::Complete::Status.....................Ok\n" << std::endl;
        SimpleTest::TestApiGamePlayerListInvalidMethod(data);
        std::cerr << "TestApiGamePlayerListInvalidMethod()::Complete::Status...........Ok\n" << std::endl;
    }

} // namespace test