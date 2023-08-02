/* Copyright 2013-2019 Homegear GmbH */

#include "RestServer.h"
#include "Gd.h"
#include "HelperFunctions.h"

#include "REST/Ping.h"
#include "REST/Ca.h"
#include "REST/Mellon.h"
#include "REST/Aes.h"
#include "REST/Crl.h"

#include <utility>

namespace MellonApi {

RestServer::RestServer() {
  try {
    Rest::Aes::init();
    Rest::Ca::init();

    _handlersPublic = std::make_shared<std::unordered_map<std::string, std::shared_ptr<std::unordered_map<std::string, std::function<BaseLib::PVariable(const PRestInfo &, bool &handled)>>>>>();

    auto publicHandler = std::make_shared<std::unordered_map<std::string, std::function<BaseLib::PVariable(const PRestInfo &, bool &handled)>>>();
    publicHandler->emplace("ca", std::bind(&Rest::Ca::getPublic, std::placeholders::_1, std::placeholders::_2));
    publicHandler->emplace("ping", std::bind(&Rest::Ping::getPublic, std::placeholders::_1, std::placeholders::_2));
    _handlersPublic->emplace("GET", publicHandler);

    publicHandler = std::make_shared<std::unordered_map<std::string, std::function<BaseLib::PVariable(const PRestInfo &, bool &handled)>>>();
    publicHandler->emplace("ca", std::bind(&Rest::Ca::postPublic, std::placeholders::_1, std::placeholders::_2));
    _handlersPublic->emplace("POST", publicHandler);

    _handlersPrivate = std::make_shared<std::unordered_map<std::string, std::shared_ptr<std::unordered_map<std::string, std::function<BaseLib::PVariable(const PRestInfo &)>>>>>();

    auto privateHandler = std::make_shared<std::unordered_map<std::string, std::function<BaseLib::PVariable(const PRestInfo &)>>>();
    privateHandler->emplace("crl", std::bind(&Rest::Crl::get, std::placeholders::_1));
    privateHandler->emplace("mellon", std::bind(&Rest::Mellon::get, std::placeholders::_1));
    _handlersPrivate->emplace("GET", privateHandler);

    privateHandler = std::make_shared<std::unordered_map<std::string, std::function<BaseLib::PVariable(const PRestInfo &)>>>();
    privateHandler->emplace("aes", std::bind(&Rest::Aes::post, std::placeholders::_1));
    privateHandler->emplace("ca", std::bind(&Rest::Ca::postPrivate, std::placeholders::_1));
    _handlersPrivate->emplace("POST", privateHandler);
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

RestServer::~RestServer() {
  try {
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

C1Net::TcpPacket RestServer::getError(int32_t code, std::string codeDescription, std::string longDescription, const std::vector<std::string> &additionalHeaders) {
  try {
    auto error = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
    error->structValue->emplace("success", std::make_shared<BaseLib::Variable>(false));
    error->structValue->emplace("error", std::make_shared<BaseLib::Variable>(longDescription));
    std::string contentString;
    BaseLib::Rpc::JsonEncoder::encode(error, contentString);

    std::string header;
    BaseLib::Http::constructHeader(contentString.size(), "application/json", code, std::move(codeDescription), additionalHeaders, header);
    C1Net::TcpPacket content;
    content.reserve(header.size() + contentString.size());
    content.insert(content.end(), header.begin(), header.end());
    content.insert(content.end(), contentString.begin(), contentString.end());
    return content;
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return {};
}

BaseLib::PVariable RestServer::getJsonSuccessResult(const BaseLib::PVariable &data) {
  BaseLib::PVariable resultJson = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
  resultJson->structValue->emplace("success", std::make_shared<BaseLib::Variable>(true));
  resultJson->structValue->emplace("result", data);
  return resultJson;
}

BaseLib::PVariable RestServer::getJsonErrorResult(const std::string &message, const std::string &description) {
  BaseLib::PVariable resultJson = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
  resultJson->structValue->emplace("success", std::make_shared<BaseLib::Variable>(false));
  resultJson->structValue->emplace("error", std::make_shared<BaseLib::Variable>(message));
  if (!description.empty())
    resultJson->structValue->emplace("description", std::make_shared<BaseLib::Variable>(description));
  return resultJson;
}

C1Net::TcpPacket RestServer::generateResponse(const std::string &content) {
  try {
    std::string header;
    std::vector<std::string> additionalHeaders;
    BaseLib::Http::constructHeader(content.size(), "application/json", 200, "OK", additionalHeaders, header);
    C1Net::TcpPacket response;
    response.reserve(header.size() + content.size());
    response.insert(response.end(), header.begin(), header.end());
    response.insert(response.end(), content.begin(), content.end());
    return response;
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return {};
}

C1Net::TcpPacket RestServer::generateResponse(const BaseLib::PVariable &resultJson) {
  try {
    std::string content;
    BaseLib::Rpc::JsonEncoder::encode(resultJson, content);

    std::string header;
    std::vector<std::string> additionalHeaders;
    BaseLib::Http::constructHeader(content.size(), "application/json", 200, "OK", additionalHeaders, header);
    C1Net::TcpPacket response;
    response.reserve(header.size() + content.size());
    response.insert(response.end(), header.begin(), header.end());
    response.insert(response.end(), content.begin(), content.end());
    return response;
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return {};
}

void RestServer::packetReceived(int32_t clientId, BaseLib::Http http) {
  try {
    if (Gd::settings.debugLevel() >= 4) Gd::out.printInfo("Info: Packet received: " + http.getHeader().method + " " + http.getHeader().path + "... (remaining header removed for security reasons) ...");
    if (Gd::settings.debugLevel() >= 5) Gd::out.printDebug("Debug: Content:\n" + std::string(http.getContent().data(), http.getContentSize()));

    std::string pathString = http.getHeader().path;

    PClientInfo clientInfo;

    {
      std::shared_lock<std::shared_mutex> clientInfoGuard(_clientInfoMutex);
      auto clientInfoIterator = _clientInfo.find(clientId);
      if (clientInfoIterator == _clientInfo.end()) {
        clientInfoGuard.unlock();
        _httpServer->send(clientId, getError(403, "Forbidden", "Unknown client."), true);
        return;
      }
      clientInfo = clientInfoIterator->second;
    }

    //clientInfo->closeConnection = (http.getHeader().connection == BaseLib::Http::Connection::Enum::close);
    clientInfo->closeConnection = true; //Always close connection
    if (clientInfo->closeConnection && Gd::settings.debugLevel() >= 5) Gd::out.printDebug("Debug: Closing connection after processing this packet.");

    std::vector<std::string> path = BaseLib::HelperFunctions::splitAll(pathString, '/');
    if (path.size() < 4 || path.at(1) != "api") {
      _httpServer->send(clientId, getError(404, "Not found", "The requested URL " + pathString + " was not found on this server."), true);
      return;
    }

    if (path.at(2) != "v1") {
      _httpServer->send(clientId, getError(400, "Bad request", "Unknown API version."), true);
      return;
    }

    std::unordered_map<std::string, std::string> queryParameters;

    {
      auto parts = BaseLib::HelperFunctions::splitAll(http.getHeader().args, '&');
      for(auto& part : parts)
      {
        auto pair = BaseLib::HelperFunctions::splitFirst(part, '=');
        queryParameters.emplace(std::move(pair));
      }
    }

    auto restInfo = std::make_shared<RestInfo>(clientInfo, http, pathString, path, queryParameters);
    if (http.getHeader().contentLength > 0) restInfo->data = BaseLib::Rpc::JsonDecoder::decode(http.getContent());

    { //Handle requests that don't require authentication
      auto handlersPublicIterator = _handlersPublic->find(http.getHeader().method);
      if (handlersPublicIterator != _handlersPublic->end()) {
        if (executePublicHandler(handlersPublicIterator->second, restInfo)) return;
      }
    }

    if (!clientInfo->authenticated) {
      //Do certificate authentication
      auto clientCertDn = _httpServer->getClientCertDn(clientId);
      auto clientCertSerial = _httpServer->getClientCertSerial(clientId);
      auto clientCertExpiration = _httpServer->getClientCertExpiration(clientId);
      if (!clientCertDn.empty() && !clientCertSerial.empty() &&
          clientCertExpiration > BaseLib::HelperFunctions::getTimeSeconds() &&
          Gd::db->isCertificateValid(clientCertSerial)) {
        Gd::out.printInfo("Info: Client successfully logged in (certificate " + clientCertSerial + ", valid to " + BaseLib::HelperFunctions::getTimeString(clientCertExpiration * 1000) + ", address " + clientInfo->ipAddress + ", port "
                              + std::to_string(clientInfo->port) + ").");
        clientInfo->authenticated = true;
        clientInfo->certificateDn = clientCertDn;
        clientInfo->certificateSerial = clientCertSerial;
        clientInfo->acl = Acl::fromSerial(clientCertSerial);

        auto signedInfo = HelperFunctions::getDataFromDn(clientCertDn);
        if (signedInfo && !signedInfo->structValue->empty()) {
          auto typeIterator = signedInfo->structValue->find("type");
          if (typeIterator != signedInfo->structValue->end()) {
            auto type = typeIterator->second->stringValue;
            Gd::out.printInfo("Info (" + clientCertSerial + "): Certificate type in DN: " + type);
            if (!clientInfo->acl) {
              clientInfo->acl = Acl::fromType(type);
              if (!clientInfo->acl) {
                Gd::out.printInfo("Info (" + clientCertSerial + "): No ACL for type found. Assigning default ACL.");
                clientInfo->acl = Acl::fromType("default");
              }
            } else {
              Gd::out.printInfo("Info (" + clientCertSerial + "): Not reading ACL for type, because certificate already has an indiviual ACL assigned.");
            }
          }
        } else if (!clientInfo->acl) {
          clientInfo->acl = Acl::fromType("default");
        }
      } else {
        Gd::out.printWarning("Warning: Client " + std::to_string(clientId) + " has invalid or no certificate (DN " + clientCertDn + ", serial " + clientCertSerial + "). Closing connection.");
      }

      //Check ACL
      if (clientInfo->authenticated) {
        if (!clientInfo->acl) {
          Gd::out.printCritical("Critical (" + clientCertSerial + "): No ACL found. Revoking authentication.");
          clientInfo->authenticated = false;
        } else {
          if (!clientInfo->acl->hasPathAccess(http.getHeader().path, http.getHeader().method)) {
            Gd::out.printWarning("Warning (" + clientCertSerial + "): Access is denied by ACL. Revoking authentication.");
            clientInfo->authenticated = false;
          }
        }
      }

      //Login was unsuccessful. Send 401 and close connection.
      if (!clientInfo->authenticated) {
        Gd::out.printInfo("Info: Authentication failed. Source address: " + clientInfo->ipAddress + ", port: " + std::to_string(clientInfo->port));
        _httpServer->send(clientId, getError(401, "Unauthorized", "Unauthorized."), true);
        return;
      }
    }

    { //Handle requests that require authentication
      auto handlersPrivateIterator = _handlersPrivate->find(http.getHeader().method);
      if (handlersPrivateIterator != _handlersPrivate->end()) {
        if (executePrivateHandler(handlersPrivateIterator->second, restInfo)) return;
      }
    }

    _httpServer->send(restInfo->clientInfo->id, getError(404, "Not found", "The requested URL " + restInfo->pathString + " was not found on this server."), true);

    return;
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }

  try {
    _httpServer->send(clientId, getError(500, "Server error", "Unknown server error."), true);
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

bool RestServer::executePublicHandler(const std::shared_ptr<std::unordered_map<std::string, std::function<BaseLib::PVariable(const PRestInfo &, bool &handled)>>> &handlers, const PRestInfo &restInfo) {
  auto handlerIterator = handlers->find(restInfo->path.at(3));
  if (handlerIterator != handlers->end()) {
    bool handled = false;
    auto result = handlerIterator->second(restInfo, handled);
    if (!handled) return false;
    if (result->errorStruct) {
      if (result->structValue->at("faultCode")->integerValue == 302) {
        std::string response = "HTTP/1.1 302 Found\r\nLocation: " + result->structValue->at("faultString")->stringValue + "\r\n\r\n";
        C1Net::TcpPacket responsePacket(response.begin(), response.end());
        _httpServer->send(restInfo->clientInfo->id, responsePacket, true);
      } else
        _httpServer->send(restInfo->clientInfo->id, getError(result->structValue->at("faultCode")->integerValue, result->structValue->at("faultString")->stringValue, result->structValue->at("faultString")->stringValue), true);
    } else if (result->type == BaseLib::VariableType::tVoid) {
      _httpServer->send(restInfo->clientInfo->id, getError(404, "Not found", "The requested URL " + restInfo->pathString + " was not found on this server."), true);
    } else _httpServer->send(restInfo->clientInfo->id, generateResponse(result), restInfo->clientInfo->closeConnection);
    return true;
  }
  return false;
}

bool RestServer::executePrivateHandler(const std::shared_ptr<std::unordered_map<std::string, std::function<BaseLib::PVariable(const PRestInfo &)>>> &handlers, const PRestInfo &restInfo) {
  auto handlerIterator = handlers->find(restInfo->path.at(3));
  if (handlerIterator != handlers->end()) {
    auto result = handlerIterator->second(restInfo);
    if (result->errorStruct) {
      if (result->structValue->at("faultCode")->integerValue == 302) {
        std::string response = "HTTP/1.1 302 Found\r\nLocation: " + result->structValue->at("faultString")->stringValue + "\r\n\r\n";
        C1Net::TcpPacket responsePacket(response.begin(), response.end());
        _httpServer->send(restInfo->clientInfo->id, responsePacket, true);
      } else
        _httpServer->send(restInfo->clientInfo->id, getError(result->structValue->at("faultCode")->integerValue, result->structValue->at("faultString")->stringValue, result->structValue->at("faultString")->stringValue), true);
    } else if (result->type == BaseLib::VariableType::tVoid) {
      _httpServer->send(restInfo->clientInfo->id, getError(404, "Not found", "The requested URL " + restInfo->pathString + " was not found on this server."), true);
    } else _httpServer->send(restInfo->clientInfo->id, generateResponse(result), restInfo->clientInfo->closeConnection);
    return true;
  }
  return false;
}

void RestServer::bind() {
  try {
    BaseLib::HttpServer::HttpServerInfo serverInfo;
    serverInfo.connectionBacklogSize = Gd::settings.connectionBacklogSize();
    serverInfo.maxConnections = Gd::settings.maxConnections();
    serverInfo.serverThreads = Gd::settings.serverThreads();
    serverInfo.processingThreads = Gd::settings.processingThreads();

    serverInfo.listenAddress = Gd::settings.httpListenAddress();
    serverInfo.port = Gd::settings.httpListenPort();

    serverInfo.useSsl = Gd::settings.httpSsl();
    if (serverInfo.useSsl) {
      C1Net::PCertificateInfo certificateInfo = std::make_shared<C1Net::CertificateInfo>();
      certificateInfo->cert_file = Gd::settings.httpCertFile();
      certificateInfo->key_file = Gd::settings.httpKeyFile();
      certificateInfo->ca_file = Gd::settings.httpClientCertCaFile();
      serverInfo.certificates.emplace("*", certificateInfo);
    }

    serverInfo.newConnectionCallback = std::bind(&RestServer::newConnection, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    serverInfo.connectionClosedCallback = std::bind(&RestServer::connectionClosed, this, std::placeholders::_1);
    serverInfo.packetReceivedCallback = std::bind(&RestServer::packetReceived, this, std::placeholders::_1, std::placeholders::_2);

    _httpServer = std::make_shared<BaseLib::HttpServer>(Gd::bl.get(), serverInfo);
    _httpServer->bind();
    Gd::out.printInfo("Info: Server bound.");
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void RestServer::start() {
  try {
    _httpServer->start();
    Gd::out.printInfo("Info: Started listening.");
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void RestServer::stop() {
  try {
    if (_httpServer) {
      _httpServer->stop();
      _httpServer->waitForStop();
    }
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void RestServer::reload() {
  if (_httpServer) _httpServer->reload();
}

void RestServer::newConnection(int32_t clientId, std::string address, uint16_t port) {
  try {
    if (Gd::bl->debugLevel >= 4) Gd::out.printInfo("Info: New connection from " + address + " on port " + std::to_string(port) + ". Client ID is: " + std::to_string(clientId));
    PClientInfo clientInfo = std::make_shared<RestClientInfo>();
    clientInfo->id = clientId;
    clientInfo->ipAddress = address;
    clientInfo->port = port;

    std::lock_guard<std::shared_mutex> clientInfoGuard(_clientInfoMutex);
    _clientInfo[clientId] = std::move(clientInfo);
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void RestServer::connectionClosed(int32_t clientId) {
  try {
    if (Gd::bl->debugLevel >= 5) Gd::out.printDebug("Debug: Connection to client " + std::to_string(clientId) + " closed.");

    std::lock_guard<std::shared_mutex> clientInfoGuard(_clientInfoMutex);
    _clientInfo.erase(clientId);
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

}