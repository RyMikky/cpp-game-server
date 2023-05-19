#include "domain.h"

namespace game_handler {

	// ���������� ������ ���� ��� ������ ������������
	std::vector<SerializedLoot> SerializedPlayer::MakeLootVector(const Player& player) {
		try
		{
			std::vector<SerializedLoot> result = {};
			for (const auto& item : player.GetBag()) {
				// �������� ���
				result.push_back(SerializedLoot{ *item.loot_ });
			}
			loots_count_ = player.GetBag().size();
			return result;
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error("SerializedPlayer::MakeLootVector::Error::" + std::string(e.what()));
		}
	}

	// ���������� ������� ���� �� �������
	const SerializedLoot& SerializedPlayer::GetLootByIndex(size_t idx) const {
		if (idx < loot_.size()) {
			return loot_[idx];
		}
		throw std::out_of_range("SerializedPlayer::GetLootByIndex::Error::Index is out of range");
	}

}