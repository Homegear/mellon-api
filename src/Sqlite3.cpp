#include <utility>
#include <algorithm>

/* Copyright 2013-2020 Homegear GmbH
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * 
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with Homegear.  If not, see
 * <http://www.gnu.org/licenses/>.
 * 
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU Lesser General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
*/

#include "Sqlite3.h"
#include "Gd.h"

namespace MellonApi {

Sqlite3::Sqlite3() = default;

Sqlite3::Sqlite3(const std::string& databasePath, std::string databaseFilename, bool databaseSynchronous, bool databaseMemoryJournal, bool databaseWalJournal) : Sqlite3() {
  if (databasePath.empty()) return;
  _databaseSynchronous = databaseSynchronous;
  _databaseMemoryJournal = databaseMemoryJournal;
  _databaseWALJournal = databaseWalJournal;
  _databasePath = databasePath;
  _databaseFilename = std::move(databaseFilename);
  _backupPath = "";
  _backupFilename = "";
  openDatabase(true);
}

void Sqlite3::init(const std::string& databasePath, std::string databaseFilename, bool databaseSynchronous, bool databaseMemoryJournal, bool databaseWalJournal, std::string backupPath, std::string backupFilename) {
  if (databasePath.empty()) return;
  _databaseSynchronous = databaseSynchronous;
  _databaseMemoryJournal = databaseMemoryJournal;
  _databaseWALJournal = databaseWalJournal;
  _databasePath = databasePath;
  _databaseFilename = std::move(databaseFilename);
  _backupPath = std::move(backupPath);
  _backupFilename = std::move(backupFilename);
  hotBackup();
}

Sqlite3::~Sqlite3() {
  closeDatabase(true);
}

void Sqlite3::dispose() {
  closeDatabase(true);
}

void Sqlite3::hotBackup() {
  try {
    if (_databasePath.empty() || _databaseFilename.empty()) {
      Gd::out.printError("Error: Can't backup database: _databasePath or _databaseFilename is empty.");
      return;
    }
    std::lock_guard<std::mutex> databaseGuard(_databaseMutex);
    closeDatabase(false);
    if (BaseLib::Io::fileExists(_databasePath + _databaseFilename)) {
      if (!checkIntegrity(_databasePath + _databaseFilename)) {
        Gd::out.printCritical("Critical: Integrity check on database failed.");
        if (!_backupPath.empty() && !_backupFilename.empty()) {
          Gd::out.printCritical("Critical: Backing up corrupted database file to: " + _backupPath + _databaseFilename + ".broken");
          Gd::bl->io.copyFile(_databasePath + _databaseFilename, _backupPath + _databaseFilename + ".broken");
          bool restored = false;
          for (int32_t i = 0; i <= 10000; i++) {
            if (BaseLib::Io::fileExists(_backupPath + _backupFilename + std::to_string(i)) && checkIntegrity(_backupPath + _backupFilename + std::to_string(i))) {
              Gd::out.printCritical("Critical: Restoring database file: " + _backupPath + _backupFilename + std::to_string(i));
              if (Gd::bl->io.copyFile(_backupPath + _backupFilename + std::to_string(i), _databasePath + _databaseFilename)) {
                restored = true;
                break;
              }
            }
          }
          if (!restored) {
            Gd::out.printCritical("Critical: Could not restore database.");
            return;
          }
        } else return;
      } else {
        if (!_backupPath.empty() && !_backupFilename.empty()) {
          Gd::out.printInfo("Info: Backing up database...");
          if (Gd::settings.databaseMaxBackups() > 1) {
            if (BaseLib::Io::fileExists(_backupPath + _backupFilename + std::to_string(Gd::settings.databaseMaxBackups() - 1))) {
              if (!BaseLib::Io::deleteFile(_backupPath + _backupFilename + std::to_string(Gd::settings.databaseMaxBackups() - 1))) {
                Gd::out.printError("Error: Cannot delete file: " + _backupPath + _backupFilename + std::to_string(Gd::settings.databaseMaxBackups() - 1));
              }
            }
            for (int32_t i = Gd::settings.databaseMaxBackups() - 2; i >= 0; i--) {
              if (BaseLib::Io::fileExists(_backupPath + _backupFilename + std::to_string(i))) {
                if (!BaseLib::Io::moveFile(_backupPath + _backupFilename + std::to_string(i), _backupPath + _backupFilename + std::to_string(i + 1))) {
                  Gd::out.printError("Error: Cannot move file: " + _backupPath + _backupFilename + std::to_string(i));
                }
              }
            }
          }
          if (Gd::settings.databaseMaxBackups() > 0) {
            if (!Gd::bl->io.copyFile(_databasePath + _databaseFilename, _backupPath + _backupFilename + '0')) {
              Gd::out.printError("Error: Cannot copy file: " + _backupPath + _backupFilename + '0');
            }
          }
        } else Gd::out.printError("Error: Can't backup database: _backupPath or _backupFilename is empty.");
      }
    } else {
      Gd::out.printWarning("Warning: No database found. Trying to restore backup.");
      if (!_backupPath.empty() && !_backupFilename.empty()) {
        bool restored = false;
        for (int32_t i = 0; i <= 10000; i++) {
          if (BaseLib::Io::fileExists(_backupPath + _backupFilename + std::to_string(i)) && checkIntegrity(_backupPath + _backupFilename + std::to_string(i))) {
            Gd::out.printWarning("Warning: Restoring database file: " + _backupPath + _backupFilename + std::to_string(i));
            if (Gd::bl->io.copyFile(_backupPath + _backupFilename + std::to_string(i), _databasePath + _databaseFilename)) {
              restored = true;
              break;
            }
          }
        }
        if (restored) Gd::out.printWarning("Warning: Database successfully restored.");
        else Gd::out.printWarning("Warning: Database could not be restored. Creating new database.");
      }
    }
    openDatabase(false);
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

bool Sqlite3::checkIntegrity(const std::string& databasePath) {
  sqlite3 *database = nullptr;
  try {
    int32_t result = sqlite3_open(databasePath.c_str(), &database);
    if (result || !database) {
      if (database) sqlite3_close(database);
      return true;
    }

    std::shared_ptr<BaseLib::Database::DataTable> integrityResult(new BaseLib::Database::DataTable());

    sqlite3_stmt *statement = nullptr;
    result = sqlite3_prepare_v2(database, "PRAGMA integrity_check", -1, &statement, nullptr);
    if (result) {
      sqlite3_close(database);
      return false;
    }
    try {
      getDataRows(statement, integrityResult);
    }
    catch (const std::exception &ex) {
      sqlite3_close(database);
      return false;
    }
    sqlite3_clear_bindings(statement);
    result = sqlite3_finalize(statement);
    if (result) {
      sqlite3_close(database);
      return false;
    }

    if (integrityResult->size() != 1 || integrityResult->at(0).empty() || integrityResult->at(0).at(0)->textValue != "ok") {
      sqlite3_close(database);
      return false;
    }
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  if (database) sqlite3_close(database);
  return true;
}

void Sqlite3::openDatabase(bool lockMutex) {
  try {
    std::unique_lock<std::mutex> databaseGuard(_databaseMutex, std::defer_lock);
    if (lockMutex) databaseGuard.lock();
    char *errorMessage = nullptr;
    std::string fullDatabasePath = _databasePath + _databaseFilename;
    int result = sqlite3_open(fullDatabasePath.c_str(), &_database);
    if (result || !_database) {
      Gd::out.printCritical("Can't open database: " + std::string(sqlite3_errmsg(_database)));
      if (_database) sqlite3_close(_database);
      _database = nullptr;
      return;
    }
    sqlite3_extended_result_codes(_database, 1);

    if (!_databaseSynchronous) {
      sqlite3_exec(_database, "PRAGMA synchronous=OFF", nullptr, nullptr, &errorMessage);
      if (errorMessage) {
        Gd::out.printError("Can't execute \"PRAGMA synchronous=OFF\": " + std::string(errorMessage));
        sqlite3_free(errorMessage);
      }
    }

    //Reset to default journal mode, because WAL stays active.
    //Also I'm not sure if VACUUM works with journal_mode=WAL.
    sqlite3_exec(_database, "PRAGMA journal_mode=DELETE", nullptr, nullptr, &errorMessage);
    if (errorMessage) {
      Gd::out.printError("Can't execute \"PRAGMA journal_mode=DELETE\": " + std::string(errorMessage));
      sqlite3_free(errorMessage);
    }

    sqlite3_exec(_database, "VACUUM", nullptr, nullptr, &errorMessage);
    if (errorMessage) {
      Gd::out.printWarning("Warning: Can't execute \"VACUUM\": " + std::string(errorMessage));
      sqlite3_free(errorMessage);
    }

    if (_databaseMemoryJournal) {
      sqlite3_exec(_database, "PRAGMA journal_mode=MEMORY", nullptr, nullptr, &errorMessage);
      if (errorMessage) {
        Gd::out.printError("Can't execute \"PRAGMA journal_mode=MEMORY\": " + std::string(errorMessage));
        sqlite3_free(errorMessage);
      }
    }

    if (_databaseWALJournal) {
      sqlite3_exec(_database, "PRAGMA journal_mode=WAL", nullptr, nullptr, &errorMessage);
      if (errorMessage) {
        Gd::out.printError("Can't execute \"PRAGMA journal_mode=WAL\": " + std::string(errorMessage));
        sqlite3_free(errorMessage);
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

void Sqlite3::closeDatabase(bool lockMutex) {
  try {
    if (!_database) return;
    std::unique_lock<std::mutex> databaseGuard(_databaseMutex, std::defer_lock);
    if (lockMutex) databaseGuard.lock();
    Gd::out.printInfo("Closing database...");
    char *errorMessage = nullptr;
    sqlite3_exec(_database, "COMMIT", nullptr, nullptr, &errorMessage); //Release all savepoints
    if (errorMessage) {
      //Normally error is: No transaction is active, so no real error
      Gd::out.printDebug("Debug: Can't execute \"COMMIT\": " + std::string(errorMessage));
      sqlite3_free(errorMessage);
    }
    sqlite3_exec(_database, "PRAGMA synchronous = FULL", nullptr, nullptr, &errorMessage);
    if (errorMessage) {
      Gd::out.printError("Error: Can't execute \"PRAGMA synchronous = FULL\": " + std::string(errorMessage));
      sqlite3_free(errorMessage);
    }
    sqlite3_exec(_database, "PRAGMA journal_mode = DELETE", nullptr, nullptr, &errorMessage);
    if (errorMessage) {
      Gd::out.printError("Error: Can't execute \"PRAGMA journal_mode = DELETE\": " + std::string(errorMessage));
      sqlite3_free(errorMessage);
    }
    sqlite3_close(_database);
    _database = nullptr;
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void Sqlite3::getDataRows(sqlite3_stmt *statement, std::shared_ptr<BaseLib::Database::DataTable> &dataRows) {
  int32_t result;
  int32_t row = 0;
  while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
    for (int32_t i = 0; i < sqlite3_column_count(statement); i++) {
      std::shared_ptr<BaseLib::Database::DataColumn> col(new BaseLib::Database::DataColumn());
      col->index = i;
      int32_t columnType = sqlite3_column_type(statement, i);
      if (columnType == SQLITE_INTEGER) {
        col->dataType = BaseLib::Database::DataColumn::DataType::Enum::INTEGER;
        col->intValue = sqlite3_column_int64(statement, i);
      } else if (columnType == SQLITE_FLOAT) {
        col->dataType = BaseLib::Database::DataColumn::DataType::Enum::FLOAT;
        col->floatValue = sqlite3_column_double(statement, i);
      } else if (columnType == SQLITE_BLOB) {
        col->dataType = BaseLib::Database::DataColumn::DataType::Enum::BLOB;
        char *binaryData = (char *)sqlite3_column_blob(statement, i);
        int32_t size = sqlite3_column_bytes(statement, i);
        if (size > 0) col->binaryValue.reset(new std::vector<char>(binaryData, binaryData + size));
      } else if (columnType == SQLITE_NULL) {
        col->dataType = BaseLib::Database::DataColumn::DataType::Enum::NODATA;
      } else if (columnType == SQLITE_TEXT) //or SQLITE3_TEXT. As we are not using SQLite version 2 it doesn't matter
      {
        col->dataType = BaseLib::Database::DataColumn::DataType::Enum::TEXT;
        col->textValue = std::string((const char *)sqlite3_column_text(statement, i));
      }
      if (i == 0) dataRows->insert(std::pair<uint32_t, std::map<uint32_t, std::shared_ptr<BaseLib::Database::DataColumn>>>(row, std::map<uint32_t, std::shared_ptr<BaseLib::Database::DataColumn>>()));
      dataRows->at(row)[i] = col;
    }
    row++;
  }
  if (result != SQLITE_DONE) {
    throw BaseLib::Exception("Can't execute command (Error-no.: " + std::to_string(result) + "): " + std::string(sqlite3_errmsg(_database)));
  }
}

void Sqlite3::bindData(sqlite3_stmt *statement, BaseLib::Database::DataRow &dataToEscape) {
  //There is no try/catch block on purpose!
  int32_t result = 0;
  int32_t index = 1;
  std::for_each(dataToEscape.begin(), dataToEscape.end(), [&](const std::shared_ptr<BaseLib::Database::DataColumn>& col) {
    switch (col->dataType) {
      case BaseLib::Database::DataColumn::DataType::Enum::NODATA:result = sqlite3_bind_null(statement, index);
        break;
      case BaseLib::Database::DataColumn::DataType::Enum::INTEGER:result = sqlite3_bind_int64(statement, index, col->intValue);
        break;
      case BaseLib::Database::DataColumn::DataType::Enum::FLOAT:result = sqlite3_bind_double(statement, index, col->floatValue);
        break;
      case BaseLib::Database::DataColumn::DataType::Enum::BLOB:
        if (col->binaryValue->empty()) result = sqlite3_bind_null(statement, index);
        else result = sqlite3_bind_blob(statement, index, &col->binaryValue->at(0), col->binaryValue->size(), SQLITE_STATIC);
        break;
      case BaseLib::Database::DataColumn::DataType::Enum::TEXT:result = sqlite3_bind_text(statement, index, col->textValue.c_str(), -1, SQLITE_STATIC);
        break;
    }
    if (result) {
      throw (BaseLib::Exception(std::string(sqlite3_errmsg(_database))));
    }
    index++;
  });
}

uint32_t Sqlite3::executeWriteCommand(const std::shared_ptr<std::pair<std::string, BaseLib::Database::DataRow>>& command) {
  try {
    if (!command) return 0;
    std::lock_guard<std::mutex> databaseGuard(_databaseMutex);
    if (!_database) {
      Gd::out.printError("Error: Could not write to database. No database handle.");
      return 0;
    }
    sqlite3_stmt *statement = nullptr;
    int32_t result = sqlite3_prepare_v2(_database, command->first.c_str(), -1, &statement, nullptr);
    if (result) {
      Gd::out.printError("Can't execute command \"" + command->first + "\": " + std::string(sqlite3_errmsg(_database)));
      return 0;
    }
    if (!command->second.empty()) bindData(statement, command->second);
    result = sqlite3_step(statement);
    if (result != SQLITE_DONE) {
      Gd::out.printError("Can't execute command: " + std::string(sqlite3_errmsg(_database)));
      return 0;
    }
    sqlite3_clear_bindings(statement);
    result = sqlite3_finalize(statement);
    if (result) {
      Gd::out.printError("Can't execute command \"" + command->first + "\": " + std::string(sqlite3_errmsg(_database)));
      return 0;
    }
    uint32_t rowID = sqlite3_last_insert_rowid(_database);
    return rowID;
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return 0;
}

uint32_t Sqlite3::executeWriteCommand(const std::string& command, BaseLib::Database::DataRow &dataToEscape) {
  try {
    std::lock_guard<std::mutex> databaseGuard(_databaseMutex);
    if (!_database) {
      Gd::out.printError("Error: Could not write to database. No database handle.");
      return 0;
    }
    sqlite3_stmt *statement = nullptr;
    int32_t result = sqlite3_prepare_v2(_database, command.c_str(), -1, &statement, nullptr);
    if (result) {
      Gd::out.printError("Can't execute command \"" + command + "\": " + std::string(sqlite3_errmsg(_database)));
      return 0;
    }
    if (!dataToEscape.empty()) bindData(statement, dataToEscape);
    result = sqlite3_step(statement);
    if (result != SQLITE_DONE) {
      Gd::out.printError("Can't execute command \"" + command + "\": " + std::string(sqlite3_errmsg(_database)));
      return 0;
    }
    sqlite3_clear_bindings(statement);
    result = sqlite3_finalize(statement);
    if (result) {
      Gd::out.printError("Can't execute command \"" + command + "\": " + std::string(sqlite3_errmsg(_database)));
      return 0;
    }
    uint32_t rowID = sqlite3_last_insert_rowid(_database);
    return rowID;
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return 0;
}

std::shared_ptr<BaseLib::Database::DataTable> Sqlite3::executeCommand(const std::string& command, BaseLib::Database::DataRow &dataToEscape) {
  std::shared_ptr<BaseLib::Database::DataTable> dataRows(new BaseLib::Database::DataTable());
  try {
    std::lock_guard<std::mutex> databaseGuard(_databaseMutex);
    if (!_database) {
      Gd::out.printError("Error: Could not write to database. No database handle.");
      return dataRows;
    }
    sqlite3_stmt *statement = nullptr;
    int32_t result = sqlite3_prepare_v2(_database, command.c_str(), -1, &statement, nullptr);
    if (result) {
      Gd::out.printError("Can't execute command \"" + command + "\": " + std::string(sqlite3_errmsg(_database)));
      return dataRows;
    }
    bindData(statement, dataToEscape);
    try {
      getDataRows(statement, dataRows);
    }
    catch (const std::exception &ex) {
      if (command.compare(0, 7, "RELEASE") == 0) {
        Gd::out.printInfo(std::string("Info: ") + ex.what());
        sqlite3_clear_bindings(statement);
        return dataRows;
      } else Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    sqlite3_clear_bindings(statement);
    result = sqlite3_finalize(statement);
    if (result) Gd::out.printError("Can't execute command \"" + command + "\" (Error-no.: " + std::to_string(result) + "): " + std::string(sqlite3_errmsg(_database)));
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return dataRows;
}

std::shared_ptr<BaseLib::Database::DataTable> Sqlite3::executeCommand(const std::string& command) {
  std::shared_ptr<BaseLib::Database::DataTable> dataRows(new BaseLib::Database::DataTable());
  try {
    std::lock_guard<std::mutex> databaseGuard(_databaseMutex);
    if (!_database) {
      Gd::out.printError("Error: Could not write to database. No database handle.");
      return dataRows;
    }
    sqlite3_stmt *statement = nullptr;
    int32_t result = sqlite3_prepare_v2(_database, command.c_str(), -1, &statement, nullptr);
    if (result) {
      Gd::out.printError("Can't execute command \"" + command + "\": " + std::string(sqlite3_errmsg(_database)));
      return dataRows;
    }
    try {
      getDataRows(statement, dataRows);
    }
    catch (const std::exception &ex) {
      if (command.compare(0, 7, "RELEASE") == 0) {
        Gd::out.printInfo(std::string("Info: ") + ex.what());
        sqlite3_clear_bindings(statement);
        return dataRows;
      } else Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    sqlite3_clear_bindings(statement);
    result = sqlite3_finalize(statement);
    if (result) {
      Gd::out.printError("Can't execute command \"" + command + "\" (Error-no.: " + std::to_string(result) + "): " + std::string(sqlite3_errmsg(_database)));
    }
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return dataRows;
}

}
