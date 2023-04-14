#pragma once
//
//#include <chrono>
//#include <iomanip>
//#include <fstream>
//#include <sstream>
//#include <string>
//#include <string_view>
//#include <optional>
//#include <mutex>
//#include <thread>
//
//#include <filesystem>
//
//#include <boost/log/trivial.hpp>
//
//using namespace std::literals;
//
//#define LOG(...) Logger::GetInstance().Log(__VA_ARGS__)
//
//// вариант строки для Linux
//// static std::filesystem::path __LOG_DIR__ = "/var/log";
//// вариант строки для Windows
//static std::filesystem::path __LOG_DIR__ = "../log";
//
//class Logger {
//    auto GetTime() const {
//        if (manual_ts_) {
//            return *manual_ts_;
//        }
//
//        return std::chrono::system_clock::now();
//    }
//
//    auto GetTimeStamp() const {
//        const auto now = GetTime();
//        const auto t_c = std::chrono::system_clock::to_time_t(now);
//        return std::put_time(std::localtime(&t_c), "%F %T");
//    }
//
//    // Для имени файла возьмите дату с форматом "%Y_%m_%d"
//    std::string GetFileTimeStamp() const {
//        const auto now = GetTime();
//        const auto t_c = std::chrono::system_clock::to_time_t(now);
//
//        char timeBuffer[std::size("yyyy_mm_dd")];
//        //const std::tm tm = *std::localtime(&t_c);
//        const std::tm tm = *std::gmtime(&t_c);
//
//
//        std::strftime(timeBuffer, std::size(timeBuffer), "%Y_%m_%d", &tm);
//        
//        return timeBuffer;
//    }
//
//    Logger() = default;
//    Logger(const Logger&) = delete;
//    Logger& operator=(const Logger&) = delete;
//    Logger(Logger&&) = delete;
//    Logger& operator=(Logger&&) = delete;
//
//    
//
//public:
//    static Logger& GetInstance() {
//        static Logger obj;
//        return obj;
//    }
//
//    // Выведите в поток все аргументы.
//    template<class... Ts>
//    void Log(const Ts&... args);
//
//    // Установите manual_ts_. Учтите, что эта операция может выполняться
//    // параллельно с выводом в поток, вам нужно предусмотреть 
//    // синхронизацию.
//    void SetTimestamp(std::chrono::system_clock::time_point ts) {
//        std::lock_guard<std::mutex> lock(set_m_);
//        manual_ts_ = ts;
//    }
//
//private:
//    std::optional<std::chrono::system_clock::time_point> manual_ts_;
//
//    std::mutex log_m_;
//    std::mutex set_m_;
//};
//
//// Выведите в поток все аргументы.
//template<class... Ts>
//void Logger::Log(const Ts&... args) {
//    std::lock_guard<std::mutex> lock(log_m_);
//
//    //std::filesystem::create_directory("logs");
//
//    //std::ofstream log_file{ /*"logs/" + */"sample_log_"s + GetFileTimeStamp() + ".log"s, std::ios::app };
//
//    // Открываемый файл
//    std::string file_name = "sample_log_"s + GetFileTimeStamp() + ".log"s;
//
//    // Результирующий путь к отрываемому файлу лога
//    std::filesystem::path log_file_path = __LOG_DIR__ / file_name;
//
//    // Открываем поток для записи
//    std::ofstream log_file(log_file_path, std::ios::app);
//
//    std::time_t t = std::chrono::system_clock::to_time_t(GetTime());
//    std::tm* tm_ptr = std::gmtime(&t);
//
//    log_file << std::put_time(tm_ptr, "%Y-%m-%d %H:%M:%S") << ": "sv;
//    ((log_file << args), ...) << "\n";
//}
