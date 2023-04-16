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

        TestApiMapsList();
        TestApiMapOne();
        TestApiMapNotFound();
        TestApiBadRequest();
        //TestApiGameLogin();

        return *this;
    }

    void SimpleTest::TestApiMapsList() {
        try
        {
            // ������������� ���������� � ��������, ��������� ���������� ����������
            stream_.connect(endpoint_);

            // ������� ������ �������
            http::request<http::string_body> req{ http::verb::get, "/api/v1/maps", 11 };
            req.set(http::field::host, endpoint_->endpoint().address().to_string());
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

            // ���������� ������ �� ������
            http::write(stream_, req);

            // ������� ������ ������
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // �������� ����� �� �������
            http::read(stream_, buffer, res);

            {
                // ��������� ���������� ���� ��������
                auto const& content = res.find(http::field::content_type);
                if (content != res.end()) {
                    assert(content->value() == http_handler::ContentType::APP_JSON);
                }
                else {
                    assert(false);
                }

                // ��������� ���������� ���� ������ �������
                assert(res.result_int() == 200);

                // ��������� ���������� ������ ������ � ���������� � �����
                // ��������� ���� � ���������� ������
                std::fstream file("../test/test_api_maps.txt", std::ios::in);

                // ����� �� �������������� � ���������� ����� � ������ ������ ������ ���� � �����
                std::stringstream buffer;
                buffer << file.rdbuf(); file.close();

                // �������� ����� ������� �� ������ � JSON
                auto server_resp = json_detail::ParseTextToBoostJson(
                    boost::beast::buffers_to_string(res.body().data()));
                // �������� ����� �� ����� � JSON
                auto file_resp = json_detail::ParseTextToBoostJson(buffer.str());
                
                // ���������� ���������� ��������
                assert(server_resp == file_resp);
            }


            // ��������� ���������� � ��������
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // ���� ��������� ������, ������� �� ���������
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
            // ������������� ���������� � ��������, ��������� ���������� ����������
            stream_.connect(endpoint_);

            // ������� ������ �������
            http::request<http::string_body> req{ http::verb::get, "/api/v1/maps/map1", 11 };
            req.set(http::field::host, endpoint_->endpoint().address().to_string());
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

            // ���������� ������ �� ������
            http::write(stream_, req);

            // ������� ������ ������
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // �������� ����� �� �������
            http::read(stream_, buffer, res);

            {
                // ��������� ���������� ���� ��������
                auto const& content = res.find(http::field::content_type);
                if (content != res.end()) {
                    assert(content->value() == http_handler::ContentType::APP_JSON);
                }
                else {
                    assert(false);
                }

                // ��������� ���������� ���� ������ �������
                assert(res.result_int() == 200);

                // ��������� ���������� ������ ������ � ���������� � �����
                // ��������� ���� � ���������� ������
                std::fstream file("../test/test_api_map1.txt", std::ios::in);

                // ����� �� �������������� � ���������� ����� � ������ ������ ������ ���� � �����
                std::stringstream buffer;
                buffer << file.rdbuf(); file.close();

                // �������� ����� ������� �� ������ � JSON
                auto server_resp = json_detail::ParseTextToBoostJson(
                    boost::beast::buffers_to_string(res.body().data()));
                // �������� ����� �� ����� � JSON
                auto file_resp = json_detail::ParseTextToBoostJson(buffer.str());

                // ���������� ���������� ��������
                assert(server_resp == file_resp);
            }

            // ��������� ���������� � ��������
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // ���� ��������� ������, ������� �� ���������
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
            // ������������� ���������� � ��������, ��������� ���������� ����������
            stream_.connect(endpoint_);

            // ������� ������ �������
            http::request<http::string_body> req{ http::verb::get, "/api/v1/maps/map15", 11 };
            req.set(http::field::host, endpoint_->endpoint().address().to_string());
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

            // ���������� ������ �� ������
            http::write(stream_, req);

            // ������� ������ ������
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // �������� ����� �� �������
            http::read(stream_, buffer, res);

            {
                // ��������� ���������� ���� ��������
                auto const& content = res.find(http::field::content_type);
                if (content != res.end()) {
                    assert(content->value() == http_handler::ContentType::APP_JSON);
                }
                else {
                    assert(false);
                }

                // ��������� ���������� ���� ������ �������
                assert(res.result_int() == 404);

                // ��������� ���������� ������ ������ � ���������� � �����
                // ��������� ���� � ���������� ������
                std::fstream file("../test/test_api_map_not_found.txt", std::ios::in);

                // ����� �� �������������� � ���������� ����� � ������ ������ ������ ���� � �����
                std::stringstream buffer;
                buffer << file.rdbuf(); file.close();

                // �������� ����� ������� �� ������ � JSON
                auto server_resp = json_detail::ParseTextToBoostJson(
                    boost::beast::buffers_to_string(res.body().data()));
                // �������� ����� �� ����� � JSON
                auto file_resp = json_detail::ParseTextToBoostJson(buffer.str());

                // ���������� ���������� ��������
                assert(server_resp == file_resp);
            }

            // ��������� ���������� � ��������
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // ���� ��������� ������, ������� �� ���������
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
            // ������������� ���������� � ��������, ��������� ���������� ����������
            stream_.connect(endpoint_);

            // ������� ������ �������
            http::request<http::string_body> req{ http::verb::get, "/api/v333/maps/map1", 11 };
            req.set(http::field::host, endpoint_->endpoint().address().to_string());
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

            // ���������� ������ �� ������
            http::write(stream_, req);

            // ������� ������ ������
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // �������� ����� �� �������
            http::read(stream_, buffer, res);

            {
                // ��������� ���������� ���� ��������
                auto const& content = res.find(http::field::content_type);
                if (content != res.end()) {
                    assert(content->value() == http_handler::ContentType::APP_JSON);
                }
                else {
                    assert(false);
                }

                // ��������� ���������� ���� ������ �������
                assert(res.result_int() == 400);

                // ��������� ���������� ������ ������ � ���������� � �����
                // ��������� ���� � ���������� ������
                std::fstream file("../test/test_api_bad_request.txt", std::ios::in);

                // ����� �� �������������� � ���������� ����� � ������ ������ ������ ���� � �����
                std::stringstream buffer;
                buffer << file.rdbuf(); file.close();

                // �������� ����� ������� �� ������ � JSON
                auto server_resp = json_detail::ParseTextToBoostJson(
                    boost::beast::buffers_to_string(res.body().data()));
                // �������� ����� �� ����� � JSON
                auto file_resp = json_detail::ParseTextToBoostJson(buffer.str());

                // ���������� ���������� ��������
                assert(server_resp == file_resp);
            }

            // ��������� ���������� � ��������
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // ���� ��������� ������, ������� �� ���������
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
            // ������������� ���������� � ��������, ��������� ���������� ����������
            stream_.connect(endpoint_);

            // ������� ������ �������
            http::request<http::string_body> req{ http::verb::post, "/api/v1/game/join"sv, 11 };
            req.set(http::field::host, endpoint_->endpoint().address().to_string());
            req.set(http::field::content_type, "application/json"sv);
            req.set(http::field::body, "{\"userName\": \"Scooby Doo\", \"mapId\": \"map1\"} "sv);
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

            // ���������� ������ �� ������
            http::write(stream_, req);

            // ������� ������ ������
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // �������� ����� �� �������
            http::read(stream_, buffer, res);

            {
                // ��������� ���������� ���� ��������
                auto const& content = res.find(http::field::content_type);
                if (content != res.end()) {
                    assert(content->value() == http_handler::ContentType::APP_JSON);
                }
                else {
                    assert(false);
                }

                // ��������� cache-control
                auto const& cache = res.find(http::field::cache_control);
                if (cache != res.end()) {
                    assert(cache->value() == "no-cache"sv);
                }
                else {
                    assert(false);
                }

                // ��������� ���������� ���� ������ �������
                assert(res.result_int() == 200);

                // �������� ����� ������� �� ������ � JSON
                auto server_resp = json_detail::ParseTextToBoostJson(
                    boost::beast::buffers_to_string(res.body().data()));

                // ��������� � ������ ������� ����� � ������ � ����
                assert(server_resp.as_object().contains("authToken"));
                assert(server_resp.as_object().contains("playerId"));
            }

            // ��������� ���������� � ��������
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // ���� ��������� ������, ������� �� ���������
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
        }
        catch (std::exception const& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }


} // namespace test