#include "serialization_handler.h"

namespace game_handler {

	// ���������� ������ �� �������
	const SerializedPlayer& SerializedSession::GetPlayerByIndex(size_t idx) const {
		if (idx < players_.size()) {
			return players_[idx];
		}
		throw std::out_of_range("SerializedSession::GetPlayerByIndex::Error::Index is out of range");
	}

	// ���������� ������� ���� �� �������
	const SerializedLoot& SerializedSession::GetLootByIndex(size_t idx) const {
		if (idx < loots_.size()) {
			return loots_[idx];
		}
		throw std::out_of_range("SerializedSession::GetLootByIndex::Error::Index is out of range");
	}

	// ���������� ������ ������� � ������
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

	// ���������� ������ ���� � ������
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

	// ��������� ������� ���������� ������� �������� ������������
	SerialHandler& SerialHandler::SetGameHandler(std::shared_ptr<GameHandler> game) {
		game_ = game;
		return *this;
	}

	// ��������� ���� � ����� ����������
	SerialHandler& SerialHandler::SetBackupFilePath(std::string path) {
		main_path_ = fs::path(path);
		temp_path_ = fs::path(path + __BACKUP_TEMP_DATA_FILE_NAME__);
		return *this;
	}

	// ��������� ���� � ����� ����������
	SerialHandler& SerialHandler::SetBackupFilePath(const fs::path& path) {
		main_path_ = path;
		return *this;
	}

	// ��������� ������������, ������ ������ � �����
	SerialHandler& SerialHandler::SerializeGameData() {
		return SerializeGameData(temp_path_);
	}

	// ��������� ������������, ������ ������ � �����
	SerialHandler& SerialHandler::SerializeGameData(const fs::path& path) {

		std::fstream stream;      // ������ ����� ������ � ����
		// ���������� ���� ���� ������ ����� ������ �� ���������
		if (OpenBackupOutputFile(stream, path)) {

			/* 1. �������������� ������ � �������� �������� */
			sessions_ = MakeSessionsVector(game_->GetSessions());

			/* 2. ������ ������� ����� */
			boost::archive::binary_oarchive ar{stream};

			/* 3. ���������� ���������� ������� ������ */
			ar << sessions_count_;

			/* 4. ��������� ���� ������ ������� ������ */
			for (const auto& session : sessions_) {
				ar << session;
			}

			/* 5. ��������� ���� ������ � ������� */
			CloseBackupFile(stream);

			/* 6. ��������������� ��������� ���� � ���������� */
			fs::rename(temp_path_, main_path_);
		}

		return *this;
	}

	// ��������� �������������� ������ �� ������
	SerialHandler& SerialHandler::DeserializeGameData() {
		return DeserializeGameData(main_path_);
	}

	// ��������� �������������� ������ �� ������
	SerialHandler& SerialHandler::DeserializeGameData(const fs::path& path) {

		std::fstream stream;      // ������ ����� ����� �� �����
		// ���������� ���� ���� ������ ����� ������ �� ���������
		if (OpenBackupInputFile(stream, path)) {
		
			/* 1. ������ ������� ����� */
			boost::archive::binary_iarchive ar{stream};

			/* 2. ������� ������ ��������������� ������ */
			sessions_.clear();;

			try
			{
				/* 3. ������ ���������� ������� ������ � ������ */
				ar >> sessions_count_;

				for (size_t i = 0; i != sessions_count_; ++i) {
					SerializedSession session;
					ar >> session;
					sessions_.push_back(session);
				}

				/* 4. ��������� ���� ������ */
				CloseBackupFile(stream);

				/* 5. ��������� ������ � ������� ���������� */
				return UploadBackupData();
			}
			catch (const std::exception&)
			{
				throw std::runtime_error("SerializationHandler::DeserializeGameData::Error::On read backup file");
			}
		}

		return *this;
	}

	// ��������� ����� ����������� ������ � ������� ����������
	SerialHandler& SerialHandler::UploadBackupData() {

		// ���������� ���������� ������ ���� ������ ������ �� ����
		if (!sessions_.empty()) {

			/* 1. ��������� ����� ������ �������� ������� */
			game_->ResetGameSessions();

			/* 2. ��������� �������������� ������� ������ */
			for (const auto& session : sessions_) {

				/* 3. ��������������� ������� ������, � �������� �������� ������������� */
				auto& context = game_->RestoreGameSession(session.GetId(), session.GetMapId());

				/* 4. ���� �� ������ ������� */
				for (size_t p = 0; p != session.GetPlayersCount(); ++p) {

					/* 5. �������� ������ � ���������� ������ � �������� */
					context.RestoreGamePlayer(session.GetPlayerByIndex(p));
				}

				/* 6. ���� �� ������ ������� */
				for (size_t l = 0; l != session.GetLootCount(); ++l) {

					/* 7. �������� ������ � ���������� ������ � �������� */
					context.RestoreGameLoot(session.GetLootByIndex(l));
				}
			}
		}

		return *this;
	}

	// ������ ������ � ����������� � ���� �������� ��������
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

	// ��������� ���� ��� ������
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

	// ��������� ���� ��� ������
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

	// ��������� ���� ������ ������
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