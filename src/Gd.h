/* Copyright 2013-2019 Homegear GmbH */

#ifndef GD_H_
#define GD_H_

#include "Settings.h"
#include "RestServer.h"
#include "Database.h"
#include "Mellon.h"

namespace MellonApi {

class Gd {
 public:
  Gd() = delete;
  ~Gd() = default;

  static std::unique_ptr<BaseLib::SharedObjects> bl;
  static BaseLib::Output out;
  static std::string runAsUser;
  static std::string runAsGroup;
  static std::string configPath;
  static std::string pidfilePath;
  static std::string workingDirectory;
  static std::string executablePath;
  static std::string executableFile;
  static int64_t startingTime;
  static Settings settings;
  static std::unique_ptr<Database> db;
  static std::unique_ptr<RestServer> restServer;
  static std::unique_ptr<Mellon> mellon;
};

}

#endif /* GD_H_ */
