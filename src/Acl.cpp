//
// Created by SLaufer on 12.08.20.
//

#include "Acl.h"
#include "Gd.h"

namespace MellonApi {

Acl::Acl() = default;

Acl::Acl(const BaseLib::PVariable &aclStruct) {
  if (aclStruct) {
    BaseLib::Rpc::JsonEncoder::encode(aclStruct, _serializedAcl);

    auto aclIterator = aclStruct->structValue->find("pathAccess");
    if (aclIterator != aclStruct->structValue->end()) {
      for (auto &path : *aclIterator->second->structValue) {
        for (auto &method : *path.second->arrayValue) {
          _pathAccess[path.first].emplace(method->stringValue);
        }
      }
    }

    aclIterator = aclStruct->structValue->find("typeSignAccess");
    if (aclIterator != aclStruct->structValue->end()) {
      for (auto &type : *aclIterator->second->arrayValue) {
        _typeSignAccess.emplace(type->stringValue);
      }
    }

    aclIterator = aclStruct->structValue->find("tokenCreationAccess");
    if (aclIterator != aclStruct->structValue->end()) {
      for (auto &type : *aclIterator->second->arrayValue) {
        _tokenCreationAccess.emplace(type->stringValue);
      }
    }
  }
}

bool Acl::isEmpty() {
  return _pathAccess.empty() && _typeSignAccess.empty();
}

std::shared_ptr<Acl> Acl::fromSerial(const std::string &serial) {
  auto acl = std::make_shared<Acl>(Gd::db->getCertificateAcl(serial));
  if (acl->isEmpty()) acl.reset();
  return acl;
}

std::shared_ptr<Acl> Acl::fromType(const std::string &type) {
  auto acl = std::make_shared<Acl>(Gd::db->getTypeAcl(type));
  if (acl->isEmpty()) acl.reset();
  return acl;
}

std::string Acl::serialize() {
  return _serializedAcl;
}

bool Acl::hasPathAccess(const std::string &path, const std::string &method) {
  if (path.empty() || method.empty()) return false;
  auto pathIterator = _pathAccess.find(path);
  if (pathIterator == _pathAccess.end()) {
    pathIterator = _pathAccess.find("*");
    if (pathIterator == _pathAccess.end()) {
      return false;
    }
  }
  auto methodIterator = pathIterator->second.find(method);
  return methodIterator != pathIterator->second.end();
}

bool Acl::hasTypeSignAccess(const std::string &type) {
  if (type.empty()) return false;
  bool hasAccess = _typeSignAccess.find(type) != _typeSignAccess.end();
  if (!hasAccess) hasAccess = _typeSignAccess.find("*") != _typeSignAccess.end();
  return hasAccess;
}

bool Acl::hasTokenCreationAccess(const std::string &type) {
  if (type.empty()) return false;
  bool hasAccess = _tokenCreationAccess.find(type) != _tokenCreationAccess.end();
  if (!hasAccess) hasAccess = _tokenCreationAccess.find("*") != _tokenCreationAccess.end();
  return hasAccess;
}

}