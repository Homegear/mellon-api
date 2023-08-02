/* Copyright 2013-2019 Homegear GmbH */

#ifndef IBS_REST_SERVER_SERVER_H
#define IBS_REST_SERVER_SERVER_H

#include "RestClientInfo.h"
#include "RestInfo.h"

#include <homegear-base/BaseLib.h>

#include <shared_mutex>

namespace MellonApi {

class RestServer {
 private:
  std::shared_ptr<BaseLib::HttpServer> _httpServer;

  std::shared_mutex _clientInfoMutex;
  std::map<int32_t, std::shared_ptr<RestClientInfo>> _clientInfo;

  std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<std::unordered_map<std::string, std::function<BaseLib::PVariable(const PRestInfo &, bool &handled)>>>>> _handlersPublic;
  std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<std::unordered_map<std::string, std::function<BaseLib::PVariable(const PRestInfo &)>>>>> _handlersPrivate;

  void newConnection(int32_t clientId, std::string address, uint16_t port);
  void connectionClosed(int32_t clientId);
  void packetReceived(int32_t clientId, BaseLib::Http http);

  C1Net::TcpPacket getError(int32_t code, std::string codeDescription, std::string longDescription, const std::vector<std::string> &additionalHeaders = {});
  static C1Net::TcpPacket generateResponse(const std::string &content);
  static C1Net::TcpPacket generateResponse(const BaseLib::PVariable &resultJson);

  bool executePublicHandler(const std::shared_ptr<std::unordered_map<std::string, std::function<BaseLib::PVariable(const PRestInfo &, bool &handled)>>> &handlers, const PRestInfo &restInfo);
  bool executePrivateHandler(const std::shared_ptr<std::unordered_map<std::string, std::function<BaseLib::PVariable(const PRestInfo &)>>> &handlers, const PRestInfo &restInfo);
 public:
  RestServer();
  virtual ~RestServer();

  void bind();
  void start();
  void stop();
  void reload();

  static BaseLib::PVariable getJsonSuccessResult(const BaseLib::PVariable &result);
  static BaseLib::PVariable getJsonErrorResult(const std::string &message, const std::string &description = "");
};

}

#endif //IBS_REST_SERVER_SERVER_H
