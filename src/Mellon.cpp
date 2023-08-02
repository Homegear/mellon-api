//
// Created by sathya on 31.01.19.
//

#include "Mellon.h"

#include "Gd.h"
#include "Serial.h"

#include <homegear-base/Sockets/SerialReaderWriter.h>

namespace MellonApi {

Mellon::Mellon(const std::string &device, bool verboseOutput) : _device(device), _verboseOutput(verboseOutput) {

}

uint32_t Mellon::getUsageDay() {
  uint32_t usage = 0;
  std::lock_guard<std::mutex> usageGuard(_usageMutex);
  auto time = BaseLib::HelperFunctions::getTime();
  while (!_usageDay.empty() && _usageDay.front().first < time - 86400000) _usageDay.pop_front();
  for (auto &element : _usageDay) {
    usage += element.second;
  }
  return usage / 1000;
}

uint32_t Mellon::getUsageHour() {
  uint32_t usage = 0;
  std::lock_guard<std::mutex> usageGuard(_usageMutex);
  auto time = BaseLib::HelperFunctions::getTime();
  while (!_usageHour.empty() && _usageHour.front().first < time - 3600000) _usageHour.pop_front();
  for (auto &element : _usageHour) {
    usage += element.second;
  }
  return usage / 1000;
}

std::vector<uint8_t> Mellon::executeCommand(MellonCommands command, const std::vector<uint8_t> &arguments) {
  try {
    std::lock_guard<std::mutex> uartGuard(_uartMutex);
    int64_t startTime = BaseLib::HelperFunctions::getTime();
    Serial serial(_device, 1000000, 0);
    serial.openDevice(false, false);

    for (int32_t i = 0; i < 10; i++) {
      { //Send command
        std::vector<uint8_t> packet;
        packet.reserve(3 + arguments.size());
        packet.push_back((uint8_t)((arguments.size() + 1) >> 8u));
        packet.push_back((uint8_t)((arguments.size() + 1) & 0xFFu));
        packet.push_back((uint8_t)command);
        packet.insert(packet.end(), arguments.begin(), arguments.end());

        std::vector<uint8_t> escapedPacket;
        escapedPacket.reserve((packet.size() * 2) + 1);
        escapedPacket.push_back(0xF0);
        for (auto byte : packet) {
          if (byte >= 0xF0) {
            escapedPacket.push_back(0xF1);
            escapedPacket.push_back((uint8_t)(byte & 0x7Fu));
          } else escapedPacket.push_back(byte);
        }

        serial.writeData(escapedPacket);
      }

      int32_t result = 0;
      char charBuffer = 0;
      uint8_t data = 0;
      bool escape = false;
      std::vector<uint8_t> buffer;
      buffer.reserve(1024);
      bool retry = false;
      for (int32_t timeouts = 0; timeouts < 300;) //Retry for 30 seconds
      {
        retry = false;
        result = serial.readChar(charBuffer, 100000);
        data = (uint8_t)charBuffer;
        if (result == 0) {
          if (buffer.empty()) {
            if (data >= 0xF2) {
              if (data >= 0xF3) {
                if (data == 0xF3) Gd::out.printError("Mellon error: Unknown command.");
                else if (data == 0xF4) Gd::out.printError("Mellon error: Wrong payload size.");
                else if (data == 0xF6) Gd::out.printError("Mellon error: Please wait a little and try again.");
                else if (data == 0xF7) Gd::out.printError("Mellon error: Not enough memory.");
                else if (data == 0xF8) Gd::out.printError("Mellon error: Too much data.");
                else if (data == 0xF9) Gd::out.printError("Mellon error: This command can only be executed once. You need to factory reset the device.");
                else if (data == 0xFA) Gd::out.printError("Mellon error: Unauthorized. Please login with apropriate permissions.");
                else if (data == 0xFB) Gd::out.printError(R"(Mellon error: The device is locked. Please unlock it using "getUnlockParameters" (on server Mellon), "getUnlockResponse" (on user Mellon) and "unlock" (on server Mellon).)");
                else if (data == 0xFC) Gd::out.printError(R"(Mellon error: Invalid time. Please call "setTime".)");
                else if (data == 0xFF && i < 9) {
                  do {
                    result = serial.readChar(charBuffer, 10);
                  } while (result == 0);
                  retry = true;
                  break;
                } else Gd::out.printError("Mellon error: NACK received (" + BaseLib::HelperFunctions::getHexString((int32_t)data, 2) + ")");
                break;
              } else {
                buffer.push_back(data);
                break;
              }
            } else if (data != 0xF0) {
              serial.writeChar((uint8_t)0xFF);
              continue;
            }
          }
          if (data == 0xF0u) {
            buffer.clear();
            escape = false;
          } else if (escape) {
            data |= 0x80u;
            escape = false;
          } else if (data == 0xF1u) {
            escape = true;
            continue;
          }
          buffer.push_back(data);
          if (buffer.size() > 3 && buffer.size() == ((((uint16_t)buffer.at(1)) << 8) | buffer.at(2)) + 3) {
            serial.writeChar((uint8_t)0xF2);
            if ((MellonCommands)buffer.at(3) == MellonCommands::output) {
              if (_verboseOutput) {
                std::string output(buffer.begin() + 4, buffer.end());
                Gd::out.printDebug("Debug: UARTOUTPUT: " + output);
              }
              buffer.clear();
            } else {
              break;
            }
          }
        } else if (result == -1) {
          Gd::out.printError("Mellon error: Communication error");
          break;
        } else timeouts++;
      }
      if (retry) continue;

      serial.closeDevice();

      auto endTime = BaseLib::HelperFunctions::getTime();
      auto timeInUse = endTime - startTime;
      {
        std::lock_guard<std::mutex> usageGuard(_usageMutex);
        while (!_usageDay.empty() && _usageDay.front().first < endTime - 86400000) _usageDay.pop_front();
        while (!_usageHour.empty() && _usageHour.front().first < endTime - 3600000) _usageHour.pop_front();
        _usageDay.emplace_back(std::make_pair(endTime, timeInUse));
        _usageHour.emplace_back(std::make_pair(endTime, timeInUse));
      }

      if (buffer.size() <= 3) {
        Gd::out.printError("Mellon error: No response received");
        return std::vector<uint8_t>();
      }
      return buffer;
    }
  }
  catch (std::exception &ex) {
    Gd::out.printError("Mellon error: " + std::string(ex.what()));
    return std::vector<uint8_t>();
  }
  return std::vector<uint8_t>();
}

std::string Mellon::getVersion() {
  auto response = executeCommand(MellonCommands::getFirmwareVersion);
  if (response.size() == 5) {
    return std::to_string((int32_t)response.at(3)) + "." + std::to_string((int32_t)response.at(4));
  } else Gd::out.printError("Mellon error: Wrong response size.");
  return "";
}

std::string Mellon::getSerialNumber() {
  auto response = executeCommand(MellonCommands::getSerialNumber);
  if (response.size() > 3) {
    return BaseLib::HelperFunctions::getHexString((char *)response.data() + 3, response.size() - 3);
  } else Gd::out.printError("Mellon error: Wrong response size.");
  return "";
}

std::string Mellon::getName() {
  auto response = executeCommand(MellonCommands::getName);
  if (response.size() > 3) {
    return std::string((char *)response.data() + 3, response.size() - 3);
  } else Gd::out.printError("Mellon error: Wrong response size.");
  return "";
}

void Mellon::setTime() {
  uint64_t timestamp = BaseLib::HelperFunctions::getTimeSeconds();
  std::vector<uint8_t> data{(uint8_t)(timestamp >> 24u), (uint8_t)((timestamp >> 16u) & 0xFFu), (uint8_t)((timestamp >> 8u) & 0xFFu), (uint8_t)(timestamp & 0xFFu)};
  auto response = executeCommand(MellonCommands::setTime, data);
  if (response.size() == 1 && response.at(0) == 0xF2) {
    Gd::out.printInfo("Mellon info: Time successfully set.");
  } else Gd::out.printError("Mellon error: Error setting time.");
}

bool Mellon::isReady() {
  auto response = executeCommand(MellonCommands::isReady);
  if (response.size() == 4) return (bool)response.at(3);
  return false;
}

bool Mellon::isLoggedIn() {
  auto response = executeCommand(MellonCommands::isLoggedIn);
  if (response.size() == 4) return (bool)response.at(3);
  return false;
}

std::vector<char> Mellon::aesEncrypt(const std::vector<char> &data, uint8_t slot) {
  std::vector<char> encryptedData;
  encryptedData.reserve(data.size() + 1024);
  for (int32_t j = 0; j < data.size(); j += 8190) {
    const uint64_t currentEnd = j + 8190 <= data.size() ? j + 8190 : data.size();
    int32_t currentCount = (j + 8190 <= data.size()) ? 8192 : (data.size() - j) + 2;
    if (currentCount % 16 != 0) currentCount += (16 - (currentCount % 16));
    std::vector<uint8_t> uartPacket{(uint8_t)(slot >> 8u), (uint8_t)(slot & 0xFFu)};
    uartPacket.insert(uartPacket.end(), data.begin() + j, data.begin() + currentEnd);
    auto response = executeCommand(MellonCommands::aesEncrypt, uartPacket);
    if (response.size() == currentCount + 16 + 3) {
      encryptedData.insert(encryptedData.end(), response.begin() + 3, response.end());
    } else {
      Gd::out.printError("Mellon error: Wrong response size.");
      return std::vector<char>();
    }
  }
  return encryptedData;
}

std::vector<char> Mellon::aesDecrypt(const std::vector<char> &data, uint8_t slot) {
  std::vector<char> decryptedData;
  decryptedData.reserve(data.size() + 1024);
  for (int32_t j = 0; j < data.size(); j += 8208) {
    const uint64_t currentEnd = j + 8208 <= data.size() ? j + 8208 : data.size();
    std::vector<uint8_t> uartPacket{(uint8_t)(slot >> 8u), (uint8_t)(slot & 0xFFu)};
    uartPacket.insert(uartPacket.end(), data.begin() + j, data.begin() + currentEnd);
    auto response = executeCommand(MellonCommands::aesDecrypt, uartPacket);
    if (response.size() > 3) {
      decryptedData.insert(decryptedData.end(), response.begin() + 3, response.end());
    } else {
      Gd::out.printError("Mellon error: Wrong response size.");
      return std::vector<char>();
    }
  }
  return decryptedData;
}

std::string Mellon::signx509csr(const std::string &csr, uint8_t slot) {
  if (csr.size() > 8192) return "";

  std::vector<uint8_t> data{slot};
  data.reserve(1 + 2 + csr.size());
  data.push_back(csr.size() >> 8u);
  data.push_back(csr.size() & 0xFFu);
  data.insert(data.end(), csr.begin(), csr.end());

  auto response = executeCommand(MellonCommands::signX509Csr, data);
  if (response.size() > 3) {
    auto rawData = std::string(response.begin() + 3, response.end());
    //A bug up to Mellon firmware version 1.3 causes the data to contain trailing zeroes. Remove them.
    //1. Make sure, the data ends with a zero.
    if (rawData.back() != '\0') rawData.push_back(0);
    //2. Recreate string. The call to ".data()" is not redundant! This removes trailing zeroes as the string is parsed up to the first zero.
    return std::string(rawData.data());
  }
  Gd::out.printError("Mellon error: Wrong response size.");
  return "";
}

}