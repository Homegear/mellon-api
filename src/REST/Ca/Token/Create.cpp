//
// Created by SLaufer on 12.08.20.
//

#include "Create.h"
#include "../../../Gd.h"
#include "../../../HelperFunctions.h"

namespace MellonApi::Rest::CaChild::TokenChild {

BaseLib::PVariable Create::post(const MellonApi::PRestInfo &restInfo) {
  auto dataIterator = restInfo->data->structValue->find("cn");
  if (dataIterator == restInfo->data->structValue->end()) {
    Gd::out.printWarning("Warning (" + restInfo->clientInfo->certificateSerial + "): Data does not contain element \"cn\".");
    return RestServer::getJsonErrorResult("request_error");
  }
  bool jsonCn = true;
  std::string cn = BaseLib::HelperFunctions::trim(dataIterator->second->stringValue);
  if (cn.empty()) {
    Gd::out.printWarning("Warning (" + restInfo->clientInfo->certificateSerial + "): CN is empty.");
    return RestServer::getJsonErrorResult("request_error");
  }
  BaseLib::PVariable cnJson;
  if (cn.front() == '{') {
    try {
      cnJson = BaseLib::Rpc::JsonDecoder::decode(cn);
    } catch (const std::exception &ex) {
      Gd::out.printWarning("Warning (" + restInfo->clientInfo->certificateSerial + "): Error decoding CN.");
      return RestServer::getJsonErrorResult("request_error");
    }
  } else jsonCn = false;

  int64_t expirationTime = BaseLib::HelperFunctions::getTimeSeconds() + 2592000;
  dataIterator = restInfo->data->structValue->find("expirationTime");
  if (dataIterator != restInfo->data->structValue->end()) {
    expirationTime = dataIterator->second->integerValue64;
    if (expirationTime > BaseLib::HelperFunctions::getTimeSeconds() + 31708800) {
      Gd::out.printWarning("Warning (" + restInfo->clientInfo->certificateSerial + "): Invalid expiration time.");
      expirationTime = BaseLib::HelperFunctions::getTimeSeconds() + 31536000;
    }
  }

  int32_t certificateSlot = -1;
  dataIterator = restInfo->data->structValue->find("certificateSlot");
  if (dataIterator == restInfo->data->structValue->end()) { //10 years RSA
    Gd::out.printWarning("Warning (" + restInfo->clientInfo->certificateSerial + "): Invalid certificate slot.");
  }
  certificateSlot = dataIterator->second->integerValue64;

  if (jsonCn) {
    auto typeIterator = cnJson->structValue->find("type");
    if (typeIterator == cnJson->structValue->end() || typeIterator->second->stringValue.empty()) {
      Gd::out.printWarning("Warning (" + restInfo->clientInfo->certificateSerial + "): CN does not contain type.");
      return RestServer::getJsonErrorResult("request_error");
    }

    if (!restInfo->clientInfo->acl->hasTokenCreationAccess(typeIterator->second->stringValue)) {
      Gd::out.printWarning("Error (" + restInfo->clientInfo->certificateSerial + "): Access to type " + typeIterator->second->stringValue + " not permitted.");
      return RestServer::getJsonErrorResult("request_error");
    }
  } else {
    if (!restInfo->clientInfo->acl->hasTokenCreationAccess("*")) {
      Gd::out.printWarning("Error (" + restInfo->clientInfo->certificateSerial + "): Access not permitted.");
      return RestServer::getJsonErrorResult("request_error");
    }
  }

  //Everything ok. Create token.
  auto token = BaseLib::HelperFunctions::getTimeUuid();
  std::string dn;
  BaseLib::Base64::encode(cn, dn);
  dn = "CN=" + dn;

  if (!Gd::db->insertToken(token, dn, expirationTime, certificateSlot)) {
    return RestServer::getJsonErrorResult("request_error");
  }
  return RestServer::getJsonSuccessResult(std::make_shared<BaseLib::Variable>(token));
}

}