//
// Created by sathya on 31.01.19.
//

#ifndef MELLON_MELLON_H
#define MELLON_MELLON_H

#include "MellonCommands.h"

#include <string>
#include <vector>
#include <mutex>
#include <deque>

namespace MellonApi {

class Mellon {
 public:
  Mellon(const std::string &device, bool verboseOutput);
  ~Mellon() = default;

  uint32_t getUsageDay();
  uint32_t getUsageHour();

  std::string getVersion();
  std::string getSerialNumber();
  std::string getName();
  void setTime();
  bool isReady();
  bool isLoggedIn();
  std::vector<char> aesEncrypt(const std::vector<char> &data, uint8_t slot);
  std::vector<char> aesDecrypt(const std::vector<char> &data, uint8_t slot);
  std::string signx509csr(const std::string &csr, uint8_t slot);
 private:
  std::mutex _uartMutex;
  std::string _device;
  bool _verboseOutput = false;
  std::mutex _usageMutex;
  std::deque<std::pair<int64_t, uint32_t>> _usageDay;
  std::deque<std::pair<int64_t, uint32_t>> _usageHour;

  std::vector<uint8_t> executeCommand(MellonCommands command, const std::vector<uint8_t> &arguments = std::vector<uint8_t>());
};

}

#endif //IBSCRYPTCLIENT_IBSCRYPTSTICK_H
