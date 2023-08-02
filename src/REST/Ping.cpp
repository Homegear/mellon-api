//
// Created by sathya on 08.10.19.
//

#include "Ping.h"
#include <homegear-base/HelperFunctions/HelperFunctions.h>

namespace MellonApi::Rest {

/**
 * @api {get} /ping Check if the API is available.
 * @apiVersion 1.0.0
 * @apiName Ping
 * @apiGroup Ping
 * @apiPermission none
 *
 * @apiDescription Checks if the API server is available. Returns the current UTC time as unix epoch timestamp.
 *
 * @apiSuccess (200) {Boolean} success ``true`` when no errors occured.
 * @apiSuccess (200) {Number}  result  The current UTC unix epoch.
 */
BaseLib::PVariable Ping::getPublic(const PRestInfo &restInfo, bool &handled) {
  handled = true;

  BaseLib::PVariable resultJson = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
  resultJson->structValue->emplace("success", std::make_shared<BaseLib::Variable>(true));
  resultJson->structValue->emplace("result", std::make_shared<BaseLib::Variable>(BaseLib::HelperFunctions::getTime()));

  return resultJson;
}

}