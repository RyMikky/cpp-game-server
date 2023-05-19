#include "serialization_handler.h"

namespace game_handler {

	// возвращает игрока по индексу
	const SerializedPlayer& SerializedSession::GetPlayerByIndex(size_t idx) const {
		if (idx < players_.size()) {
			return players_[idx];
		}
		throw std::out_of_range("SerializedSession::GetPlayerByIndex::Error::Index is out of range");
	}

	// возвращает единицу лута по индексу
	const SerializedLoot& SerializedSession::GetLootByIndex(size_t idx) const {
		if (idx < loots_.size()) {
			return loots_[idx];
		}
		throw std::out_of_range("SerializedSession::GetLootByIndex::Error::Index is out of range");
	}

	// возвращает вектор игроков в сессии
	std::vector<SerializedPlayer> SerializedSession::MakePlayersVector(const SessionPlayers& players) {
		try
		{
			std::vector<SerializedPlayer> result = {};
			for (const auto& [token, player] : players) {
				result.push_back(SerializedPlayer{ player });
			}
			players_count_ = players.size();
			return result;
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error("SerializedSession::MakePlayersVector::Error::" + std::string(e.what()));
		}
	}

	// возвращает вектор лута в сессии
	std::vector<SerializedLoot> SerializedSession::MakeLootsVector(const SessionLoots& loots) {
		try
		{
			std::vector<SerializedLoot> result = {};
			for (const auto& [id, loot] : loots) {
				result.push_back(SerializedLoot{ loot });
			}
			loots_count_ = loots.size();
			return result;
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error("SerializedSession::MakeLootsVector::Error::" + std::string(e.what()));
		}
	}

	// назначает игровой обработчик который подлежит сериализации
	SerialHandler& SerialHandler::SetGameHandler(std::shared_ptr<GameHandler> game) {
		game_ = game;
		return *this;
	}

	// назначает путь к файлу сохранения
	SerialHandler& SerialHandler::SetBackupFilePath(std::string path) {
		main_path_ = fs::path(path);
		temp_path_ = fs::path(path + __BACKUP_TEMP_DATA_FILE_NAME__);
		return *this;
	}

	// назначает путь к файлу сохранения
	SerialHandler& SerialHandler::SetBackupFilePath(const fs::path& path) {
		main_path_ = path;
		return *this;
	}

	// выполняет сериализацию, запись данных в бекап
	SerialHandler& SerialHandler::SerializeGameData() {
		return SerializeGameData(temp_path_);
	}

	// выполняет сериализацию, запись данных в бекап
	SerialHandler& SerialHandler::SerializeGameData(const fs::path& path) {

		std::fstream stream;      // создаём поток вывода в файл
		// продолжаем если файл открыт иначе ничего не получится
		if (OpenBackupOutputFile(stream, path)) {

			/* 1. Подготавливаем массив с игровыми сессиями */
			sessions_ = MakeSessionsVector(game_->GetSessions());

			/* 2. Создаём коробку архив */
			boost::archive::binary_oarchive ar{stream};

			/* 3. Записываем количество игровых сессий */
			ar << sessions_count_;

			/* 4. Запускаем цикл записи игровых сессий */
			for (const auto& session : sessions_) {
				ar << session;
			}

			/* 5. Закрываем файл записи и выходим */
			CloseBackupFile(stream);

			/* 6. Переименовываем временный файл в нормальный */
			fs::rename(temp_path_, main_path_);
		}

		return *this;
	}

	// выполняет восстановление данных из бекапа
	SerialHandler& SerialHandler::DeserializeGameData() {
		return DeserializeGameData(main_path_);
	}

	// выполняет восстановление данных из бекапа
	SerialHandler& SerialHandler::DeserializeGameData(const fs::path& path) {

		std::fstream stream;      // создаём поток ввода из файла
		// продолжаем если файл открыт иначе ничего не получится
		if (OpenBackupInputFile(stream, path)) {
		
			/* 1. Создаём коробку архив */
			boost::archive::binary_iarchive ar{stream};

			/* 2. очищаем список сериализованных сессий */
			sessions_.clear();;

			try
			{
				/* 3. Читаем количество игровых сессий в записи */
				ar >> sessions_count_;

				for (size_t i = 0; i != sessions_count_; ++i) {
					SerializedSession session;
					ar >> session;
					sessions_.push_back(session);
				}

				/* 4. Закрываем файл записи */
				CloseBackupFile(stream);

				/* 5. Загружаем данные в игровой обработчик */
				return UploadBackupData();
			}
			catch (const std::exception&)
			{
				throw std::runtime_error("SerializationHandler::DeserializeGameData::Error::On read backup file");
			}
		}

		return *this;
	}

	// загружает ранее сохраненные данные в игровой обработчик
	SerialHandler& SerialHandler::UploadBackupData() {

		// выполнение происходит только если вектор сессий не пуст
		if (!sessions_.empty()) {

			/* 1. Выполняем сброс данных игрового сервера */
			game_->ResetGameSessions();

			/* 2. Выполняем восстановление игровых сессий */
			for (const auto& session : sessions_) {

				/* 3. Восстанавливаем игровую сессию, и получаем контекст восстановлеия */
				auto& context = game_->RestoreGameSession(session.GetId(), session.GetMapId());

				/* 4. идем по списку игроков */
				for (size_t p = 0; p != session.GetPlayersCount(); ++p) {

					/* 5. получаем игрока и отправляем данные в контекст */
					context.RestoreGamePlayer(session.GetPlayerByIndex(p));
				}

				/* 6. идем по списку игроков */
				for (size_t l = 0; l != session.GetLootCount(); ++l) {

					/* 7. получаем игрока и отправляем данные в контекст */
					context.RestoreGameLoot(session.GetLootByIndex(l));
				}
			}
		}

		return *this;
	}

	// создаёт вектор с запущенными в игре игровыми сессиями
	std::vector<SerializedSession> SerialHandler::MakeSessionsVector(const GameSessionList& sessions) {
		try
		{
			std::vector<SerializedSession> result = {};
			for (const auto& [id, session] : sessions) {
				result.push_back(SerializedSession{ *session.get()});
			}
			sessions_count_ = sessions.size();
			return result;
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error("SerializationHandler::MakeSessionsVector::Error::" + std::string(e.what()));
		}
	}

	// открывает файл для записи
	bool SerialHandler::OpenBackupOutputFile(std::fstream& stream, fs::path file) {
		try
		{
			stream.open(file, std::ios::out | std::ios_base::binary | std::ios::trunc);
			return stream.is_open();
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error("SerializationHandler::OpenBackupOutputFile::Error::" + std::string(e.what()));
		}
	}

	// открывает файл для чтения
	bool SerialHandler::OpenBackupInputFile(std::fstream& stream, fs::path file) {
		try
		{
			stream.open(file, std::ios::in | std::ios_base::binary);
			return stream.is_open();
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error("SerializationHandler::OpenBackupInputFile::Error::" + std::string(e.what()));
		}
	}

	// закрывает файл записи данных
	bool SerialHandler::CloseBackupFile(std::fstream& stream) {
		try
		{
			stream.close();
			return !stream.is_open();
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error("SerializationHandler::CloseBackupFile::Error::" + std::string(e.what()));
		}
	}

} // namespace game_handler