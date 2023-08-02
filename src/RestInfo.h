#include <utility>

#include <utility>

#include <utility>

//
// Created by sathya on 08.10.19.
//

#ifndef MELLON_API_RESTINFO_H
#define MELLON_API_RESTINFO_H

#include "RestClientInfo.h"
#include <homegear-base/Encoding/Http.h>

namespace MellonApi {

struct RestInfo {
  const PClientInfo clientInfo;
  BaseLib::Http http;
  const std::string pathString;
  const std::vector<std::string> path;
  const std::unordered_map<std::string, std::string> queryParameters;
  BaseLib::PVariable data = std::make_shared<BaseLib::Variable>();

  RestInfo() {}
  RestInfo(PClientInfo clientInfo, const BaseLib::Http &http, std::string pathString, std::vector<std::string> path)
      : clientInfo(std::move(clientInfo)), http(http), pathString(std::move(pathString)), path(std::move(path)) {}
  RestInfo(PClientInfo clientInfo, const BaseLib::Http &http, std::string pathString, std::vector<std::string> path, std::unordered_map<std::string, std::string> queryParameters)
      : clientInfo(std::move(clientInfo)), http(http), pathString(std::move(pathString)), path(std::move(path)), queryParameters(std::move(queryParameters)) {}
  RestInfo(PClientInfo clientInfo, const BaseLib::Http &http, std::string pathString, std::vector<std::string> path, BaseLib::PVariable data)
      : clientInfo(std::move(clientInfo)), http(http), pathString(std::move(pathString)), path(std::move(path)), data(std::move(data)) {}
};

typedef std::shared_ptr<RestInfo> PRestInfo;

}

#endif
