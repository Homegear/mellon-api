//
// Created by SLaufer on 12.08.20.
//

#include "Decrypt.h"
#include "../../../Gd.h"
#include "../../../HelperFunctions.h"

namespace MellonApi::Rest::AesChild::AesKeySlotChild {

BaseLib::PVariable Decrypt::post(const MellonApi::PRestInfo &restInfo, uint32_t aesKeySlot) {
  auto dataIterator = restInfo->data->structValue->find("data");
  if (dataIterator == restInfo->data->structValue->end()) return RestServer::getJsonErrorResult("request_error");

  std::vector<char> data;
  BaseLib::Base64::decode(dataIterator->second->stringValue, data);

  if (data.size() > 100000) return RestServer::getJsonErrorResult("request_error");

  auto decryptedData = Gd::mellon->aesDecrypt(data, aesKeySlot);
  if (decryptedData.empty()) return RestServer::getJsonErrorResult("request_error");

  std::string decryptedBase64;
  BaseLib::Base64::encode(decryptedData, decryptedBase64);

  return RestServer::getJsonSuccessResult(std::make_shared<BaseLib::Variable>(decryptedBase64));
}

}