/* Copyright 2013-2019 Homegear GmbH */

#ifndef MELLON_API_SRC_DATABASE_H_
#define MELLON_API_SRC_DATABASE_H_

#include "Sqlite3.h"

#include <homegear-base/IQueue.h>
#include <homegear-base/Variable.h>

namespace MellonApi {

class Database : public BaseLib::IQueue {
 private:
  class QueueEntry : public BaseLib::IQueueEntry {
   public:
    QueueEntry(const std::string &command, BaseLib::Database::DataRow &data) { _entry = std::make_shared<std::pair<std::string, BaseLib::Database::DataRow>>(command, data); };

    ~QueueEntry() override = default;;

    std::shared_ptr<std::pair<std::string, BaseLib::Database::DataRow>> &getEntry() { return _entry; }
   private:
    std::shared_ptr<std::pair<std::string, BaseLib::Database::DataRow>> _entry;
  };

  std::atomic_bool _disposing{true};
  Sqlite3 _db;

  void processQueueEntry(int32_t index, std::shared_ptr<BaseLib::IQueueEntry> &entry) override;
 public:
  Database();
  ~Database() override;

  void init();
  void dispose();

  // {{{ General
  void open(const std::string& databasePath, std::string databaseFilename, bool databaseSynchronous, bool databaseMemoryJournal, bool databaseWALJournal, std::string backupPath, std::string backupFilename);

  void hotBackup();

  bool isOpen() { return _db.isOpen(); }

  void initializeDatabase();

  bool convertDatabase();

  void createSavepointSynchronous(const std::string &name);

  void releaseSavepointSynchronous(const std::string &name);

  void createSavepointAsynchronous(const std::string &name);

  void releaseSavepointAsynchronous(const std::string &name);
  // }}}

  // {{{ Certificates
  BaseLib::PVariable getCertificateAcl(const std::string &serial);
  BaseLib::PVariable getCrl();
  std::string getTokenDn(const std::string &token, int32_t &certificateSlot);
  BaseLib::PVariable getTypeAcl(const std::string &type);
  bool insertCertificate(const std::string &serial, const std::string &csr, const std::string &certificate, int64_t expirationTime, const std::string &acl);
  bool insertToken(const std::string &token, const std::string &dn, int64_t expirationTime, int32_t certificateSlot);
  bool isCertificateValid(const std::string &serial);
  void removeToken(const std::string &token);
  // }}}
};

}

#endif //MELLON_API_SRC_DATABASE_H_
