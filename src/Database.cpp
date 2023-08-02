/* Copyright 2013-2019 Homegear GmbH */

#include "Database.h"

#include <memory>
#include <utility>
#include <sys/stat.h>
#include "Gd.h"

namespace MellonApi {

Database::Database() : IQueue(Gd::bl.get(), 1, 100000) {

}

Database::~Database() {
  dispose();
}

void Database::init() {
  if (!Gd::bl) {
    Gd::out.printCritical("Critical: Could not initialize database controller, because base library is not initialized.");
    return;
  }

  _disposing = false;

  startQueue(0, true, 1, 0, SCHED_OTHER);
}

void Database::dispose() {
  if (_disposing) return;
  _disposing = true;
  stopQueue(0);
  _db.dispose();
}

void Database::processQueueEntry(int32_t index, std::shared_ptr<BaseLib::IQueueEntry> &entry) {
  std::shared_ptr<QueueEntry> queueEntry = std::dynamic_pointer_cast<QueueEntry>(entry);
  if (!queueEntry) return;
  _db.executeWriteCommand(queueEntry->getEntry());
}

//{{{ General
void Database::open(const std::string &databasePath, std::string databaseFilename, bool databaseSynchronous, bool databaseMemoryJournal, bool databaseWALJournal, std::string backupPath, std::string backupFilename) {
  _db.init(databasePath, std::move(databaseFilename), databaseSynchronous, databaseMemoryJournal, databaseWALJournal, std::move(backupPath), std::move(backupFilename));
}

void Database::hotBackup() {
  _db.hotBackup();
}

void Database::initializeDatabase() {
  try {
    _db.executeCommand("CREATE TABLE IF NOT EXISTS mellonApiVariables (variableId INTEGER PRIMARY KEY UNIQUE, variableIndex INTEGER NOT NULL, integerValue INTEGER, stringValue TEXT, binaryValue BLOB)");
    _db.executeCommand("CREATE INDEX IF NOT EXISTS mellonApiVariablesIndex ON mellonApiVariables (variableId, variableIndex)");
    _db.executeCommand("CREATE TABLE IF NOT EXISTS typeAcls (id INTEGER PRIMARY KEY UNIQUE, type TEXT, acl BLOB)");
    _db.executeCommand("CREATE INDEX IF NOT EXISTS typeAclsIndex ON typeAcls (id, type)");
    _db.executeCommand("CREATE TABLE IF NOT EXISTS signedCertificates (id INTEGER PRIMARY KEY UNIQUE, serial TEXT, csr TEXT, certificate TEXT, acl BLOB, revoked INTEGER, expirationTime INTEGER)");
    _db.executeCommand("CREATE INDEX IF NOT EXISTS signedCertificatesIndex ON signedCertificates (id, serial, revoked)");
    _db.executeCommand("CREATE TABLE IF NOT EXISTS certificateTokens (id INTEGER PRIMARY KEY UNIQUE, token TEXT, dn TEXT, expirationTime INTEGER, certificateSlot Integer)");
    _db.executeCommand("CREATE INDEX IF NOT EXISTS certificateTokensIndex ON certificateTokens (id, token, expirationTime)");

    {
      BaseLib::Database::DataRow data;
      data.push_back(std::make_shared<BaseLib::Database::DataColumn>(0));
      auto result = _db.executeCommand("SELECT 1 FROM mellonApiVariables WHERE variableIndex=?", data);
      if (result->empty()) {
        data.clear();
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(0));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>("0.1.0"));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        _db.executeCommand("INSERT INTO mellonApiVariables VALUES(?, ?, ?, ?, ?)", data);
      }
    }

    {
      auto acl = getTypeAcl("default");
      if (!acl) {
        Gd::out.printInfo("Info: Creating default ACL.");
        acl = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        auto pathAccess = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        acl->structValue->emplace("pathAccess", pathAccess);

        auto methods = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
        methods->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>("GET"));
        pathAccess->structValue->emplace("/api/v1/crl", methods);

        methods = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
        methods->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>("POST"));
        pathAccess->structValue->emplace("/api/v1/ca/6/sign", methods);

        std::vector<char> json;
        BaseLib::Rpc::JsonEncoder::encode(acl, json);

        BaseLib::Database::DataRow data;
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>("default"));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(json));
        _db.executeCommand("REPLACE INTO typeAcls VALUES(?, ?, ?)", data);
      }

      acl = getTypeAcl("password-manager");
      if (!acl) {
        Gd::out.printInfo("Info: Creating ACL for type password-manager.");
        acl = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        auto pathAccess = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        acl->structValue->emplace("pathAccess", pathAccess);

        auto methods = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
        methods->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>("GET"));
        pathAccess->structValue->emplace("/api/v1/crl", methods);

        methods = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
        methods->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>("POST"));
        pathAccess->structValue->emplace("/api/v1/aes/0/encrypt", methods);
        pathAccess->structValue->emplace("/api/v1/aes/0/decrypt", methods);
        pathAccess->structValue->emplace("/api/v1/ca/1/sign", methods);

        std::vector<char> json;
        BaseLib::Rpc::JsonEncoder::encode(acl, json);

        BaseLib::Database::DataRow data;
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>("password-manager"));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(json));
        _db.executeCommand("REPLACE INTO typeAcls VALUES(?, ?, ?)", data);
      }

      acl = getTypeAcl("mellon-maintenance");
      if (!acl) {
        Gd::out.printInfo("Info: Creating ACL for type mellon-maintenance.");
        acl = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        auto pathAccess = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        acl->structValue->emplace("pathAccess", pathAccess);

        auto methods = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
        methods->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>("GET"));
        pathAccess->structValue->emplace("/api/v1/crl", methods);

        methods = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
        methods->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>("POST"));
        pathAccess->structValue->emplace("/api/v1/ca/1/sign", methods);
        pathAccess->structValue->emplace("/api/v1/mellon", methods);

        std::vector<char> json;
        BaseLib::Rpc::JsonEncoder::encode(acl, json);

        BaseLib::Database::DataRow data;
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>("mellon-maintenance"));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(json));
        _db.executeCommand("REPLACE INTO typeAcls VALUES(?, ?, ?)", data);
      }

      acl = getTypeAcl("apartment");
      if (!acl) {
        Gd::out.printInfo("Info: Creating ACL for type apartment.");
        acl = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        auto pathAccess = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        acl->structValue->emplace("pathAccess", pathAccess);

        auto methods = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
        methods->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>("GET"));
        pathAccess->structValue->emplace("/api/v1/crl", methods);

        methods = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
        methods->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>("POST"));
        pathAccess->structValue->emplace("/api/v1/ca/2/sign", methods);

        std::vector<char> json;
        BaseLib::Rpc::JsonEncoder::encode(acl, json);

        BaseLib::Database::DataRow data;
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>("apartment"));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(json));
        _db.executeCommand("REPLACE INTO typeAcls VALUES(?, ?, ?)", data);
      }
    }
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

bool Database::convertDatabase() {
  try {
    std::string databasePath = Gd::bl->settings.databasePath();
    if (databasePath.empty()) databasePath = Gd::bl->settings.dataPath();
    std::string databaseFilename = "db.sqlite";
    std::string backupPath = Gd::bl->settings.databaseBackupPath();
    if (backupPath.empty()) backupPath = databasePath;
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(std::string("table")));
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(std::string("mellonApiVariables")));
    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT 1 FROM sqlite_master WHERE type=? AND name=?", data);
    //Cannot proceed, because table mellonApiVariables does not exist
    if (rows->empty()) return false;
    data.clear();
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(0));
    std::shared_ptr<BaseLib::Database::DataTable> result = _db.executeCommand("SELECT * FROM mellonApiVariables WHERE variableIndex=?", data);
    if (result->empty()) return false; //Handled in initializeDatabase
    int64_t versionId = result->at(0).at(0)->intValue;
    std::string version = result->at(0).at(3)->textValue;

    if (version == "0.1.0") return false; //Up to date

    return false;
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return true;
}

void Database::createSavepointSynchronous(const std::string &name) {
  if (Gd::bl->debugLevel > 5) Gd::out.printDebug("Debug: Creating savepoint (synchronous) " + name);
  BaseLib::Database::DataRow data;
  _db.executeWriteCommand("SAVEPOINT " + name, data);
}

void Database::releaseSavepointSynchronous(const std::string &name) {
  if (Gd::bl->debugLevel > 5) Gd::out.printDebug("Debug: Releasing savepoint (synchronous) " + name);
  BaseLib::Database::DataRow data;
  _db.executeWriteCommand("RELEASE " + name, data);
}

void Database::createSavepointAsynchronous(const std::string &name) {
  if (Gd::bl->debugLevel > 5) Gd::out.printDebug("Debug: Creating savepoint (asynchronous) " + name);
  BaseLib::Database::DataRow data;
  std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("SAVEPOINT " + name, data);
  enqueue(0, entry);
}

void Database::releaseSavepointAsynchronous(const std::string &name) {
  if (Gd::bl->debugLevel > 5) Gd::out.printDebug("Debug: Releasing savepoint (asynchronous) " + name);
  BaseLib::Database::DataRow data;
  std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("RELEASE " + name, data);
  enqueue(0, entry);
}
// }}} General

// {{{ Certificates
bool Database::isCertificateValid(const std::string &serial) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(serial));
    auto rows = _db.executeCommand("SELECT revoked, expirationTime FROM signedCertificates WHERE serial=?", data);
    if (rows->empty() || rows->at(0).empty()) return true;
    for (auto &row : *rows) {
      if ((bool)row.second.at(0)->intValue || row.second.at(1)->intValue < BaseLib::HelperFunctions::getTimeSeconds()) return false;
    }
    return true;
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return false;
}

BaseLib::PVariable Database::getCertificateAcl(const std::string &serial) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(serial));
    auto rows = _db.executeCommand("SELECT acl FROM signedCertificates WHERE serial=?", data);
    if (rows->empty() || rows->at(0).empty()) return {};
    if (!rows->at(0).at(0)->binaryValue->empty()) return BaseLib::Rpc::JsonDecoder::decode(*rows->at(0).at(0)->binaryValue);
    else if (!rows->at(0).at(0)->textValue.empty()) return BaseLib::Rpc::JsonDecoder::decode(rows->at(0).at(0)->textValue);
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::PVariable();
}

BaseLib::PVariable Database::getTypeAcl(const std::string &type) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(type));
    auto rows = _db.executeCommand("SELECT acl FROM typeAcls WHERE type=?", data);
    if (rows->empty() || rows->at(0).empty()) return BaseLib::PVariable();
    if (!rows->at(0).at(0)->binaryValue->empty()) return BaseLib::Rpc::JsonDecoder::decode(*rows->at(0).at(0)->binaryValue);
    else if (!rows->at(0).at(0)->textValue.empty()) return BaseLib::Rpc::JsonDecoder::decode(rows->at(0).at(0)->textValue);
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::PVariable();
}

BaseLib::PVariable Database::getCrl() {
  try {
    auto crl = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);

    auto rows = _db.executeCommand("SELECT serial FROM signedCertificates WHERE revoked=1");
    crl->arrayValue->reserve(rows->size());
    for (auto &row : *rows) {
      crl->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(row.second.at(0)->textValue));
    }
    return crl;
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
}

std::string Database::getTokenDn(const std::string &token, int32_t &certificateSlot) {
  try {
    certificateSlot = -1;
    _db.executeCommand("DELETE FROM certificateTokens WHERE expirationTime<" + std::to_string(BaseLib::HelperFunctions::getTimeSeconds()));

    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(token));
    auto rows = _db.executeCommand("SELECT dn, certificateSlot FROM certificateTokens WHERE token=?", data);
    if (rows->empty() || rows->at(0).empty()) return "";
    certificateSlot = rows->at(0).at(1)->intValue;
    return rows->at(0).at(0)->textValue;
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return "";
}

bool Database::insertCertificate(const std::string &serial, const std::string &csr, const std::string &certificate, int64_t expirationTime, const std::string &acl) {
  try {
    if (!acl.empty()) {
      //Check JSON, throws exception when JSON is invalid.
      auto aclJson = BaseLib::Rpc::JsonDecoder::decode(acl);
      if (!aclJson) return false;
    }

    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>()); //id
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(serial));
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(csr));
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(certificate));
    if (acl.empty()) data.push_back(std::make_shared<BaseLib::Database::DataColumn>()); //acl
    else data.push_back(std::make_shared<BaseLib::Database::DataColumn>(std::vector<uint8_t>(acl.begin(), acl.end()))); //acl
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(0)); //revoked
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(expirationTime)); //expirationTime
    _db.executeCommand("REPLACE INTO signedCertificates VALUES(?, ?, ?, ?, ?, ?, ?)", data);
    return true;
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return false;
}

bool Database::insertToken(const std::string &token, const std::string &dn, int64_t expirationTime, int32_t certificateSlot) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>()); //id
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(token));
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(dn));
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(expirationTime));
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(certificateSlot));
    _db.executeCommand("REPLACE INTO certificateTokens VALUES(?, ?, ?, ?, ?)", data);
    return true;
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return false;
}

void Database::removeToken(const std::string &token) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(token));
    _db.executeCommand("DELETE FROM certificateTokens WHERE token=?", data);
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}
// }}}
}