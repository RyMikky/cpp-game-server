﻿#include "request_handler.h"
#include "boost_json.h"

namespace http_handler {

    // включает выполнение автотаймера, выкидывает исклоючение, если таймер уже включен
    RequestHandler& RequestHandler::StartGameTimer() {
        if (timer_enable_) {
            throw std::runtime_error("RequestHandler::game_timer_start::Error::game_timer already active");
        }

        if (timer_) {
            try
            {
                timer_->Start();
                timer_enable_ = true;
            }
            catch (const std::exception& e)
            {
                throw std::runtime_error("RequestHandler::game_timer_start::Error::" + std::string(e.what()));
            }
        }
        return *this;
    }

    // включает выполнение автотаймера, выкидывает исклоючение, если таймер уже включен
    RequestHandler& RequestHandler::StartGameTimer(std::chrono::milliseconds period) {
        if (timer_enable_) {
            throw std::runtime_error("RequestHandler::game_timer_start::Error::game_timer already active");
        }

        if (timer_) {
            try
            {
                timer_->SetAlignPeriod(period);
                timer_->Start();
                timer_enable_ = true;
            }
            catch (const std::exception& e)
            {
                throw std::runtime_error("RequestHandler::game_timer_start::Error::" + std::string(e.what()));
            }
        }

        return *this;
    }

    // выключает выполнение автотаймера, выкидывает исключение, если таймер уже выключен
    RequestHandler& RequestHandler::StopGameTimer() {
        if (!timer_enable_) {
            throw std::runtime_error("RequestHandler::game_timer_stop::Error::game_timer already stopped");
        }

        timer_->Stop(); 
        timer_enable_ = false;
        return *this;
    }

    // выполняет запись данных игрового сервера
    RequestHandler& RequestHandler::SerializeGameData() {
        serializer_->SerializeGameData();
        return *this;
    }

    // выполняет восстановленние данных игрового сервера
    RequestHandler& RequestHandler::DeserializeGameData() {
        serializer_->DeserializeGameData();
        return *this;
    }

    // метод настройки игрового таймера, генерирует команды для обработки
    RequestHandler& RequestHandler::TimerConfigurationPipeline() {

        // если задано время автообновления состояния
        if (arguments_.game_timer_launch && !arguments_.game_timer_period.empty()) {

            // запускаем таймер обновления игровых состояний с периодом выравнивания - обновления состояния
            timer_ = std::make_shared<time::TimeHandler>(api_strand_,
                std::chrono::milliseconds(std::stoi(arguments_.game_timer_period)));

            // конфигурируем метод обновления состояния игровых сессий
            auto state_update = std::make_shared<time::OnTimeCommand
                <std::chrono::milliseconds>>(
                    [this](std::chrono::milliseconds delta) {
                        this->game_->UpdateGameSessions(static_cast<int>(delta.count()));
                    },
                    std::chrono::milliseconds(std::stoi(arguments_.game_timer_period)));
            // загружаем метод обновления состояния игровых сессий
            timer_->AddCommand(arguments_.game_timer_period, std::move(state_update));

            // если задан путь к сохранениям и задано время автосохранения
            if (arguments_.game_autosave && !arguments_.save_state_period.empty()) {
                // конфигурируем метод сериализации игровых состояний
                auto serialization = std::make_shared<time::OnTimeCommand<void*>>(
                    [this](void*) { this->SerializeGameData(); }, (void*) nullptr
                );
                // загружаем метод сериализации игровых состояний
                timer_->AddCommand(arguments_.save_state_period, std::move(serialization));
            }

            timer_->Start();                              // стартуем обработчик
            timer_enable_ = true;                         // поднинмаем флаг
        }

        return *this;
    }

    // базовая функция активации всех элементов вызываемая в конструкторе по переданным параметрам
    RequestHandler& RequestHandler::ConfigurationPipeline() {

        try
        {
            // загружаем настройки игровой модели
            game_ = std::make_shared<game::GameHandler>(arguments_.config_json_path);

            // задаём игровой обработчик в сериализатор
            serializer_ = std::make_shared<game::SerialHandler>(game_);

            // если при старте указан флаг сериализации, то должно восстановить данные по указанному пути
            if (arguments_.game_autosave) {
                // назначаем путь к сохранению и сохраненным данным и выполняем восстановление
                serializer_->SetBackupFilePath(arguments_.state_file_path).DeserializeGameData();
            }

            // устанавливаем флаг рандомной позиции игроков на старте
            game_->SetRandomStartPosition(arguments_.randomize_spawn_points);

            // загружаем статические данные в менеджер файлов
            resource_ = std::make_shared<res::ResourceHandler>(arguments_.static_content_path);
            //// устанавливаем флаг запуска тест-системы
            //test_enable_ = arguments_.test_frame_launch;

            return TimerConfigurationPipeline();
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error("RequestHandler::configuration_pipeline::Error::class modules construction is fail::" + std::string(e.what()));
        }
    }

    // возвращает запрошенный документ
    Response RequestHandler::StaticFileBodyResponse(StringRequest&& req, const resource_handler::ResourcePtr resource) {
        FileResponse response(http::status::ok, req.version());

        // в огромном свиче выбираем тип контента
        switch (resource->_type)
        {
        case resource_handler::ResourceType::html:
        case resource_handler::ResourceType::htm:
            response.set(http::field::content_type, ContentType::TEXT_HTML);
            break;

        case resource_handler::ResourceType::txt:
        case resource_handler::ResourceType::folder:
            response.set(http::field::content_type, ContentType::TEXT_TXT);
            break;

        case resource_handler::ResourceType::css:
            response.set(http::field::content_type, ContentType::TEXT_CSS);
            break;

        case resource_handler::ResourceType::js:
            response.set(http::field::content_type, ContentType::TEXT_JS);
            break;

        case resource_handler::ResourceType::json:
            response.set(http::field::content_type, ContentType::APP_JSON);
            break;

        case resource_handler::ResourceType::xml:
            response.set(http::field::content_type, ContentType::APP_XML);
            break;

        case resource_handler::ResourceType::png:
            response.set(http::field::content_type, ContentType::IMAGE_PNG);
            break;

        case resource_handler::ResourceType::jpg:
        case resource_handler::ResourceType::jpe:
        case resource_handler::ResourceType::jpeg:
            response.set(http::field::content_type, ContentType::IMAGE_JPEG);
            break;

        case resource_handler::ResourceType::gif:
            response.set(http::field::content_type, ContentType::IMAGE_GIF);
            break;

        case resource_handler::ResourceType::bmp:
            response.set(http::field::content_type, ContentType::IMAGE_BMP);
            break;

        case resource_handler::ResourceType::ico:
            response.set(http::field::content_type, ContentType::IMAGE_ICO);
            break;

        case resource_handler::ResourceType::tif:
        case resource_handler::ResourceType::tiff:
            response.set(http::field::content_type, ContentType::IMAGE_TIFF);
            break;

        case resource_handler::ResourceType::svg:
        case resource_handler::ResourceType::svgz:
            response.set(http::field::content_type, ContentType::IMAGE_SVG);
            break;

        case resource_handler::ResourceType::mp3:
            response.set(http::field::content_type, ContentType::AUDIO_MPEG);
            break;

        case resource_handler::ResourceType::unknow:
            response.set(http::field::content_type, ContentType::APP_UNKNOW);
            break;

        default:
            break;
        }

        // открываем файл
        http::file_body::value_type file;
        if (sys::error_code ec; file.open(resource->_path.data(), beast::file_mode::read, ec), ec) {
            std::cerr << "Failed to open file "sv << resource->_path << std::endl;
            assert(false);
        }

        response.body() = std::move(file);
        response.prepare_payload();

        return response;
    }

    // возвращает index.html основной страницы
    Response RequestHandler::StaticRootIndexResponse(StringRequest&& req) {
        FileResponse response(http::status::ok, req.version());
        response.set(http::field::content_type, ContentType::TEXT_HTML);

        std::string path_line = std::string(resource_->GetRootPath()) + "index.html";

        http::file_body::value_type file;
        if (sys::error_code ec; file.open(path_line.data(), beast::file_mode::read, ec), ec) {
            std::cerr << "Failed to open file "sv << path_line << std::endl;
            assert(false);
        }

        response.body() = std::move(file);
        response.prepare_payload();

        return response;
    }

    // базовый ответ 404 - not found
    Response RequestHandler::StaticNotFoundResponse(StringRequest&& req) {
        StringResponse response(http::status::not_found, req.version());
        response.set(http::field::content_type, ContentType::TEXT_TXT);
        response.body() = json_detail::GetErrorString("NotFound"sv, "file not found"sv);

        return response;
    }

    // базовый ответ 400 - bad request
    Response RequestHandler::StaticBadRequestResponse(StringRequest&& req) {
        StringResponse response(http::status::bad_request, req.version());
        response.set(http::field::content_type, ContentType::TEXT_TXT);
        response.body() = json_detail::GetErrorString("BadRequest"sv, "access denied"sv);

        return response;
    }

    // возвращает ответ на неверный запрос к дебаговым модулям
    Response RequestHandler::DebugCommonFailResponse(http_handler::StringRequest&& req, http::status status,
        std::string_view code, std::string_view message, [[maybe_unused]] std::string_view allow) {

        http_handler::StringResponse response(status, req.version());
        response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
        response.set(http::field::cache_control, "no-cache");

        if (!allow.empty()) {
            // если указано что разрешено, то заполняем этот хеддер
            response.set(http::field::allow, allow);
        }

        // заполняем тушку ответа с помощью жисонского метода
        std::string body_str = json_detail::GetErrorString(code, message);
        response.set(http::field::content_length, std::to_string(body_str.size()));
        response.body() = body_str;
        response.prepare_payload();

        return response;

    }

    // возвращает ответ на запрос по удалению всех игровых сессий из обработчика
    Response RequestHandler::DebugSessionsResetResponse(StringRequest&& req) {
        try
        {
            game_->ResetGameSessions();        // вызываем удаление всех данных в игровом обработчике

            http_handler::StringResponse response(http::status::ok, req.version());
            response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
            response.set(http::field::cache_control, "no-cache");

            // заполняем тушку ответа с помощью жисонского метода
            std::string body_str = json_detail::GetDebugArgument("gameDataStatus", "dataIsClear");
            response.set(http::field::content_length, std::to_string(body_str.size()));
            response.body() = body_str;
            response.prepare_payload();

            return response;
        }
        catch (const std::exception& e)
        {
            return DebugCommonFailResponse(std::move(req), http::status::bad_request,
                "invalidOperation", "RequestHandler::debug_sessions_reset_response::Exception::" + std::string(e.what()), ""s);
        }
    }

    // возвращает ответ на запрос по установке флага случайного стартового расположения
    Response RequestHandler::DebugStartPositionResponse(StringRequest&& req) {
        try
        {
            // парсим тело запроса, все исключения в процессе будем ловить в catch_блоке
            boost::json::value req_data = json_detail::ParseTextToJSON(req.body());
            std::string flag = req_data.at("randomPosition").as_string().data();
            bool randomPosition = false;

            if (flag == "true") {
                randomPosition = true;
            }
            // назначаем флаг размещения игроков в случайном месте
            game_->SetRandomStartPosition(randomPosition);

            http_handler::StringResponse response(http::status::ok, req.version());
            response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
            response.set(http::field::cache_control, "no-cache");

            // заполняем тушку ответа с помощью жисонского метода
            std::string body_str;
            if (randomPosition) {
                body_str = json_detail::GetDebugArgument("startRandomPosition", "true");
            }
            else {
                body_str = json_detail::GetDebugArgument("startRandomPosition", "false");
            }

            response.set(http::field::content_length, std::to_string(body_str.size()));
            response.body() = body_str;
            response.prepare_payload();

            return response;
        }
        catch (const std::exception& e)
        {
            return DebugCommonFailResponse(std::move(req), http::status::bad_request,
                "invalidOperation", "RequestHandler::debug_start_position_response::Exception::" + std::string(e.what()), ""s);
        }
    }

    // возвращает ответ на запрос по установке флага случайного стартового расположения из конфига
    Response RequestHandler::DebugDefaultPositionResponse(StringRequest&& req) {
        try
        {
            // назначаем флаг размещения игроков в случайном месте
            game_->SetRandomStartPosition(arguments_.randomize_spawn_points);

            http_handler::StringResponse response(http::status::ok, req.version());
            response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
            response.set(http::field::cache_control, "no-cache");

            // заполняем тушку ответа с помощью жисонского метода
            std::string body_str = json_detail::GetDebugArgument("startRandomPosition", "default");

            response.set(http::field::content_length, std::to_string(body_str.size()));
            response.body() = body_str;
            response.prepare_payload();

            return response;
        }
        catch (const std::exception& e)
        {
            return DebugCommonFailResponse(std::move(req), http::status::bad_request,
                "invalidOperation", "RequestHandler::debug_start_position_response::Exception::" + std::string(e.what()), ""s);
        }
    }

    // возвращает ответ на отчёт о завершении работы тестовой системы
    Response RequestHandler::DebugUnitTestsEndResponse(StringRequest&& req) {
        try
        {
            //test_enable_ = false;                        // первым делом снимаем флаг о работе тест системы
            //arguments_.test_frame_launch = false;        // также снимаем флаг и в переданном конфиге на всякий случай
            // таймер может быть только в том случае если изначально был сконфигурирован, включаем его
            if (timer_ && !timer_enable_) {

                timer_->Start();               // запускаем таймер автообновления
                timer_enable_ = true;                    // поднимаем флаг включения таймера
            }

            // также устанавливаем флаг рандомного старта персонажей по переданному конфигу малоли его тест система изменила
            game_->SetRandomStartPosition(arguments_.randomize_spawn_points);

            http_handler::StringResponse response(http::status::ok, req.version());
            response.set(http::field::content_type, http_handler::ContentType::APP_JSON);
            response.set(http::field::cache_control, "no-cache");

            // заполняем тушку ответа с помощью жисонского метода
            std::string body_str = json_detail::GetDebugArgument("gameDataStatus", "tfEndpointIsClose");
            response.set(http::field::content_length, std::to_string(body_str.size()));
            response.body() = body_str;
            response.prepare_payload();

            return response;
        }
        catch (const std::exception& e)
        {
            return DebugCommonFailResponse(std::move(req), http::status::bad_request,
                "invalidOperation", "RequestHandler::debug_test_frame_end_response::Exception::" + std::string(e.what()), ""s);
        }
    }

    // обработчик для запросов к статическим данным
    Response RequestHandler::HandleStaticRequest(StringRequest&& req) {
        // если у нас просто переход по адресу, или с указанием странички index.html
        if (req.target() == "/"sv || req.target() == "/index.html"sv) {
            return StaticRootIndexResponse(std::move(req));
        }

        // для начала сделаем репарсинг полученной строки запроса, отсекаем первый слеш за ненадобностью
        std::string reparse_line = ParseRequestTarget(req.target().begin() + 1, req.target().end());

        // ищем выход за пределы рута, в левом запросе будет так или иначе в начале присутствовать замаскированный обратный слеш
        if (reparse_line.find("..%5C") != std::string::npos
            || reparse_line.find("/../") != std::string::npos) {
            return StaticBadRequestResponse(std::move(req));
        }

        // если на краю слеш - то будем искать файл index.html в крайнем каталоге
        if (reparse_line.size() != 0 && *(reparse_line.end() - 1) == '/') {
            reparse_line += "index.html";
        }

        // берем путь до основной директории
        std::string root_path = std::string(resource_->GetRootPath());

        // если запрос валидный, то проверяем наличие файла по пути
        if (resource_handler::ResourcePtr resource = resource_->GetItem(fs::path(root_path + reparse_line)); !resource) {
            // если упоминания о файле во внутренних каталогах нет отвечаем, что отдать нечего
            return StaticNotFoundResponse(std::move(req));
        }
        else {
            return StaticFileBodyResponse(std::move(req), resource);
        }
    }

    // обработчик запросов для к api-игрового сервера
    Response RequestHandler::HandleApiRequest(StringRequest&& req, std::string_view api_request_line) {

        if (api_request_line.size() == 0) {
            // если предается голое "api", то вызываем ответ по ошибке
            return DebugCommonFailResponse(std::move(req), http::status::bad_request, "badRequest"sv, "Bad request"sv, ""sv);
        }

        if (api_request_line == "/v1/maps"sv) {
            // выводим список доступных карт
            return game_->MapsListResponse(std::move(req));
        }

        if (api_request_line == "/v1/game/join"sv) {
            // обрабатываем запрос по присоединению к игре
            return game_->JoinGameResponse(std::move(req));
        }

        if (api_request_line == "/v1/game/players"sv) {
            // обрабатываем запрос по выдаче информации о подключенных игроках к сессии
            return game_->PlayersListResponse(std::move(req));
        }

        if (api_request_line == "/v1/game/tick"sv) {
            if (timer_enable_) {
                // если активирован таймер, то кидаем отбойник на подобный запрос
                return DebugCommonFailResponse(std::move(req), http::status::bad_request, "badRequest"sv, "Invalid endpoint"sv, ""sv);
            }
            // обрабатываем запрос по изменению состояния игровой сессии со временем
            return HandleSpecialCoopMethods(std::move(this->SerializeGameData()),
                std::move(game_->SessionsUpdateResponse(std::move(req))));

            /*return HandleSpecialCoopMethods(
                std::move(game_->SessionsUpdateResponse(std::move(req))),
                std::move(this->SerializeGameData()));*/
            //return game_->SessionsUpdateResponse(std::move(req));
        }

        if (api_request_line == "/v1/game/state"sv) {
            // обрабатываем запрос по получению инфы о игровом состоянии персонажей
            return game_->GameStateResponse(std::move(req));
        }

        if (api_request_line == "/v1/game/player/action"sv) {
            // обрабатываем запрос по совершению действий персонажем
            return game_->PlayerActionResponse(std::move(req));
        }

        // важный момент парсинга - блок сработает только если строка больше 9 символов и первые слова "/v1/maps/"
        // по идее сюда можно добавлять разные элементы, если их будет много то имеет смысл сделать специализированный парсер
        if (api_request_line.size() >= 9 && std::string{ api_request_line.begin(),  api_request_line.begin() + 9 } == "/v1/maps/"sv) {
            // отправляемся на поиски запрошенной карты
            return game_->FindMapResponse(std::move(req),
                { api_request_line.begin() + 9, api_request_line.end() });
        }

        // на крайний случай просто скажем, что запрос плохой
        return DebugCommonFailResponse(std::move(req), http::status::bad_request, "badRequest"sv, "Bad request"sv, ""sv);
    }

    // обработчик для конфигурационных запросов от тестовой системы
    Response RequestHandler::HandleTestRequest(StringRequest&& req, std::string_view debug_request_line) {

        if (debug_request_line.size() == 0) {
            // если предается голое "api", то вызываем ответ по ошибке
            return DebugCommonFailResponse(std::move(req), http::status::bad_request,  "badRequest"sv, "Bad request"sv, ""sv);
        }

        if (debug_request_line == "/reset"sv) {
            // обрабатываем запрос на сброс и удаление всех игровых сессий, нужно для тестов
            // в случае успешной авторизации, лямбда вызовет нужный обработчик
            return DebugAuthorizationImpl(std::move(req),
                [this](http_handler::StringRequest&& req) {
                    return this->DebugSessionsResetResponse(std::move(req));
                });
        }

        if (debug_request_line == "/position"sv) {
            // обрабатываем запрос на установку флага случайной позиции на старте
            // в случае успешной авторизации, лямбда вызовет нужный обработчик
            return DebugAuthorizationImpl(std::move(req),
                [this](http_handler::StringRequest&& req) {
                    return this->DebugStartPositionResponse(std::move(req));
                });
        }

        if (debug_request_line == "/position/default"sv) {
            // обрабатываем запрос на установку флага случайной позиции на старте
            // в случае успешной авторизации, лямбда вызовет нужный обработчик
            return DebugAuthorizationImpl(std::move(req),
                [this](http_handler::StringRequest&& req) {
                    return this->DebugDefaultPositionResponse(std::move(req));
                });
        }

        if (debug_request_line == "/test_end"sv) {
            // обрабатываем отчёт о завершении тестов
            // в случае успешной авторизации, лямбда вызовет нужный обработчик
            return DebugAuthorizationImpl(std::move(req),
                [this](http_handler::StringRequest&& req) {
                    return this->DebugUnitTestsEndResponse(std::move(req));
                });
        }

        return DebugCommonFailResponse(std::move(req), http::status::bad_request, "badRequest"sv, "Bad request"sv, ""sv);
    }


}  // namespace http_handler