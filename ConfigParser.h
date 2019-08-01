#ifndef _CONFIG_PARSER_H_
#define _CONFIG_PARSER_H_

#include <stdint.h>
#include <string>
#include <map>
#include <mutex>

class ConfigValue
{
public:
    ConfigValue(const std::string & value);
 
    std::string asString(const std::string & defValue = "") const;
    int32_t asInt32(int32_t defValue = 0) const;
    int64_t asInt64(int64_t defValue = 0) const;
	
private:
    std::string _value;
};

class ConfigParser
{
public:
    ConfigParser(const std::string & filename);
    
	bool init(int * linenum = NULL);
	ConfigValue get(std::string section, std::string key);
	bool set(std::string section, std::string key, std::string value);
	bool have(std::string section, std::string key);
private:
	void load_config();
	void insert_section(const std::string & section, const std::string & key, const std::string & value);
	void update_key(const std::string & section, const std::string & key, const std::string & value);
	void insert_key(const std::string & section, const std::string & key, const std::string & value);

    std::string read_section(const std::string& line);
 
	bool find_value(const std::string & line, const std::string & key);
    bool read_value(const std::string & line, const std::string & header);
	void write_value();
 
    static void ltrim(std::string &s);
    static void rtrim(std::string &s);
    static void trim(std::string &s);
    static std::string ltrim_copy(std::string s);
    static std::string rtrim_copy(std::string s);
    static std::string trim_copy(std::string s);
private:
    std::map<std::string, std::map<std::string, std::string> > _sections;
	std::string _filename;
	std::mutex _mutex;
};
#endif