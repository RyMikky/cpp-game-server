#pragma once

#include <mutex>
#include <vector>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <filesystem>

#include "game_handler.h"
#include "logger_handler.h"

namespace game_handler {

	static const std::string __BACKUP_TEMP_DATA_FILE_NAME__ = ".temporaly";

	namespace fs = std::filesystem;
	
	// класс сериализации игровой сессии
	class SerializedSession {
	public:
		SerializedSession() = default;
		explicit SerializedSession(const GameSession& session) 
			: session_id_(session.GetId()), map_id_(*(session.GetMap()->GetId()))
			, players_(MakePlayersVector(session.GetPlayers()))
			, loots_(MakeLootsVector(session.GetLoots())){
		}

		template <typename Archive>
		void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
			ar& session_id_;
			ar& map_id_;
			ar& players_count_;
			ar& players_;
			ar& loots_count_;
			ar& loots_;
		}

		// возвращает идентификатор сессии
		size_t GetId() const {
			return session_id_;
		}
		// возвращает ID карты
		std::string GetMapId() const {
			return map_id_;
		}

		// возвращает количество игроков в сессии
		size_t GetPlayersCount() const {
			return players_count_;
		}
		// возвращает игрока по индексу
		const SerializedPlayer& GetPlayerByIndex(size_t) const;
		// возвращает массив с игроками
		const std::vector<SerializedPlayer>& GetPlayers() {
			return players_;
		}

		// возвращает действительное количество лута в инвентаре игрока
		size_t GetLootCount() const {
			return loots_count_;
		}
		// возвращает единицу лута по индексу
		const SerializedLoot& GetLootByIndex(size_t) const;
		// возвращает массив с лутом
		const std::vector<SerializedLoot>& GetLoots() {
			return loots_;
		}

	private:
		size_t session_id_ = 0;                             // идентификатор игровой сессии
		std::string map_id_ = "";                           // идентификатор игровой карты
		
		size_t players_count_ = 0;                          // количество игроков в сессии
		std::vector<SerializedPlayer> players_;             // вектор с игроками
		size_t loots_count_ = 0;                            // количество лута на карте
		std::vector<SerializedLoot> loots_;                 // вектор с игровым лутом

		// возвращает вектор игроков в сессии
		std::vector<SerializedPlayer> MakePlayersVector(const SessionPlayers&);
		// возвращает вектор лута в сессии
		std::vector<SerializedLoot> MakeLootsVector(const SessionLoots&);
	};

	/*
	* Основной обработчик сериализации данных, напрямую подключается к обработчику игры
	* получает данные и сохраняет их по указанному пути. 
	*/
	class SerialHandler {
	public:
		SerialHandler() = default;
		SerialHandler(std::shared_ptr<GameHandler> game)
			: game_(game) {
		}
		SerialHandler(const std::string& path, std::shared_ptr<GameHandler> game)
			: main_path_(path), temp_path_(path + __BACKUP_TEMP_DATA_FILE_NAME__), game_(game) {
		}
		SerialHandler(const fs::path& path, std::shared_ptr<GameHandler> game)
			: main_path_(path), temp_path_(path.generic_string() + __BACKUP_TEMP_DATA_FILE_NAME__), game_(game) {
		}

		// назначает игровой обработчик который подлежит сериализации
		SerialHandler& SetGameHandler(std::shared_ptr<GameHandler>);
		// назначает путь к файлу сохранения
		SerialHandler& SetBackupFilePath(std::string);
		// назначает путь к файлу сохранения
		SerialHandler& SetBackupFilePath(const fs::path&);
		
		// выполняет сериализацию, запись данных в бекап
		SerialHandler& SerializeGameData();
		// выполняет сериализацию, запись данных в бекап
		SerialHandler& SerializeGameData(const fs::path&);
		// выполняет восстановление данных из бекапа
		SerialHandler& DeserializeGameData();
		// выполняет восстановление данных из бекапа
		SerialHandler& DeserializeGameData(const fs::path&);

	private:
		fs::path main_path_;
		fs::path temp_path_;
		std::shared_ptr<GameHandler> game_;
		std::mutex mutex_;

		size_t sessions_count_ = 0;
		std::vector<SerializedSession> sessions_;

		// загружает ранее сохраненные данные в игровой обработчик
		SerialHandler& UploadBackupData();
		// создаёт вектор с запущенными в игре игровыми сессиями
		std::vector<SerializedSession> MakeSessionsVector(const GameSessionList&);

		// открывает файл для записи
		bool OpenBackupOutputFile(std::fstream&, fs::path);
		// открывает файл для чтения
		bool OpenBackupInputFile(std::fstream&, fs::path);
		// закрывает файл записи данных
		bool CloseBackupFile(std::fstream&);
	};


} // namespace game_handler