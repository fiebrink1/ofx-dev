//
// WinRegistryKey.cpp
//
// $Id: //poco/1.3/Util/src/WinRegistryKey.cpp#3 $
//
// Library: Util
// Package: Windows
// Module:  WinRegistryKey
//
// Copyright (c) 2004-2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
// 
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//


#include "Poco/Util/WinRegistryKey.h"
#include "Poco/Exception.h"
#if defined(POCO_WIN32_UTF8)
#include "Poco/UnicodeConverter.h"
#endif


using Poco::SystemException;
using Poco::NotFoundException;
using Poco::InvalidArgumentException;


namespace Poco {
namespace Util {


WinRegistryKey::WinRegistryKey(const std::string& key):
	_hKey(0)
{
	std::string::size_type pos = key.find('\\');
	if (pos != std::string::npos)
	{
		std::string rootKey = key.substr(0, pos);
		_hRootKey = handleFor(rootKey);
		_subKey   = key.substr(pos + 1);
	}
	else throw InvalidArgumentException("Not a valid registry key", key);
}


WinRegistryKey::WinRegistryKey(HKEY hRootKey, const std::string& subKey):
	_hRootKey(hRootKey),
	_subKey(subKey),
	_hKey(0)
{
}


WinRegistryKey::~WinRegistryKey()
{
	close();
}


void WinRegistryKey::setString(const std::string& name, const std::string& value)
{
	open();
#if defined(POCO_WIN32_UTF8)
	std::wstring uname;
	Poco::UnicodeConverter::toUTF16(name, uname);
	std::wstring uvalue;
	Poco::UnicodeConverter::toUTF16(value, uvalue);
	if (RegSetValueExW(_hKey, uname.c_str(), 0, REG_SZ, (CONST BYTE*) uvalue.c_str(), (DWORD) (uvalue.size() + 1)*sizeof(wchar_t)) != ERROR_SUCCESS)
		handleSetError(name); 
#else
	if (RegSetValueEx(_hKey, name.c_str(), 0, REG_SZ, (CONST BYTE*) value.c_str(), (DWORD) value.size() + 1) != ERROR_SUCCESS)
		handleSetError(name); 
#endif
}


std::string WinRegistryKey::getString(const std::string& name)
{
	open();
	DWORD type;
	DWORD size;
#if defined(POCO_WIN32_UTF8)
	std::wstring uname;
	Poco::UnicodeConverter::toUTF16(name, uname);
	if (RegQueryValueExW(_hKey, uname.c_str(), NULL, &type, NULL, &size) != ERROR_SUCCESS || type != REG_SZ && type != REG_EXPAND_SZ)
		throw NotFoundException(key(name));
	if (size > 0)
	{
		DWORD len = size/2;
		wchar_t* buffer = new wchar_t[len + 1];
		RegQueryValueExW(_hKey, uname.c_str(), NULL, NULL, (BYTE*) buffer, &size);
		buffer[len] = 0;
		std::wstring uresult(buffer);
		delete [] buffer;
		std::string result;
		Poco::UnicodeConverter::toUTF8(uresult, result);
		return result;
	}
#else
	if (RegQueryValueEx(_hKey, name.c_str(), NULL, &type, NULL, &size) != ERROR_SUCCESS || type != REG_SZ && type != REG_EXPAND_SZ)
		throw NotFoundException(key(name));
	if (size > 0)
	{
		char* buffer = new char[size + 1];
		RegQueryValueEx(_hKey, name.c_str(), NULL, NULL, (BYTE*) buffer, &size);
		buffer[size] = 0;
		std::string result(buffer);
		delete [] buffer;
		return result;
	}
#endif
	return std::string();
}


void WinRegistryKey::setStringExpand(const std::string& name, const std::string& value)
{
	open();
#if defined(POCO_WIN32_UTF8)
	std::wstring uname;
	Poco::UnicodeConverter::toUTF16(name, uname);
	std::wstring uvalue;
	Poco::UnicodeConverter::toUTF16(value, uvalue);
	if (RegSetValueExW(_hKey, uname.c_str(), 0, REG_EXPAND_SZ, (CONST BYTE*) uvalue.c_str(), (DWORD) (uvalue.size() + 1)*sizeof(wchar_t)) != ERROR_SUCCESS)
		handleSetError(name); 
#else
	if (RegSetValueEx(_hKey, name.c_str(), 0, REG_EXPAND_SZ, (CONST BYTE*) value.c_str(), (DWORD) value.size() + 1) != ERROR_SUCCESS)
		handleSetError(name); 
#endif
}


std::string WinRegistryKey::getStringExpand(const std::string& name)
{
	open();
	DWORD type;
	DWORD size;
#if defined(POCO_WIN32_UTF8)
	std::wstring uname;
	Poco::UnicodeConverter::toUTF16(name, uname);
	if (RegQueryValueExW(_hKey, uname.c_str(), NULL, &type, NULL, &size) != ERROR_SUCCESS || type != REG_SZ && type != REG_EXPAND_SZ)
		throw NotFoundException(key(name));
	if (size > 0)
	{
		DWORD len = size/2;
		wchar_t* buffer = new wchar_t[len + 1];
		RegQueryValueExW(_hKey, uname.c_str(), NULL, NULL, (BYTE*) buffer, &size);
		buffer[len] = 0;
		wchar_t temp;
		DWORD expSize = ExpandEnvironmentStringsW(buffer, &temp, 1);	
		wchar_t* expBuffer = new wchar_t[expSize];
		ExpandEnvironmentStringsW(buffer, expBuffer, expSize);
		std::string result;
		UnicodeConverter::toUTF8(expBuffer, result);
		delete [] buffer;
		delete [] expBuffer;
		return result;
	}
#else
	if (RegQueryValueEx(_hKey, name.c_str(), NULL, &type, NULL, &size) != ERROR_SUCCESS || type != REG_SZ && type != REG_EXPAND_SZ)
		throw NotFoundException(key(name));
	if (size > 0)
	{
		char* buffer = new char[size + 1];
		RegQueryValueEx(_hKey, name.c_str(), NULL, NULL, (BYTE*) buffer, &size);
		buffer[size] = 0;
		char temp;
		DWORD expSize = ExpandEnvironmentStringsA(buffer, &temp, 1);	
		char* expBuffer = new char[expSize];
		ExpandEnvironmentStringsA(buffer, expBuffer, expSize);
		std::string result(expBuffer);
		delete [] buffer;
		delete [] expBuffer;
		return result;
	}
#endif
	return std::string();
}


void WinRegistryKey::setInt(const std::string& name, int value)
{
	open();
	DWORD data = value;
#if defined(POCO_WIN32_UTF8)
	std::wstring uname;
	Poco::UnicodeConverter::toUTF16(name, uname);
	if (RegSetValueExW(_hKey, uname.c_str(), 0, REG_DWORD, (CONST BYTE*) &data, sizeof(data)) != ERROR_SUCCESS)
		handleSetError(name); 
#else
	if (RegSetValueEx(_hKey, name.c_str(), 0, REG_DWORD, (CONST BYTE*) &data, sizeof(data)) != ERROR_SUCCESS)
		handleSetError(name); 
#endif
}

	
int WinRegistryKey::getInt(const std::string& name)
{
	open();
	DWORD type;
	DWORD data;
	DWORD size;
#if defined(POCO_WIN32_UTF8)
	std::wstring uname;
	Poco::UnicodeConverter::toUTF16(name, uname);
	if (RegQueryValueExW(_hKey, uname.c_str(), NULL, &type, (BYTE*) &data, &size) != ERROR_SUCCESS || type != REG_DWORD)
		throw NotFoundException(key(name));
#else
	if (RegQueryValueEx(_hKey, name.c_str(), NULL, &type, (BYTE*) &data, &size) != ERROR_SUCCESS || type != REG_DWORD)
		throw NotFoundException(key(name));
#endif
	return data;
}


void WinRegistryKey::deleteValue(const std::string& name)
{
	open();
#if defined(POCO_WIN32_UTF8)
	std::wstring uname;
	Poco::UnicodeConverter::toUTF16(name, uname);
	if (RegDeleteValueW(_hKey, uname.c_str()) != ERROR_SUCCESS)
		throw NotFoundException(key(name));
#else
	if (RegDeleteValue(_hKey, name.c_str()) != ERROR_SUCCESS)
		throw NotFoundException(key(name));
#endif
}


void WinRegistryKey::deleteKey()
{
	close();
#if defined(POCO_WIN32_UTF8)
	std::wstring usubKey;
	Poco::UnicodeConverter::toUTF16(_subKey, usubKey);
	if (RegDeleteKeyW(_hRootKey, usubKey.c_str()) != ERROR_SUCCESS)
		throw NotFoundException(key());
#else
	if (RegDeleteKey(_hRootKey, _subKey.c_str()) != ERROR_SUCCESS)
		throw NotFoundException(key());
#endif
}


bool WinRegistryKey::exists()
{
	open();
	DWORD size;
#if defined(POCO_WIN32_UTF8)
	wchar_t name[256];
	return RegEnumValueW(_hKey, 0, name, &size, NULL, NULL, NULL, NULL) == ERROR_SUCCESS;
#else
	char name[256];
	return RegEnumValue(_hKey, 0, name, &size, NULL, NULL, NULL, NULL) == ERROR_SUCCESS;
#endif
}


WinRegistryKey::Type WinRegistryKey::type(const std::string& name)
{
	open();
	DWORD type = REG_NONE;
	DWORD size;
#if defined(POCO_WIN32_UTF8)
	std::wstring uname;
	Poco::UnicodeConverter::toUTF16(name, uname);
	if (RegQueryValueExW(_hKey, uname.c_str(), NULL, &type, NULL, &size) != ERROR_SUCCESS)
		throw NotFoundException(key(name));
#else
	if (RegQueryValueEx(_hKey, name.c_str(), NULL, &type, NULL, &size) != ERROR_SUCCESS)
		throw NotFoundException(key(name));
#endif
	if (type != REG_SZ && type != REG_EXPAND_SZ && type != REG_DWORD)
		throw NotFoundException(key(name)+": type not supported");

	Type aType = (Type)type;
	return aType;
}


bool WinRegistryKey::exists(const std::string& name)
{
	open();
#if defined(POCO_WIN32_UTF8)
	std::wstring uname;
	Poco::UnicodeConverter::toUTF16(name, uname);
	return RegQueryValueExW(_hKey, uname.c_str(), NULL, NULL, NULL, NULL) == ERROR_SUCCESS;
#else
	return RegQueryValueEx(_hKey, name.c_str(), NULL, NULL, NULL, NULL) == ERROR_SUCCESS;
#endif
}


void WinRegistryKey::open()
{
	if (!_hKey)
	{
#if defined(POCO_WIN32_UTF8)
		std::wstring usubKey;
		Poco::UnicodeConverter::toUTF16(_subKey, usubKey);
		if (RegCreateKeyExW(_hRootKey, usubKey.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &_hKey, NULL) != ERROR_SUCCESS)
			throw SystemException("Cannot open registry key: ", key());
#else
		if (RegCreateKeyEx(_hRootKey, _subKey.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &_hKey, NULL) != ERROR_SUCCESS)
			throw SystemException("Cannot open registry key: ", key());
#endif
	}
}


void WinRegistryKey::close()
{
	if (_hKey)
	{
		RegCloseKey(_hKey);
		_hKey = 0;
	}
}


std::string WinRegistryKey::key() const
{
	std::string result;
	if (_hRootKey == HKEY_CLASSES_ROOT)
		result = "HKEY_CLASSES_ROOT";
	else if (_hRootKey == HKEY_CURRENT_CONFIG)
		result = "HKEY_CURRENT_CONFIG";
	else if (_hRootKey == HKEY_CURRENT_USER)
		result = "HKEY_CURRENT_USER";
	else if (_hRootKey == HKEY_LOCAL_MACHINE)
		result = "HKEY_LOCAL_MACHINE";
	else if (_hRootKey == HKEY_USERS)
		result = "HKEY_USERS";
	else if (_hRootKey == HKEY_PERFORMANCE_DATA)
		result = "HKEY_PERFORMANCE_DATA";
	else
		result = "(UNKNOWN)";
	result += '\\';
	result += _subKey;
	return result;
}


std::string WinRegistryKey::key(const std::string& valueName) const
{
	std::string& result = key();
	if (!valueName.empty())
	{
		result += '\\';
		result += valueName;
	}
	return result;
}


HKEY WinRegistryKey::handleFor(const std::string& rootKey)
{
	if (rootKey == "HKEY_CLASSES_ROOT")
		return HKEY_CLASSES_ROOT;
	else if (rootKey == "HKEY_CURRENT_CONFIG")
		return HKEY_CURRENT_CONFIG;
	else if (rootKey == "HKEY_CURRENT_USER")
		return HKEY_CURRENT_USER;
	else if (rootKey == "HKEY_LOCAL_MACHINE")
		return HKEY_LOCAL_MACHINE;
	else if (rootKey == "HKEY_USERS")
		return HKEY_USERS;
	else if (rootKey == "HKEY_PERFORMANCE_DATA")
		return HKEY_PERFORMANCE_DATA;
	else
		throw InvalidArgumentException("Not a valid root key", rootKey);
}


void WinRegistryKey::handleSetError(const std::string& name)
{
	std::string msg = "Failed to set registry value";
	throw SystemException(msg, key(name));
}


void WinRegistryKey::subKeys(WinRegistryKey::Keys& keys)
{
	open();

	DWORD subKeyCount = 0;
	DWORD valueCount = 0;
	
	if (RegQueryInfoKey(_hKey, NULL, NULL, NULL, &subKeyCount, NULL, NULL, &valueCount, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
		return;

#if defined(POCO_WIN32_UTF8)
	wchar_t buf[256];
	DWORD bufSize = sizeof(buf)/sizeof(wchar_t);
	for (DWORD i = 0; i< subKeyCount; ++i)
	{
		if (RegEnumKeyExW(_hKey, i, buf, &bufSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
		{
			std::wstring uname(buf);
			std::string name;
			Poco::UnicodeConverter::toUTF8(uname, name);
			keys.push_back(name);
		}
		bufSize = sizeof(buf)/sizeof(wchar_t);
	}
#else
	char buf[256];
	DWORD bufSize = sizeof(buf);
	for (DWORD i = 0; i< subKeyCount; ++i)
	{
		if (RegEnumKeyEx(_hKey, i, buf, &bufSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
		{
			std::string name(buf);
			keys.push_back(name);
		}
		bufSize = sizeof(buf);
	}
#endif
}


void WinRegistryKey::values(WinRegistryKey::Values& vals)
{
	open();

	DWORD valueCount = 0;
	
	if (RegQueryInfoKey(_hKey, NULL, NULL, NULL, NULL, NULL, NULL, &valueCount, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
		return ;

#if defined(POCO_WIN32_UTF8)
	wchar_t buf[256];
	DWORD bufSize = sizeof(buf)/sizeof(wchar_t);
	for (DWORD i = 0; i< valueCount; ++i)
	{
		if (RegEnumValueW(_hKey, i, buf, &bufSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
		{
			std::wstring uname(buf);
			std::string name;
			Poco::UnicodeConverter::toUTF8(uname, name);
			vals.push_back(name);
		}
		bufSize = sizeof(buf)/sizeof(wchar_t);
	}
#else
	char buf[256];
	DWORD bufSize = sizeof(buf);
	for (DWORD i = 0; i< valueCount; ++i)
	{
		if (RegEnumValue(_hKey, i, buf, &bufSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
		{
			std::string name(buf);
			vals.push_back(name);
		}
		bufSize = sizeof(buf);
	}
#endif
}


} } // namespace Poco::Util
