#ifndef CONFIG_H
#define CONFIG_H

#include <vector>
#include <string>
#include <ostream>

#define VERBOSE (0)

class Config
{
public:
	Config() { SetDefaults(); }
	Config(const std::string& path);


	bool							quietMode;
	bool							debugMode;

	std::string						sequenceBasePath;
	std::string						sequenceName;
	std::string						resultsPath;

	int								frameWidth;
	int								frameHeight;


private:
	void SetDefaults();
};

#endif