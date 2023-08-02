//
// Created by SLaufer on 12.08.20.
//

#include "Encrypt.h"
#include "../../../Gd.h"
#include "../../../HelperFunctions.h"

namespace MellonApi::Rest::AesChild::AesKeySlotChild {

BaseLib::PVariable Encrypt::post(const MellonApi::PRestInfo &restInfo, uint32_t aesKeySlot) {
  auto dataIterator = restInfo->data->structValue->find("data");
  if (dataIterator == restInfo->data->structValue->end()) return RestServer::getJsonErrorResult("request_error");

  std::vector<char> data;
  BaseLib::Base64::decode(dataIterator->second->stringValue, data);

  if (data.size() > 100000) return RestServer::getJsonErrorResult("request_error");

  auto encryptedData = Gd::mellon->aesEncrypt(data, aesKeySlot);
  if (encryptedData.empty()) return RestServer::getJsonErrorResult("request_error");

  std::string encryptedBase64;
  BaseLib::Base64::encode(encryptedData, encryptedBase64);

  return RestServer::getJsonSuccessResult(std::make_shared<BaseLib::Variable>(encryptedBase64));
}

}