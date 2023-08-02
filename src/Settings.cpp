/* Copyright 2013-2019 Homegear GmbH */

#include "Settings.h"

#include <utility>
#include "Gd.h"

namespace MellonApi {

Settings::Settings() = default;

void Settings::reset() {
  _runAsUser = "";
  _runAsGroup = "";
  _debugLevel = 3;
  _memoryDebugging = false;
  _enableCoreDumps = true;
  _workingDirectory = _executablePath;
  _logfilePath = "/var/log/mellon-api/";
  _secureMemorySize = 65536;
  _connectionBacklogSize = 1000;
  _maxConnections = 100000;
  _serverThreads = 50;
  _processingThreads = 100;
  _hostname = "";
  _httpListenAddress = "::1";
  _httpListenPort = 80;
  _httpSsl = true;
  _httpClientCertCaFile = "";
  _httpCertFile = "";
  _httpKeyFile = "";
  _databaseSynchronous = true;
  _databaseMemoryJournal = false;
  _databaseWalJournal = true;
  _databasePath = "";
  _databaseBackupPath = "";
  _databaseMaxBackups = 10;
  _mellonDevice = "";
}

bool Settings::changed() {
  return BaseLib::Io::getFileLastModifiedTime(_path) != _lastModified;
}

void Settings::load(const std::string &filename, std::string executablePath) {
  try {
    _executablePath = std::move(executablePath);
    reset();
    _path = filename;
    char input[1024];
    FILE *fin;
    int32_t len, ptr;
    bool found = false;

    if (!(fin = fopen(filename.c_str(), "r"))) {
      Gd::bl->out.printError("Unable to open config file: " + filename + ". " + strerror(errno));
      return;
    }

    while (fgets(input, 1024, fin)) {
      if (input[0] == '#' || input[0] == '-' || input[0] == '_') continue;
      len = strlen(input);
      if (len < 2) continue;
      if (input[len - 1] == '\n') input[len - 1] = '\0';
      ptr = 0;
      found = false;
      while (ptr < len) {
        if (input[ptr] == '=') {
          found = true;
          input[ptr++] = '\0';
          break;
        }
        ptr++;
      }
      if (found) {
        std::string name(input);
        BaseLib::HelperFunctions::toLower(name);
        BaseLib::HelperFunctions::trim(name);
        std::string value(&input[ptr]);
        BaseLib::HelperFunctions::trim(value);
        if (name == "runasuser") {
          _runAsUser = value;
          Gd::bl->out.printDebug("Debug: runAsUser set to " + _runAsUser);
        } else if (name == "runasgroup") {
          _runAsGroup = value;
          Gd::bl->out.printDebug("Debug: runAsGroup set to " + _runAsGroup);
        } else if (name == "debuglevel") {
          _debugLevel = BaseLib::Math::getNumber(value);
          if (_debugLevel < 0) _debugLevel = 3;
          Gd::bl->debugLevel = _debugLevel;
          Gd::bl->out.printDebug("Debug: debugLevel set to " + std::to_string(_debugLevel));
        } else if (name == "memorydebugging") {
          if (BaseLib::HelperFunctions::toLower(value) == "true") _memoryDebugging = true;
          Gd::bl->out.printDebug("Debug: memoryDebugging set to " + std::to_string(_memoryDebugging));
        } else if (name == "enablecoredumps") {
          if (BaseLib::HelperFunctions::toLower(value) == "false") _enableCoreDumps = false;
          Gd::bl->out.printDebug("Debug: enableCoreDumps set to " + std::to_string(_enableCoreDumps));
        } else if (name == "workingdirectory") {
          _workingDirectory = value;
          if (_workingDirectory.empty()) _workingDirectory = _executablePath;
          if (_workingDirectory.back() != '/') _workingDirectory.push_back('/');
          Gd::bl->out.printDebug("Debug: workingDirectory set to " + _workingDirectory);
        } else if (name == "logfilepath") {
          _logfilePath = value;
          if (_logfilePath.empty()) _logfilePath = "/var/log/mellon-api/";
          if (_logfilePath.back() != '/') _logfilePath.push_back('/');
          Gd::bl->out.printDebug("Debug: logfilePath set to " + _logfilePath);
        } else if (name == "securememorysize") {
          _secureMemorySize = BaseLib::Math::getNumber(value);
          //Allow 0 => disable secure memory. 16384 is minimum size. Values smaller than 16384 are set to 16384 by gcrypt: https://gnupg.org/documentation/manuals/gcrypt-devel/Controlling-the-library.html
          Gd::bl->out.printDebug("Debug: secureMemorySize set to " + std::to_string(_secureMemorySize));
        } else if (name == "connectionbacklogsize") {
          _connectionBacklogSize = BaseLib::Math::getNumber(value);
          Gd::bl->out.printDebug("Debug: connectionBacklogSize set to " + std::to_string(_connectionBacklogSize));
        } else if (name == "maxconnections") {
          _maxConnections = BaseLib::Math::getNumber(value);
          Gd::bl->out.printDebug("Debug: maxConnections set to " + std::to_string(_maxConnections));
        } else if (name == "serverthreads") {
          _serverThreads = BaseLib::Math::getNumber(value);
          Gd::bl->out.printDebug("Debug: serverThreads set to " + std::to_string(_serverThreads));
        } else if (name == "processingthreads") {
          _processingThreads = BaseLib::Math::getUnsignedNumber(value);
          Gd::bl->out.printDebug("Debug: processingThreads set to " + std::to_string(_processingThreads));
        } else if (name == "hostname") {
          _hostname = value;
          Gd::bl->out.printDebug("Debug: hostname set to " + _hostname);
        } else if (name == "httplistenaddress") {
          _httpListenAddress = value;
          if (_httpListenAddress.empty()) _httpListenAddress = "::";
          Gd::bl->out.printDebug("Debug: httpListenAddress set to " + _httpListenAddress);
        } else if (name == "httplistenport") {
          _httpListenPort = (uint16_t) BaseLib::Math::getUnsignedNumber(value);
          if (_httpListenPort == 0) _httpListenPort = 80;
          Gd::bl->out.printDebug("Debug: httpListenPort set to " + std::to_string(_httpListenPort));
        } else if (name == "httpssl") {
          _httpSsl = (value == "true");
          Gd::bl->out.printDebug("Debug: httpSsl set to " + std::to_string(_httpSsl));
        } else if (name == "httpclientcertcafile") {
          _httpClientCertCaFile = value;
          Gd::bl->out.printDebug("Debug: httpClientCertCaFile set to " + _httpClientCertCaFile);
        } else if (name == "httpcertfile") {
          _httpCertFile = value;
          Gd::bl->out.printDebug("Debug: httpCertFile set to " + _httpCertFile);
        } else if (name == "httpkeyfile") {
          _httpKeyFile = value;
          Gd::bl->out.printDebug("Debug: httpKeyFile set to " + _httpKeyFile);
        } else if (name == "databasesynchronous") {
          if (BaseLib::HelperFunctions::toLower(value) == "false") _databaseSynchronous = false;
          Gd::bl->out.printDebug("Debug: databaseSynchronous set to " + std::to_string(_databaseSynchronous));
        } else if (name == "databasememoryjournal") {
          if (BaseLib::HelperFunctions::toLower(value) == "true") _databaseMemoryJournal = true;
          Gd::bl->out.printDebug("Debug: databaseMemoryJournal set to " + std::to_string(_databaseMemoryJournal));
        } else if (name == "databasewaljournal") {
          if (BaseLib::HelperFunctions::toLower(value) == "false") _databaseWalJournal = false;
          Gd::bl->out.printDebug("Debug: databaseWALJournal set to " + std::to_string(_databaseWalJournal));
        } else if (name == "databasepath") {
          _databasePath = value;
          if (!_databasePath.empty() && _databasePath.back() != '/') _databasePath.push_back('/');
          Gd::bl->out.printDebug("Debug: databasePath set to " + _databasePath);
        } else if (name == "databasebackuppath") {
          _databaseBackupPath = value;
          if (!_databaseBackupPath.empty() && _databaseBackupPath.back() != '/') _databaseBackupPath.push_back('/');
          Gd::bl->out.printDebug("Debug: databaseBackupPath set to " + _databaseBackupPath);
        } else if (name == "databasemaxbackups") {
          _databaseMaxBackups = BaseLib::Math::getNumber(value);
          if (_databaseMaxBackups > 10000) _databaseMaxBackups = 10000;
          Gd::bl->out.printDebug("Debug: databaseMaxBackups set to " + std::to_string(_databaseMaxBackups));
        } else if (name == "mellondevice") {
          _mellonDevice = value;
          Gd::bl->out.printDebug("Debug: mellonDevice set to " + _mellonDevice);
        } else {
          Gd::bl->out.printWarning("Warning: Setting not found: " + std::string(input));
        }
      }
    }

    fclose(fin);
    _lastModified = BaseLib::Io::getFileLastModifiedTime(filename);
  }
  catch (const std::exception &ex) {
    Gd::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

}