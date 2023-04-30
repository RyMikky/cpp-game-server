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

		void add_resource_item(ResourceItem&& item);                                // добавляет запись о документе в базовый вектор

		std::string_view get_root_directory_name() const {
			return _resource_base[0]._name;
		}
		std::string_view get_root_directory_path() const {
			return _resource_base[0]._path;
		}

		ResourcePtr get_resource_item(std::string_view file_name) const;            // возвращает указатель на данные по имени файла
		ResourcePtr get_resource_item(const fs::path& file_path) const;             // возвращает указатель на данные по пути к файлу

		bool in_database(std::string_view file_name) const;                         // возвращает подтверждение наличия файла
		bool in_database(fs::path file_path) const;                                 // возвращает подтверждение наличия файла

	private:

		std::deque<ResourceItem> _resource_base;

		FileIndexNameToPath _name_to_data_ptr;
		FileIndexPathToName _path_to_data_ptr;

		void set_root_directory(const fs::path& file_path);
		ResourceType parse_file_extension(std::string_view label);                  // парсит расширение файла и возвращает тип
		void resource_index_recourse(const fs::path& file_path);                    // рекурентный проход по всем вложенным каталогам
	};

	namespace detail {

		resource_handler::ResourceHandler LoadFiles(const char* file_path);         // базовый препроцессор-индексатор файлов

	} // namespace detail

} // namespace resourse_handler