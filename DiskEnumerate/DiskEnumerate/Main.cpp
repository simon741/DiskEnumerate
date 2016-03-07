#include <windows.h>
#include <string>
#include <vector>
#include <stack>
#include <iostream>
#include <fcntl.h>
#include <io.h>

using namespace std;

const wstring prefix = L"\\\\?\\";
const wstring allFiles = L"*";

void findAds(wstring &filePath, vector<wstring>& list) {
	HANDLE hStream = INVALID_HANDLE_VALUE;
	WIN32_FIND_STREAM_DATA streamData;

	hStream = FindFirstStreamW(filePath.c_str(), FindStreamInfoStandard, &streamData, 0);
	if (hStream == INVALID_HANDLE_VALUE) {
		wcout << "ADS INVALID_HANDLE_VALUE" << filePath << endl;
	}

	while (FindNextStreamW(hStream, &streamData) != 0)
	{
		list.push_back(filePath.substr(prefix.length()) + streamData.cStreamName);
	}

	if (GetLastError() != ERROR_HANDLE_EOF) {
		FindClose(hStream);
	}

	FindClose(hStream);
	hStream = INVALID_HANDLE_VALUE;
}

bool is_prefix(wstring &string1, wstring &string2) {
	if (string1.substr(0, string2.size()) == string2) {
		return true;
	}
	return false;
}

bool checkReparsePoint(wstring &linkPath, wstring &directoryPath) {
	HANDLE handle = CreateFileW(linkPath.c_str(),
		FILE_READ_ATTRIBUTES,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		0,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		0);

	DWORD bufsize = MAX_PATH;
	vector<wchar_t> linkTargetPath(bufsize);
	DWORD pathLength = GetFinalPathNameByHandleW(handle, linkTargetPath.data(), bufsize, FILE_NAME_NORMALIZED);

	if (pathLength > bufsize) {
		bufsize = pathLength;
		linkTargetPath.resize(bufsize);
		pathLength = GetFinalPathNameByHandleW(handle, linkTargetPath.data(), bufsize, FILE_NAME_NORMALIZED);
	}
	if (pathLength == 0) {
		//error
	}

	wstring target(linkTargetPath.data());
	if (!is_prefix(directoryPath, target) &&
		is_prefix(target, directoryPath)) {
		return true;
	}
	return false; //kedy vraca false kedy true?? upravit to???
}

void enumerate(const wstring& directoryPath, const wstring& mask, vector<wstring>& filesList) {
	stack<wstring> directories;
	directories.push(directoryPath);
	filesList.clear();
	vector<WIN32_FIND_DATAW>;

	while (!directories.empty()) {
		wstring currentDirectoryPath = directories.top();
		wstring findPath = currentDirectoryPath + L"\\" + mask;
		directories.pop();

		HANDLE hFind = INVALID_HANDLE_VALUE;
		WIN32_FIND_DATAW findData;
		hFind = FindFirstFileW(findPath.c_str(), &findData);
		if (hFind == INVALID_HANDLE_VALUE) {
			wcout << "INVALID_HANDLE_VALUE " << findPath << findPath.size() << endl;
			continue;
		}
		do {
			wstring fullPath(currentDirectoryPath + L"\\" + findData.cFileName);
			if (wcscmp(findData.cFileName, L".") != 0 &&
				wcscmp(findData.cFileName, L"..") != 0) {
				if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					if (findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
						if (checkReparsePoint(fullPath, currentDirectoryPath)) {
							directories.push(fullPath);
						}
					}
					else {
						directories.push(fullPath);
					}
				}
				else {
					filesList.push_back(fullPath.substr(prefix.length()));
					findAds(fullPath, filesList);
				}
			}
		} while (FindNextFileW(hFind, &findData) != 0);

		if (GetLastError() != ERROR_NO_MORE_FILES) {
			FindClose(hFind);
		}
		FindClose(hFind);
	}
}

int wmain(int argc, wchar_t* argv[])
{
	_setmode(_fileno(stdout), _O_U16TEXT);

	if (argc > 0) {
		wstring path(prefix + argv[1]);

		vector<wstring> filesList;
		enumerate(path, allFiles, filesList);

		for (wstring file : filesList) {
			wcout << file << endl;
		}
	}
	else {
		cerr << "nezadali ste cestu" << endl;
	}

	return 0;
}