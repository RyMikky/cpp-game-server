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

        //TestApiGameLogin();
        //std::cerr << "TestApiGameLogin()::Complete::Status.............................Ok\n" << std::endl;

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

} // namespace test