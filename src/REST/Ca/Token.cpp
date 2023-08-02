//
// Created by SLaufer on 12.08.20.
//

#include "Token.h"
#include "Token/Create.h"

#include "../../Gd.h"

namespace MellonApi::Rest::CaChild {

std::shared_ptr<std::unordered_map<std::string, std::function<BaseLib::PVariable(const PRestInfo &)>>> Token::_postHandlersPrivate;

void Token::init() {
  _postHandlersPrivate = std::make_shared<std::unordered_map<std::string, std::function<BaseLib::PVariable(const PRestInfo &)>>>();
  _postHandlersPrivate->emplace("create", std::bind(&TokenChild::Create::post, std::placeholders::_1));
}

BaseLib::PVariable Token::getPublic(const PRestInfo &restInfo, bool &handled) {
  handled = true;

  auto tokenIterator = restInfo->queryParameters.find("token");
  if (tokenIterator == restInfo->queryParameters.end()) {
    return RestServer::getJsonErrorResult("request_error");
  }

  int32_t certificateSlot = 0;
  auto dn = Gd::db->getTokenDn(tokenIterator->second, certificateSlot);
  if (dn.empty()) return RestServer::getJsonErrorResult("request_error");

  return RestServer::getJsonSuccessResult(std::make_shared<BaseLib::Variable>(dn));
}

BaseLib::PVariable Token::postPublic(const PRestInfo &restInfo, bool &handled) {
  if(restInfo->path.size() >= 6) {
    handled = false;
    return RestServer::getJsonErrorResult("request_error");
  }

  handled = true;

  auto dataIterator = restInfo->data->structValue->find("token");
  if (dataIterator == restInfo->data->structValue->end()) {
    return RestServer::getJsonErrorResult("request_error");
  }
  auto token = dataIterator->second->stringValue;

  int32_t certificateSlot = 0;
  auto dn = Gd::db->getTokenDn(token, certificateSlot);
  if (dn.empty()) return RestServer::getJsonErrorResult("request_error");

  dataIterator = restInfo->data->structValue->find("csr");
  if (dataIterator == restInfo->data->structValue->end()) {
    return RestServer::getJsonErrorResult("request_error");
  }

  auto &crqPem = dataIterator->second->stringValue;
  std::string crqDn;

  {
    gnutls_x509_crq_t crq;
    gnutls_x509_crq_init(&crq);

    gnutls_datum_t certificateData;
    certificateData.data = (unsigned char *)crqPem.data();
    certificateData.size = crqPem.size();

    if (gnutls_x509_crq_import(crq, &certificateData, gnutls_x509_crt_fmt_t::GNUTLS_X509_FMT_PEM) == GNUTLS_E_SUCCESS) {
      std::array<char, 1024> buffer{};
      size_t bufferSize = buffer.size();
      if (gnutls_x509_crq_get_dn(crq, buffer.data(), &bufferSize) == 0) {
        crqDn = std::string(buffer.begin(), buffer.begin() + bufferSize);
      }
    } else {
      Gd::out.printError("Error (" + restInfo->clientInfo->certificateSerial + "): Could not import CSR.");
    }

    gnutls_x509_crq_deinit(crq);
  }

  if(crqDn != dn) {
    Gd::out.printError("Error (" + restInfo->clientInfo->certificateSerial + "): DN of CSR does not match DN in database (" + crqDn + " != " + dn + ").");
    return RestServer::getJsonErrorResult("request_error");
  }

  auto signedCertificate = Gd::mellon->signx509csr(crqPem, certificateSlot);
  if (signedCertificate.empty()) return RestServer::getJsonErrorResult("request_error");

  int64_t expirationTime = 0;
  std::string serial;
  bool error = false;

  {
    gnutls_x509_crt_t certificate;
    gnutls_x509_crt_init(&certificate);

    gnutls_datum_t certificateData;
    certificateData.data = (unsigned char *)signedCertificate.data();
    certificateData.size = signedCertificate.size();

    if (gnutls_x509_crt_import(certificate, &certificateData, gnutls_x509_crt_fmt_t::GNUTLS_X509_FMT_PEM) == GNUTLS_E_SUCCESS) {
      std::array<char, 1024> buffer{};
      size_t bufferSize = buffer.size();
      expirationTime = gnutls_x509_crt_get_expiration_time(certificate);
      if(gnutls_x509_crt_get_serial(certificate, buffer.data(), &bufferSize) == GNUTLS_E_SUCCESS) {
        if (bufferSize > buffer.size()) bufferSize = buffer.size();
        serial = BaseLib::HelperFunctions::getHexString(buffer.data(), bufferSize);
      }
    } else {
      Gd::out.printError("Error (" + restInfo->clientInfo->certificateSerial + "): Could not import signed certificate.");
      error = true;
    }

    gnutls_x509_crt_deinit(certificate);
  }

  if (error || expirationTime == 0 || serial.empty()) return RestServer::getJsonErrorResult("request_error");

  if (!Gd::db->insertCertificate(serial, crqPem, signedCertificate, expirationTime, "")) {
    return RestServer::getJsonErrorResult("request_error");
  }

  Gd::db->removeToken(token);

  return RestServer::getJsonSuccessResult(std::make_shared<BaseLib::Variable>(signedCertificate));
}

BaseLib::PVariable Token::postPrivate(const PRestInfo &restInfo, uint32_t dummy) {
  if (restInfo->path.size() >= 6) {
    auto handlerIterator = _postHandlersPrivate->find(restInfo->path.at(5));
    if (handlerIterator != _postHandlersPrivate->end()) {
      return handlerIterator->second(restInfo);
    }
  }

  return RestServer::getJsonErrorResult("request_error");
}

}