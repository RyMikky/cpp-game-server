#include "resource_handler.h"

#include <iostream>

namespace resource_handler {

	ResourceHandler::ResourceHandler(const char* main_root) {

		std::string path_line(main_root);
		// добавляем слеш, если его не поставили
		if (path_line.back() != '/' && path_line.back() != '\\') {
			path_line.push_back('/');
		}

		SetRoot(path_line);
	}

	ResourceHandler::ResourceHandler(const fs::path& file_path) {
		SetRoot(file_path);
	}

	// добавляет запись о документе в базовый вектор
	void ResourceHandler::AddItem(ResourceItem&& item) {
		// записываем данные в вектор и берем ссылку
		auto& ref = _resource_base.emplace_back(std::move(item));
		// также обновляем записи в словарях для быстрого поиска
		_name_to_data_ptr.insert( { ref._name, &ref });
		_path_to_data_ptr.insert({ ref._path, &ref });
	}

	// возвращает указатель на данные по имени файла
	ResourcePtr ResourceHandler::GetItem(std::string_view file_name) const {
		return !Count(file_name) ? nullptr : _name_to_data_ptr.at(file_name);
	}

	// возвращает указатель на данные по пути к файлу
	ResourcePtr ResourceHandler::GetItem(const fs::path& file_path) const {
		return !Count(file_path) ? nullptr : _path_to_data_ptr.at(file_path.generic_string());
	}

	// подтверждает наличие файла по имени
	bool ResourceHandler::Count(std::string_view file_name) const {
		return _name_to_data_ptr.count(file_name);
	}	

	// подтверждает наличие файла по пути
	bool ResourceHandler::Count(fs::path file_path) const {
		return _path_to_data_ptr.count(file_path.generic_string());
	}

	void ResourceHandler::SetRoot(const fs::path& file_path) {
		//  в данную функцию должен придти путь к основному руту с файлами
		if (!fs::is_directory(file_path)) {
			throw std::runtime_error("Incoming Path is not a RootFolder");
		}

		// создаём первую запись о руте
		resource_handler::ResourceItem root;
		root._path = file_path.generic_string();
		root._name = file_path.filename().string();
		root._type = ResourceType::root;
		// добавляем первую запись о руте
		AddItem(std::move(root));
		// запускаем рекурсию обхода дерева вложенных папок
		ProcessRootTree(file_path);
	}

	// парсит расширение файла и возвращает тип
	ResourceType ResourceHandler::ParseFileExtension(std::string_view label) {
		std::string extension = "";

		// перебираем полученную строку задом на перед
		for (auto it = label.rbegin(); it != label.rend(); it++) {
			if (*it == '.') {
				break;                   // как только дошли до точки, то прекращаем цикл
			}
			extension += std::tolower(*it);
		}

		// реверсим строку обратно в нормальный вид
		std::reverse(extension.begin(), extension.end());

		// смотрим совпадение расширения файла в константной мапе
		if (__FILES_EXTENSIONS__.count(extension)) {
			return __FILES_EXTENSIONS__.at(extension);
		}
		else {
			return ResourceType::unknow;
		}
	}

	// рекурентный проход по всем вложенным каталогам
	void ResourceHandler::ProcessRootTree(const fs::path& file_path) {

		for (const fs::directory_entry& dir_entry
			: fs::directory_iterator(file_path)) {

			if (dir_entry.is_directory())
			{
				// создаём запись о папке
				resource_handler::ResourceItem folder;
				// generic_string() необходим что бы на Windows не было \\ в качестве разделителя
				folder._path = dir_entry.path().generic_string();
				folder._name = dir_entry.path().filename().string();
				folder._type = ResourceType::folder;
				// добавляем запись о папке
				AddItem(std::move(folder));
				// продолжаем рекурентный перебор
				ProcessRootTree(dir_entry);

			}
			else if (dir_entry.is_regular_file())
			{
				// создаём запись о файле
				resource_handler::ResourceItem file;
				file._path = dir_entry.path().generic_string();
				file._name = dir_entry.path().filename().string();
				file._type = ParseFileExtension(file._name);
				// добавляем запись о файле
				AddItem(std::move(file));
			}
		}
	}

	namespace detail {

		resource_handler::ResourceHandler LoadFiles(const char* main_root) {

			std::string path_line(main_root);
			// добавляем слеш, если его не поставили
			if (path_line.back() != '/' && path_line.back() != '\\') {
				path_line.push_back('/');
			}

			return resource_handler::ResourceHandler(path_line);
		}

	} // namespace detail

} // namespace resourse_handler