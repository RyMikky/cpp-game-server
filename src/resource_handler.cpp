#include "resource_handler.h"

#include <iostream>

namespace resource_handler {

	ResourceHandler::ResourceHandler(const char* main_root) {

		std::string path_line(main_root);
		// ��������� ����, ���� ��� �� ���������
		if (path_line.back() != '/' && path_line.back() != '\\') {
			path_line.push_back('/');
		}

		SetRoot(path_line);
	}

	ResourceHandler::ResourceHandler(const fs::path& file_path) {
		SetRoot(file_path);
	}

	// ��������� ������ � ��������� � ������� ������
	void ResourceHandler::AddItem(ResourceItem&& item) {
		// ���������� ������ � ������ � ����� ������
		auto& ref = _resource_base.emplace_back(std::move(item));
		// ����� ��������� ������ � �������� ��� �������� ������
		_name_to_data_ptr.insert( { ref._name, &ref });
		_path_to_data_ptr.insert({ ref._path, &ref });
	}

	// ���������� ��������� �� ������ �� ����� �����
	ResourcePtr ResourceHandler::GetItem(std::string_view file_name) const {
		return !Count(file_name) ? nullptr : _name_to_data_ptr.at(file_name);
	}

	// ���������� ��������� �� ������ �� ���� � �����
	ResourcePtr ResourceHandler::GetItem(const fs::path& file_path) const {
		return !Count(file_path) ? nullptr : _path_to_data_ptr.at(file_path.generic_string());
	}

	// ������������ ������� ����� �� �����
	bool ResourceHandler::Count(std::string_view file_name) const {
		return _name_to_data_ptr.count(file_name);
	}	

	// ������������ ������� ����� �� ����
	bool ResourceHandler::Count(fs::path file_path) const {
		return _path_to_data_ptr.count(file_path.generic_string());
	}

	void ResourceHandler::SetRoot(const fs::path& file_path) {
		//  � ������ ������� ������ ������ ���� � ��������� ���� � �������
		if (!fs::is_directory(file_path)) {
			throw std::runtime_error("Incoming Path is not a RootFolder");
		}

		// ������ ������ ������ � ����
		resource_handler::ResourceItem root;
		root._path = file_path.generic_string();
		root._name = file_path.filename().string();
		root._type = ResourceType::root;
		// ��������� ������ ������ � ����
		AddItem(std::move(root));
		// ��������� �������� ������ ������ ��������� �����
		ProcessRootTree(file_path);
	}

	// ������ ���������� ����� � ���������� ���
	ResourceType ResourceHandler::ParseFileExtension(std::string_view label) {
		std::string extension = "";

		// ���������� ���������� ������ ����� �� �����
		for (auto it = label.rbegin(); it != label.rend(); it++) {
			if (*it == '.') {
				break;                   // ��� ������ ����� �� �����, �� ���������� ����
			}
			extension += std::tolower(*it);
		}

		// �������� ������ ������� � ���������� ���
		std::reverse(extension.begin(), extension.end());

		// ������� ���������� ���������� ����� � ����������� ����
		if (__FILES_EXTENSIONS__.count(extension)) {
			return __FILES_EXTENSIONS__.at(extension);
		}
		else {
			return ResourceType::unknow;
		}
	}

	// ����������� ������ �� ���� ��������� ���������
	void ResourceHandler::ProcessRootTree(const fs::path& file_path) {

		for (const fs::directory_entry& dir_entry
			: fs::directory_iterator(file_path)) {

			if (dir_entry.is_directory())
			{
				// ������ ������ � �����
				resource_handler::ResourceItem folder;
				// generic_string() ��������� ��� �� �� Windows �� ���� \\ � �������� �����������
				folder._path = dir_entry.path().generic_string();
				folder._name = dir_entry.path().filename().string();
				folder._type = ResourceType::folder;
				// ��������� ������ � �����
				AddItem(std::move(folder));
				// ���������� ����������� �������
				ProcessRootTree(dir_entry);

			}
			else if (dir_entry.is_regular_file())
			{
				// ������ ������ � �����
				resource_handler::ResourceItem file;
				file._path = dir_entry.path().generic_string();
				file._name = dir_entry.path().filename().string();
				file._type = ParseFileExtension(file._name);
				// ��������� ������ � �����
				AddItem(std::move(file));
			}
		}
	}

	namespace detail {

		resource_handler::ResourceHandler LoadFiles(const char* main_root) {

			std::string path_line(main_root);
			// ��������� ����, ���� ��� �� ���������
			if (path_line.back() != '/' && path_line.back() != '\\') {
				path_line.push_back('/');
			}

			return resource_handler::ResourceHandler(path_line);
		}

	} // namespace detail

} // namespace resourse_handler