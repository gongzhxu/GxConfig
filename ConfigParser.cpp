#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
 
#include "ConfigParser.h"
 
ConfigValue::ConfigValue(const std::string & value):_value(value)
{
}
 
std::string ConfigValue::asString(const std::string & defValue) const
{
	if(_value.empty())
	{
		return defValue;
	}
	
    return _value;
}
 
int32_t ConfigValue::asInt32(int32_t defValue) const
{
	if(_value.empty())
	{
		return defValue;
	}
	
    return std::atoi(_value.c_str());
}
 
int64_t ConfigValue::asInt64(int64_t defValue) const
{
	if(_value.empty())
	{
		return defValue;
	}
	
    return std::atol(_value.c_str());
}

ConfigParser::ConfigParser(const std::string & filename):_filename(filename)
{
}

bool ConfigParser::init(int * linenum)
{
	int nline = 0;
	bool bErr = false;
	
	{
		std::unique_lock<std::mutex> lock(_mutex);
		std::ifstream ifs(_filename);
		if(ifs.good())
		{
			std::string line;
			std::string section;
			
			while(std::getline(ifs, line))
			{
				++nline;
				
				trim(line);
				if(line.size() == 0)
					continue;
	 
				switch(line[0])
				{
					case '#':
					case ';':
						// Ignore comments
						break;
					case '[':
						// Section header
						section = read_section(line);
						if(section.empty())
						{
							bErr = true;
						}
						break;
					default:
						bErr = !read_value(line, section);
						break;
				}
				
				if(bErr)
				{
					break;
				}
			}
			ifs.close();
		}
	}
	
	if(linenum)
	{
		*linenum = nline;
	}
	
	return !bErr;
}

ConfigValue ConfigParser::get(std::string section, std::string key)
{
	trim(section);
	trim(key);
	
	std::unique_lock<std::mutex> lock(_mutex);
	if(_sections.count(section) == 0 || _sections[section].count(key) == 0)
    {
       return ConfigValue("");
    }
	
	return ConfigValue(_sections[section][key]);
}

bool ConfigParser::set(std::string section, std::string key, std::string value)
{
	trim(section);
	trim(key);
	trim(value);
	
	if(section.empty() || key.empty() || value.empty())
	{
		return false;
	}
	
	std::unique_lock<std::mutex> lock(_mutex);
	if(_sections.count(section) > 0)
	{
		if(_sections[section].count(key) > 0)
		{
			update_key(section, key, value);
		}
		else
		{
			insert_key(section, key, value);
		}
	}
	else
	{
		insert_section(section, key, value);
	}
	
	_sections[section][key] = value;
	return true;
}

bool ConfigParser::have(std::string section, std::string key)
{
	trim(section);
	trim(key);
	
	std::unique_lock<std::mutex> lock(_mutex);
	return (_sections.count(section) > 0 && _sections[section].count(key) > 0)? true: false;
}

void ConfigParser::insert_section(const std::string & section, const std::string & key, const std::string & value)
{
	std::ofstream ofs(_filename, std::ios::out|std::ios::app);
    if(ofs.good())
    {
		ofs <<  "[" << section << "]" << std::endl;
		ofs << key << " = " << value << std::endl;
		ofs.close();
	}
}

void ConfigParser::update_key(const std::string & section, const std::string & key, const std::string & value)
{
	std::stringstream ss;
	
	std::ifstream ifs(_filename);
	if(ifs.good())
    {
        std::string line1;
        std::string section1;
		
        while(std::getline(ifs, line1))
        {
            std::string line = trim_copy(line1);
            if(line.size() == 0)
			{
				ss << line1 << std::endl;
                continue;
			}
			
			bool bUpdate = false;
            switch(line[0])
            {
			case '#':
			case ';':
				// Ignore comments
				break;
			case '[':
				// Section header
				section1 = read_section(line);
				break;
			default:
				if(section1 == section && find_value(line, key))
				{
					bUpdate = true;
				}
				break;
            }
			
			if(bUpdate)
			{
				ss << key << " = " << value << std::endl;
			}
			else
			{
				ss << line1 << std::endl;
			}
        }    
		
		ifs.close();
    }
	
	std::ofstream ofs(_filename, std::ios::in|std::ios::trunc);
	if(ofs.good())
	{
		ofs << ss.str();
		ofs.close();
	}
}

void ConfigParser::insert_key(const std::string & section, const std::string & key, const std::string & value)
{
	std::stringstream ss;
	
	std::ifstream ifs(_filename);
	if(ifs.good())
    {
        std::string line1;
        std::string section1 = "";
		bool bInsert = false;
        while(std::getline(ifs, line1))
        {
            std::string line = trim_copy(line1);
            if(line.size() == 0)
			{
				ss << line1 << std::endl;
                continue;
			}
			
            switch(line[0])
            {
			case '#':
			case ';':
				// Ignore comments
				break;
			case '[':
				// Section header
				section1 = read_section(line);
				break;
			default:
				if(section1 == section && !bInsert)
				{
					ss << key << " = " << value << std::endl;
					bInsert = true;
				}
				break;
            }
			
			ss << line1 << std::endl;
        }   

		ifs.close();
    }
	
	std::ofstream ofs(_filename, std::ios::in|std::ios::trunc);
	if(ofs.good())
	{
		ofs << ss.str();
		ofs.close();
	}
}

std::string ConfigParser::read_section(const std::string& line)
{
    if(line[line.size() - 1] != ']')
        return "";
	
    return trim_copy(line.substr(1, line.size() - 2));
}

bool ConfigParser::find_value(const std::string & line, const std::string & key)
{
    if(line.find('=') == std::string::npos)
		return false;
	
    std::istringstream iss(line);
    std::string key1;
    std::getline(iss, key1, '=');
	trim(key1);
	
	return key == key1? true: false; 
}

bool ConfigParser::read_value(const std::string& line, const std::string& header)
{
    if(header == "" || line.find('=') == std::string::npos)
		return false;
 
    std::istringstream iss(line);
    std::string key;
    std::string val;
    std::getline(iss, key, '=');
	trim(key);
	
    if(key.size() == 0)
    {
        return false;
    }
 
    std::getline(iss, val);
	trim(val);
	
    _sections[header][key] = val;
	return true;
}

void ConfigParser::ltrim(std::string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
                return !std::isspace(ch);
                }));
}
 
void ConfigParser::rtrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
                return !std::isspace(ch);
                }).base(), s.end());
}

void ConfigParser::trim(std::string &s)
{
    ltrim(s);
    rtrim(s);
}

std::string ConfigParser::ltrim_copy(std::string s)
{
    ltrim(s);
    return s;
}

std::string ConfigParser::rtrim_copy(std::string s)
{
    rtrim(s);
    return s;
}
 
std::string ConfigParser::trim_copy(std::string s)
{
    trim(s);
    return s;
}
