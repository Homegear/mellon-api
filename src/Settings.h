/* Copyright 2013-2019 Homegear GmbH */

#ifndef CONFIGSETTINGS_H_
#define CONFIGSETTINGS_H_

#include <homegear-base/BaseLib.h>

namespace MellonApi {

class Settings {
 public:
  Settings();
  ~Settings() = default;

  void load(const std::string& filename, std::string executablePath);

  bool changed();

  [[nodiscard]] std::string runAsUser() const { return _runAsUser; }
  [[nodiscard]] std::string runAsGroup() const { return _runAsGroup; }
  [[nodiscard]] int32_t debugLevel() const { return _debugLevel; }
  [[nodiscard]] bool memoryDebugging() const { return _memoryDebugging; }
  [[nodiscard]] bool enableCoreDumps() const { return _enableCoreDumps; };
  [[nodiscard]] std::string workingDirectory() const { return _workingDirectory; }
  [[nodiscard]] std::string logfilePath() const { return _logfilePath; }
  [[nodiscard]] uint32_t secureMemorySize() const { return _secureMemorySize; }
  [[nodiscard]] uint32_t connectionBacklogSize() const { return _connectionBacklogSize; }
  [[nodiscard]] uint32_t maxConnections() const { return _maxConnections; }
  [[nodiscard]] uint32_t serverThreads() const { return _serverThreads; }
  [[nodiscard]] uint32_t processingThreads() const { return _processingThreads; }
  [[nodiscard]] std::string hostname() const { return _hostname; }
  [[nodiscard]] std::string httpListenAddress() const { return _httpListenAddress; }
  [[nodiscard]] uint16_t httpListenPort() const { return _httpListenPort; }
  [[nodiscard]] bool httpSsl() const { return _httpSsl; }
  [[nodiscard]] std::string httpClientCertCaFile() const { return _httpClientCertCaFile; }
  [[nodiscard]] std::string httpCertFile() const { return _httpCertFile; }
  [[nodiscard]] std::string httpKeyFile() const { return _httpKeyFile; }
  [[nodiscard]] bool databaseSynchronous() const { return _databaseSynchronous; }
  [[nodiscard]] bool databaseMemoryJournal() const { return _databaseMemoryJournal; }
  [[nodiscard]] bool databaseWALJournal() const { return _databaseWalJournal; }
  [[nodiscard]] std::string databasePath() const { return _databasePath; }
  [[nodiscard]] std::string databaseBackupPath() const { return _databaseBackupPath; }
  [[nodiscard]] uint32_t databaseMaxBackups() const { return _databaseMaxBackups; }
  [[nodiscard]] std::string mellonDevice() const { return _mellonDevice; }
 private:
  std::string _executablePath;
  std::string _path;
  int32_t _lastModified = -1;

  std::string _runAsUser;
  std::string _runAsGroup;
  int32_t _debugLevel = 3;
  bool _memoryDebugging = false;
  bool _enableCoreDumps = true;
  std::string _workingDirectory;
  std::string _logfilePath;
  uint32_t _secureMemorySize = 65536;
  uint32_t _connectionBacklogSize = 1024;
  uint32_t _maxConnections = 100000;
  uint32_t _serverThreads = 50;
  uint32_t _processingThreads = 100;
  std::string _hostname;
  std::string _httpListenAddress;
  uint16_t _httpListenPort = 0;
  bool _httpSsl = true;
  std::string _httpClientCertCaFile;
  std::string _httpCertFile;
  std::string _httpKeyFile;
  bool _databaseSynchronous = true;
  bool _databaseMemoryJournal = false;
  bool _databaseWalJournal = true;
  std::string _databasePath;
  std::string _databaseBackupPath;
  uint32_t _databaseMaxBackups = 10;
  std::string _mellonDevice;

  void reset();
};

}
#endif
