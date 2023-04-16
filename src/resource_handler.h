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

		void AddResourceItem(ResourceItem&& item);                                  // ��������� ������ � ��������� � ������� ������

		std::string_view GetRootDirectoryName() const {
			return _resource_base[0]._name;
		}
		std::string_view GetRootDirectoryPath() const {
			return _resource_base[0]._path;
		}

		ResourcePtr GetResourseByName(std::string_view file_name) const;            // ���������� ��������� �� ������ �� ����� �����
		ResourcePtr GetResourseByPath(const fs::path& file_path) const;             // ���������� ��������� �� ������ �� ���� � �����

		bool IsIndexed(std::string_view file_name) const;                           // ���������� ������������� ������� �����
		bool IsIndexed(fs::path file_path) const;                                   // ���������� ������������� ������� �����

	private:

		std::deque<ResourceItem> _resource_base;

		FileIndexNameToPath _name_to_data_ptr;
		FileIndexPathToName _path_to_data_ptr;

		void SetRootDirectory(const fs::path& file_path);
		ResourceType ParseFileLabel(std::string_view label);                        // ������ ���������� ����� � ���������� ���
		void FileIndexRecourse(const fs::path& file_path);                          // ����������� ������ �� ���� ��������� ���������
	};

	namespace detail {

		resource_handler::ResourceHandler LoadFiles(const char* file_path);         // ������� ������������-���������� ������

	} // namespace detail

} // namespace resourse_handler