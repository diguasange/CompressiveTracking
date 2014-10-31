#include "Config.h"

#include <fstream>
#include <iostream>
#include <sstream>

using namespace std;

Config::Config(const std::string& path)
{
	SetDefaults();

	ifstream f(path.c_str());
	if (!f)
	{
		cout << "error: could not load config file: " << path << endl;
		return;
	}

	string line, name, tmp;
	while (getline(f, line))
	{
		istringstream iss(line);
		iss >> name >> tmp;

		// skip invalid lines and comments
		if (iss.fail() || tmp != "=" || name[0] == '#') continue;

		if (name == "quietMode") iss >> quietMode;
		else if (name == "debugMode") iss >> debugMode;
		else if (name == "sequenceBasePath") iss >> sequenceBasePath;
		else if (name == "sequenceName") iss >> sequenceName;
		else if (name == "resultsPath") iss >> resultsPath;
		else if (name == "frameWidth") iss >> frameWidth;
		else if (name == "frameHeight") iss >> frameHeight;
		
	}
}

void Config::SetDefaults()
{

	quietMode = false;
	debugMode = false;

	sequenceBasePath = "";
	sequenceName = "";
	resultsPath = "";

	frameWidth = 320;
	frameHeight = 240;

}