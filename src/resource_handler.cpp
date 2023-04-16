#include "resource_handler.h"

#include <iostream>

namespace resource_handler {

	ResourceHandler::ResourceHandler(const fs::path& file_path) {
		SetRootDirectory(file_path);
	}

	// добавляет запись о документе в базовый вектор
	void ResourceHandler::AddResourceItem(ResourceItem&& item) {
		// записываем данные в вектор и берем ссылку
		auto& ref = _resource_base.emplace_back(std::move(item));
		// также обновляем записи в словарях для быстрого поиска
		_name_to_data_ptr.insert( { ref._name, &ref });
		_path_to_data_ptr.insert({ ref._path, &ref });
	}

	// возвращает указатель на данные по имени файла
	ResourcePtr ResourceHandler::GetResourseByName(std::string_view file_name) const {
		return !IsIndexed(file_name) ? nullptr : _name_to_data_ptr.at(file_name);
	}
	// возвращает указатель на данные по пути к файлу
	ResourcePtr ResourceHandler::GetResourseByPath(const fs::path& file_path) const {
		return !IsIndexed(file_path) ? nullptr : _path_to_data_ptr.at(file_path.generic_string());
	}

	// возвращает подтверждение наличия файла
	bool ResourceHandler::IsIndexed(std::string_view file_name) const {
		return _name_to_data_ptr.count(file_name);
	}	
	// возвращает подтверждение наличия файла
	bool ResourceHandler::IsIndexed(fs::path file_path) const {
		return _path_to_data_ptr.count(file_path.generic_string());
	}

	void ResourceHandler::SetRootDirectory(const fs::path& file_path) {
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
		AddResourceItem(std::move(root));
		// запускаем рекурсию обхода дерева вложенных папок
		FileIndexRecourse(file_path);
	}

	// парсит расширение файла и возвращает тип
	ResourceType ResourceHandler::ParseFileLabel(std::string_view label) {
		std::string labels = "";

		for (auto it = label.rbegin(); it != label.rend(); it++) {
			if (*it == '.') {
				break;
			}
			labels += std::tolower(*it);
		}

		std::reverse(labels.begin(), labels.end());

		if (labels == "html") return ResourceType::html;
		if (labels == "htm") return ResourceType::htm;
		if (labels == "css") return ResourceType::css;
		if (labels == "txt") return ResourceType::txt;
		if (labels == "json") return ResourceType::json;
		if (labels == "js") return ResourceType::js;
		if (labels == "xml") return ResourceType::xml;
		if (labels == "png") return ResourceType::png;
		if (labels == "jpg") return ResourceType::jpg;
		if (labels == "jpe") return ResourceType::jpe;
		if (labels == "jpeg") return ResourceType::jpeg;
		if (labels == "gif") return ResourceType::gif;
		if (labels == "bmp") return ResourceType::bmp;
		if (labels == "ico") return ResourceType::ico;
		if (labels == "tif") return ResourceType::tif;
		if (labels == "tiff") return ResourceType::tiff;
		if (labels == "svg") return ResourceType::svg;
		if (labels == "svgz") return ResourceType::svgz;
		if (labels == "mp3") return ResourceType::mp3;

		return ResourceType::unknow;
	}
	// рекурентный проход по всем вложенным каталогам
	void ResourceHandler::FileIndexRecourse(const fs::path& file_path) {

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
				AddResourceItem(std::move(folder));
				// продолжаем рекурентный перебор
				FileIndexRecourse(dir_entry);

			}
			else if (dir_entry.is_regular_file())
			{
				// создаём запись о файле
				resource_handler::ResourceItem file;
				file._path = dir_entry.path().generic_string();
				file._name = dir_entry.path().filename().string();
				file._type = ParseFileLabel(file._name);
				// добавляем запись о файле
				AddResourceItem(std::move(file));
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