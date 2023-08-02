/* Copyright 2013-2019 Homegear GmbH */

#include "Gd.h"

#include <homegear-base/BaseLib.h>

namespace MellonApi {

std::unique_ptr<BaseLib::SharedObjects> Gd::bl;
BaseLib::Output Gd::out;
std::string Gd::runAsUser;
std::string Gd::runAsGroup;
std::string Gd::configPath = "/etc/mellon-api/";
std::string Gd::pidfilePath;
std::string Gd::workingDirectory;
std::string Gd::executablePath;
std::string Gd::executableFile;
int64_t Gd::startingTime = BaseLib::HelperFunctions::getTime();
Settings Gd::settings;
std::unique_ptr<Database> Gd::db;
std::unique_ptr<RestServer> Gd::restServer;
std::unique_ptr<Mellon> Gd::mellon;

}