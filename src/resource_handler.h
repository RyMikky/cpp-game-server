#pragma once

#include "domain.h"

namespace resource_handler {

	namespace fs = std::filesystem;

	class ResourceHandler {
	public:

		ResourceHandler() = delete;
		explicit ResourceHandler(const char* main_root);
		explicit ResourceHandler(const fs::path& file_path);
		ResourceHandler(const ResourceHandler&) = delete;
		ResourceHandler(ResourceHandler&&) = default;

		ResourceHandler& operator=(const ResourceHandler&) = delete;
		ResourceHandler& operator=(ResourceHandler&&) = default;
		// добавляет запись о документе в базовый вектор
		void AddItem(ResourceItem&& item);

		std::string_view GetRootName() const {
			return _resource_base[0]._name;
		}
		std::string_view GetRootPath() const {
			return _resource_base[0]._path;
		}
		// возвращает указатель на данные по имени файла
		ResourcePtr GetItem(std::string_view file_name) const;
		// возвращает указатель на данные по пути к файлу
		ResourcePtr GetItem(const fs::path& file_path) const;

		// подтверждает наличие файла по имени
		bool Count(std::string_view file_name) const;
		// подтверждает наличие файла по пути
		bool Count(fs::path file_path) const;

	private:
		std::deque<ResourceItem> _resource_base;

		FileIndexNameToPath _name_to_data_ptr;
		FileIndexPathToName _path_to_data_ptr;

		void SetRoot(const fs::path& file_path);
		// парсит расширение файла и возвращает тип
		ResourceType ParseFileExtension(std::string_view label);
		// рекурентный проход по всем вложенным каталогам
		void ProcessRootTree(const fs::path& file_path);
	};

	namespace detail {
		// базовый препроцессор-индексатор файлов
		resource_handler::ResourceHandler LoadFiles(const char* file_path);

	} // namespace detail

} // namespace resourse_handler