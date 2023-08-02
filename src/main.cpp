/* Copyright 2013-2019 Homegear GmbH */

#include "Gd.h"

#include <iostream>
#include <memory>

#include <malloc.h>
#include <sys/prctl.h> //For function prctl
#ifdef BSDSYSTEM
#include <sys/sysctl.h> //For BSD systems
#endif
#include <sys/resource.h> //getrlimit, setrlimit
#include <sys/file.h> //flock
#include <sys/types.h>
#include <sys/stat.h>

#include <gnutls/x509.h>
#include <gcrypt.h>
#include <grp.h>

#define VERSION "1.0.0"

using namespace MellonApi;

void startUp();

GCRY_THREAD_OPTION_PTHREAD_IMPL;

bool _startAsDaemon = false;
std::thread _signalHandlerThread;
bool _stopProgram = false;
int _signalNumber = -1;
std::mutex _stopProgramMutex;
std::condition_variable _stopProgramConditionVariable;

struct MellonApiArguments {
  bool importCertificate = false;
  std::string certificatePath;
  std::string acl;
};

void terminateProgram(int signalNumber) {
  try {
    Gd::bl->shuttingDown = true;
    Gd::out
        .printMessage("(Shutdown) => Stopping Mellon REST server (Signal: " + std::to_string(signalNumber) + ")");
    Gd::out.printMessage("(Shutdown) => Stopping REST server...");

    if (Gd::restServer) Gd::restServer->stop();
    Gd::restServer.reset();

    if (Gd::db) {
      Gd::out.printMessage("(Shutdown) => Disposing database");
      Gd::db->dispose();
    }

    Gd::out.printMessage("(Shutdown) => Shutdown complete.");
    fclose(stdout);
    fclose(stderr);
    gnutls_global_deinit();
    gcry_control(GCRYCTL_SUSPEND_SECMEM_WARN);
    gcry_control(GCRYCTL_TERM_SECMEM);
    gcry_control(GCRYCTL_RESUME_SECMEM_WARN);

    return;
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  _exit(1);
}

void signalHandlerThread() {
  sigset_t set{};
  int signalNumber = -1;
  sigemptyset(&set);
  sigaddset(&set, SIGHUP);
  sigaddset(&set, SIGTERM);
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGABRT);
  sigaddset(&set, SIGSEGV);
  sigaddset(&set, SIGQUIT);
  sigaddset(&set, SIGILL);
  sigaddset(&set, SIGFPE);
  sigaddset(&set, SIGALRM);
  sigaddset(&set, SIGUSR1);
  sigaddset(&set, SIGUSR2);
  sigaddset(&set, SIGTSTP);
  sigaddset(&set, SIGTTIN);
  sigaddset(&set, SIGTTOU);

  while (!_stopProgram) {
    try {
      if (sigwait(&set, &signalNumber) != 0) {
        Gd::out.printError("Error calling sigwait. Killing myself.");
        kill(getpid(), SIGKILL);
      }
      if (signalNumber == SIGTERM || signalNumber == SIGINT) {
        std::unique_lock<std::mutex> stopHomegearGuard(_stopProgramMutex);
        _stopProgram = true;
        _signalNumber = signalNumber;
        stopHomegearGuard.unlock();
        _stopProgramConditionVariable.notify_all();
        return;
      } else if (signalNumber == SIGHUP) {
        Gd::out.printMessage("Info: SIGHUP received. Reloading...");

        if (!std::freopen((Gd::settings.logfilePath() + "mellon-api.log").c_str(), "a", stdout)) {
          Gd::out.printError("Error: Could not redirect output to new log file.");
        }
        if (!std::freopen((Gd::settings.logfilePath() + "mellon-api.err").c_str(), "a", stderr)) {
          Gd::out.printError("Error: Could not redirect errors to new log file.");
        }

        Gd::mellon->setTime();

        if (Gd::bl) {
          Gd::out.printMessage("Info: Backing up database...");
          Gd::db->hotBackup();
          if (!Gd::db->isOpen()) {
            Gd::out.printCritical("Critical: Can't reopen database. Exiting...");
            _exit(1);
          }
        }

        if(Gd::restServer) {
          Gd::out.printMessage("Info: Reloading REST server...");
          Gd::restServer->reload();
        }

        Gd::out.printInfo("Info: Reload complete.");
      } else {
        Gd::out.printCritical("Signal " + std::to_string(signalNumber) + " received.");
        pthread_sigmask(SIG_SETMASK, &BaseLib::SharedObjects::defaultSignalMask, nullptr);
        kill(getpid(), signalNumber); //Raise same signal again using the default action.
      }
    }
    catch (const std::exception &ex) {
      Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
  }
}

void getExecutablePath(int argc, char *argv[]) {
  char path[1024];
  if (!getcwd(path, sizeof(path))) {
    std::cerr << "Could not get working directory." << std::endl;
    exit(1);
  }
  Gd::workingDirectory = std::string(path);
#ifdef KERN_PROC //BSD system
  int mib[4];
  mib[0] = CTL_KERN;
  mib[1] = KERN_PROC;
  mib[2] = KERN_PROC_PATHNAME;
  mib[3] = -1;
  size_t cb = sizeof(path);
  int result = sysctl(mib, 4, path, &cb, NULL, 0);
  if(result == -1)
  {
      std::cerr << "Could not get executable path." << std::endl;
      exit(1);
  }
  path[sizeof(path) - 1] = '\0';
  GD::executablePath = std::string(path);
  GD::executablePath = GD::executablePath.substr(0, GD::executablePath.find_last_of("/") + 1);
#else
  int length = readlink("/proc/self/exe", path, sizeof(path) - 1);
  if (length < 0) {
    std::cerr << "Could not get executable path." << std::endl;
    exit(1);
  }
  if ((unsigned)length > sizeof(path)) {
    std::cerr << "The path to the binary is in has more than 1024 characters." << std::endl;
    exit(1);
  }
  path[length] = '\0';
  Gd::executablePath = std::string(path);
  Gd::executablePath = Gd::executablePath.substr(0, Gd::executablePath.find_last_of("/") + 1);
#endif

  Gd::executableFile = std::string(argc > 0 ? argv[0] : "mellon-api");
  BaseLib::HelperFunctions::trim(Gd::executableFile);
  if (Gd::executableFile.empty()) Gd::executableFile = "mellon-api";
  std::pair<std::string, std::string> pathNamePair = BaseLib::HelperFunctions::splitLast(Gd::executableFile, '/');
  if (!pathNamePair.second.empty()) Gd::executableFile = pathNamePair.second;
}

void initGnuTls() {
  // {{{ Init gcrypt and GnuTLS
  gcry_error_t gcryResult;
  if ((gcryResult = gcry_control(GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread)) != GPG_ERR_NO_ERROR) {
    Gd::out.printCritical("Critical: Could not enable thread support for gcrypt.");
    exit(2);
  }

  if (!gcry_check_version(GCRYPT_VERSION)) {
    Gd::out.printCritical("Critical: Wrong gcrypt version.");
    exit(2);
  }
  gcry_control(GCRYCTL_SUSPEND_SECMEM_WARN);
  if ((gcryResult = gcry_control(GCRYCTL_INIT_SECMEM, (int)Gd::settings.secureMemorySize(), 0)) != GPG_ERR_NO_ERROR) {
    Gd::out.printCritical(
        "Critical: Could not allocate secure memory. Error code is: " + std::to_string((int32_t)gcryResult));
    exit(2);
  }
  gcry_control(GCRYCTL_RESUME_SECMEM_WARN);
  gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);

  int32_t gnutlsResult = 0;
  if ((gnutlsResult = gnutls_global_init()) != GNUTLS_E_SUCCESS) {
    Gd::out.printCritical("Critical: Could not initialize GnuTLS: " + std::string(gnutls_strerror(gnutlsResult)));
    exit(2);
  }
  // }}}
}

void setLimits() {
  struct rlimit limits{};
  if (!Gd::settings.enableCoreDumps()) prctl(PR_SET_DUMPABLE, 0);
  else {
    //Set rlimit for core dumps
    getrlimit(RLIMIT_CORE, &limits);
    limits.rlim_cur = limits.rlim_max;
    setrlimit(RLIMIT_CORE, &limits);
    getrlimit(RLIMIT_CORE, &limits);
  }
}

void printHelp() {
  std::cout << "Usage: mellon-api [OPTIONS]" << std::endl << std::endl;
  std::cout << "Option                     Meaning" << std::endl;
  std::cout << "-h                         Show this help" << std::endl;
  std::cout << "-u                         Run as user" << std::endl;
  std::cout << "-g                         Run as group" << std::endl;
  std::cout << "-c <path>                  Specify path to config file" << std::endl;
  std::cout << "-d                         Run as daemon" << std::endl;
  std::cout << "-p <pid path>              Specify path to process id file" << std::endl;
  std::cout << "-i <certificate path>      Import PEM encoded certificate" << std::endl;
  std::cout << "-a <acl>                   Only in combination with \"-i\". Sets the certificate's ACL to <acl>. Expects a JSON object." << std::endl;
  std::cout << "-v                         Print program version" << std::endl;
}

void startDaemon() {
  try {
    pid_t pid, sid;
    pid = fork();
    if (pid < 0) {
      exit(1);
    }
    if (pid > 0) {
      exit(0);
    }

    //Set process permission
    umask(S_IWGRP | S_IWOTH);

    //Set child processe's id
    sid = setsid();
    if (sid < 0) {
      exit(1);
    }

    close(STDIN_FILENO);
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void startUp() {
  try {
    if ((chdir(Gd::settings.workingDirectory().c_str())) < 0) {
      Gd::out.printError("Could not change working directory to " + Gd::settings.workingDirectory() + ".");
      exit(1);
    }

    if (Gd::settings.memoryDebugging())
      mallopt(M_CHECK_ACTION,
              3); //Print detailed error message, stack trace, and memory, and abort the program. See: http://man7.org/linux/man-pages/man3/mallopt.3.html

    initGnuTls();

    setLimits();

    if (!std::freopen((Gd::settings.logfilePath() + "mellon-api.log").c_str(), "a", stdout)) {
      Gd::out.printError("Error: Could not redirect output to log file.");
    }
    if (!std::freopen((Gd::settings.logfilePath() + "mellon-api.err").c_str(), "a", stderr)) {
      Gd::out.printError("Error: Could not redirect errors to log file.");
    }

    Gd::out.printMessage("Starting Mellon REST server...");

    if (Gd::runAsUser.empty()) Gd::runAsUser = Gd::settings.runAsUser();
    if (Gd::runAsGroup.empty()) Gd::runAsGroup = Gd::settings.runAsGroup();
    if ((!Gd::runAsUser.empty() && Gd::runAsGroup.empty()) || (!Gd::runAsGroup.empty() && Gd::runAsUser.empty())) {
      Gd::out
          .printCritical("Critical: You only provided a user OR a group for Homegear to run as. Please specify both.");
      exit(1);
    }
    uid_t userId = BaseLib::HelperFunctions::userId(Gd::runAsUser);
    gid_t groupId = BaseLib::HelperFunctions::groupId(Gd::runAsGroup);
    std::string currentPath;
    if (!Gd::pidfilePath.empty() && Gd::pidfilePath.find('/') != std::string::npos) {
      currentPath = Gd::pidfilePath.substr(0, Gd::pidfilePath.find_last_of('/'));
      if (!currentPath.empty()) {
        if (!BaseLib::Io::directoryExists(currentPath)) BaseLib::Io::createDirectory(currentPath, S_IRWXU | S_IRWXG);
        if (chown(currentPath.c_str(), userId, groupId) == -1)
          std::cerr << "Could not set owner on " << currentPath << std::endl;
        if (chmod(currentPath.c_str(), S_IRWXU | S_IRWXG) == -1)
          std::cerr << "Could not set permissions on " << currentPath << std::endl;
      }
    }

    currentPath = Gd::settings.databasePath();
    {
      std::vector<std::string> files;
      try {
        files = Gd::bl->io.getFiles(currentPath, false);
      }
      catch (const std::exception &ex) {
        Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
      }
      for (auto &file : files) {
        auto filePath = currentPath + file;
        if (chown(filePath.c_str(), userId, groupId) == -1) Gd::out.printError("Could not set owner on " + filePath);
        if (chmod(filePath.c_str(), S_IRWXU | S_IRWXG) == -1) Gd::out.printError("Could not set permissions on " + filePath);
      }
    }

    currentPath = Gd::settings.databaseBackupPath();
    if (!currentPath.empty()) {
      std::vector<std::string> files;
      try {
        files = Gd::bl->io.getFiles(currentPath, false);
      }
      catch (const std::exception &ex) {
        Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
      }
      for (auto &file : files) {
        auto filePath = currentPath + file;
        if (chown(filePath.c_str(), userId, groupId) == -1) Gd::out.printError("Could not set owner on " + filePath);
        if (chmod(filePath.c_str(), S_IRWXU | S_IRWXG) == -1) Gd::out.printError("Could not set permissions on " + filePath);
      }
    }

    Gd::db = std::make_unique<Database>();
    Gd::db->init();
    if (Gd::settings.databasePath().empty()) {
      Gd::out.printCritical("Could initialize database. No database path specified.");
      exit(1);
    }
    auto databaseBackupPath = Gd::settings.databaseBackupPath();
    if (databaseBackupPath.empty()) databaseBackupPath = Gd::settings.databasePath();
    Gd::db->open(Gd::settings.databasePath(), "db.sqlite", Gd::settings.databaseSynchronous(), Gd::settings.databaseMemoryJournal(), Gd::settings.databaseWALJournal(), databaseBackupPath, "db.sqlite.bak");
    if (!Gd::db->isOpen()) exit(1);

    Gd::out.printInfo("Initializing database...");
    if (Gd::db->convertDatabase()) exit(0);
    Gd::db->initializeDatabase();

    Gd::out.printMessage("Binding REST server...");
    Gd::restServer = std::make_unique<RestServer>();
    Gd::restServer->bind();

    if (getuid() == 0 && !Gd::runAsUser.empty() && !Gd::runAsGroup.empty()) {
      if (Gd::bl->userId == 0 || Gd::bl->groupId == 0) {
        Gd::out.printCritical("Could not drop privileges. User name or group name is not valid.");
        exit(1);
      }
      Gd::out.printInfo(
          "Info: Dropping privileges to user " + Gd::runAsUser + " (" + std::to_string(Gd::bl->userId) + ") and group "
              + Gd::runAsGroup + " (" + std::to_string(Gd::bl->groupId) + ")");

      int result = -1;
      std::vector<gid_t> supplementaryGroups(10);
      int numberOfGroups = 10;
      while (result == -1) {
        result = getgrouplist(Gd::runAsUser.c_str(), 10000, supplementaryGroups.data(), &numberOfGroups);

        if (result == -1) supplementaryGroups.resize(numberOfGroups);
        else supplementaryGroups.resize(result);
      }

      if (setgid(Gd::bl->groupId) != 0) {
        Gd::out.printCritical("Critical: Could not drop group privileges.");
        exit(1);
      }

      if (setgroups(supplementaryGroups.size(), supplementaryGroups.data()) != 0) {
        Gd::out.printCritical("Critical: Could not set supplementary groups: " + std::string(strerror(errno)));
        exit(1);
      }

      if (setuid(Gd::bl->userId) != 0) {
        Gd::out.printCritical("Critical: Could not drop user privileges.");
        exit(1);
      }

      //Core dumps are disabled by setuid. Enable them again.
      if (Gd::settings.enableCoreDumps()) prctl(PR_SET_DUMPABLE, 1);
    }

    if (getuid() == 0) {
      if (!Gd::runAsUser.empty() && !Gd::runAsGroup.empty()) {
        Gd::out.printCritical(
            "Critical: The program still has root privileges though privileges should have been dropped. Exiting as this is a security risk.");
        exit(1);
      }
    } else {
      if (setuid(0) != -1) {
        Gd::out
            .printCritical("Critical: Regaining root privileges succeded. Exiting as this is a security risk.");
        exit(1);
      }
      Gd::out.printInfo("Info: Mellon REST server is (now) running as user with id " + std::to_string(getuid())
                            + " and group with id " + std::to_string(getgid()) + '.');
    }

    //Create PID file
    try {
      if (!Gd::pidfilePath.empty()) {
        int32_t pidfile = open(Gd::pidfilePath.c_str(), O_CREAT | O_RDWR, 0666);
        if (pidfile < 0) {
          Gd::out.printError("Error: Cannot create pid file \"" + Gd::pidfilePath + "\".");
        } else {
          int32_t rc = flock(pidfile, LOCK_EX | LOCK_NB);
          if (rc && errno == EWOULDBLOCK) {
            Gd::out.printError("Error: Mellon REST server is already running - Can't lock PID file.");
          }
          std::string pid(std::to_string(getpid()));
          int32_t bytesWritten = write(pidfile, pid.c_str(), pid.size());
          if (bytesWritten <= 0) Gd::out.printError("Error writing to PID file: " + std::string(strerror(errno)));
          close(pidfile);
        }
      }
    }
    catch (const std::exception &ex) {
      Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch (...) {
      Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }

    while (BaseLib::HelperFunctions::getTime() < 1000000000000) {
      Gd::out.printWarning("Warning: Time is in the past. Waiting for ntp to set the time...");
      std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    }

    Gd::out.printMessage("Initializing Mellon...");
    Gd::mellon = std::make_unique<Mellon>(Gd::settings.mellonDevice(), true);

    Gd::out.printMessage("Starting REST server...");
    Gd::restServer->start();

    Gd::bl->threadManager.start(_signalHandlerThread, true, &signalHandlerThread);

    Gd::out.printMessage("Startup complete.");

    Gd::bl->booting = false;

    if (BaseLib::Io::fileExists(Gd::settings.workingDirectory() + "core")) {
      Gd::out.printError("Error: A core file exists in Mellon REST server's working directory (\""
                             + Gd::settings.workingDirectory() + "core"
                             + "\"). Please send this file to the Homegear team including information about your system (Linux distribution, CPU architecture), the Mellon REST server version, the current log files and information what might've caused the error.");
    }

    while (!_stopProgram) {
      std::unique_lock<std::mutex> stopHomegearGuard(_stopProgramMutex);
      _stopProgramConditionVariable.wait(stopHomegearGuard);
    }

    terminateProgram(_signalNumber);
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

int main(int argc, char *argv[]) {
  try {
    MellonApiArguments arguments;
    getExecutablePath(argc, argv);
    Gd::bl = std::make_unique<BaseLib::SharedObjects>();
    Gd::bl->out.init(Gd::bl.get());
    Gd::out.init(Gd::bl.get());

    if (BaseLib::Io::directoryExists(Gd::executablePath + "config")) Gd::configPath = Gd::executablePath + "config/";
    else if (BaseLib::Io::directoryExists(Gd::executablePath + "cfg")) Gd::configPath = Gd::executablePath + "cfg/";
    else Gd::configPath = "/etc/mellon-api/";

    for (int32_t i = 1; i < argc; i++) {
      std::string arg(argv[i]);
      if (arg == "-h" || arg == "--help") {
        printHelp();
        exit(0);
      } else if (arg == "-c") {
        if (i + 1 < argc) {
          std::string configPath = std::string(argv[i + 1]);
          if (!configPath.empty()) Gd::configPath = configPath;
          if (Gd::configPath[Gd::configPath.size() - 1] != '/') Gd::configPath.push_back('/');
          i++;
        } else {
          printHelp();
          exit(1);
        }
      } else if (arg == "-p") {
        if (i + 1 < argc) {
          Gd::pidfilePath = std::string(argv[i + 1]);
          i++;
        } else {
          printHelp();
          exit(1);
        }
      } else if (arg == "-u") {
        if (i + 1 < argc) {
          Gd::runAsUser = std::string(argv[i + 1]);
          i++;
        } else {
          printHelp();
          exit(1);
        }
      } else if (arg == "-g") {
        if (i + 1 < argc) {
          Gd::runAsGroup = std::string(argv[i + 1]);
          i++;
        } else {
          printHelp();
          exit(1);
        }
      } else if (arg == "-d") {
        _startAsDaemon = true;
      } else if (arg == "-i") {
        if (i + 1 < argc) {
          arguments.importCertificate = true;
          arguments.certificatePath = std::string(argv[i + 1]);
          i++;
        } else {
          printHelp();
          exit(1);
        }
      } else if (arg == "-a") {
        if (i + 1 < argc) {
          arguments.acl = std::string(argv[i + 1]);
          i++;
        } else {
          printHelp();
          exit(1);
        }
      } else if (arg == "-v") {
        std::cout << "Mellon REST server version " << VERSION << std::endl;
        std::cout << "Copyright (c) 2013-2020 Homegear GmbH" << std::endl << std::endl;
        exit(0);
      } else {
        printHelp();
        exit(1);
      }
    }

    //Import certificate
    if (arguments.importCertificate) {
      if (!BaseLib::Io::fileExists(arguments.certificatePath)) {
        std::cerr << "The specified certificate file does not exist." << std::endl;
        exit(1);
      }

      Gd::settings.load(Gd::configPath + "main.conf", Gd::executablePath);

      Gd::db = std::make_unique<Database>();
      Gd::db->init();
      if (Gd::settings.databasePath().empty()) {
        Gd::out.printCritical("Could initialize database. No database path specified.");
        exit(1);
      }
      auto databaseBackupPath = Gd::settings.databaseBackupPath();
      if (databaseBackupPath.empty()) databaseBackupPath = Gd::settings.databasePath();
      Gd::db->open(Gd::settings.databasePath(), "db.sqlite", Gd::settings.databaseSynchronous(), Gd::settings.databaseMemoryJournal(), Gd::settings.databaseWALJournal(), databaseBackupPath, "db.sqlite.bak");
      if (!Gd::db->isOpen()) exit(1);

      if (Gd::db->convertDatabase()) exit(0);
      Gd::db->initializeDatabase();

      int64_t expirationTime = 0;
      std::string serial;
      bool error = false;

      auto signedCertificate = BaseLib::Io::getFileContent(arguments.certificatePath);

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
          if (gnutls_x509_crt_get_serial(certificate, buffer.data(), &bufferSize) == GNUTLS_E_SUCCESS) {
            if (bufferSize > buffer.size()) bufferSize = buffer.size();
            serial = BaseLib::HelperFunctions::getHexString(buffer.data(), bufferSize);
          }
        } else {
          std::cerr << "Could not import signed certificate." << std::endl;
          error = true;
        }

        gnutls_x509_crt_deinit(certificate);
      }

      if (error) exit(1);
      if (!Gd::db->insertCertificate(serial, "", signedCertificate, expirationTime, arguments.acl)) {
        std::cerr << "Could not import signed certificate." << std::endl;
        exit(1);
      }

      std::cout << "Certificate with serial number " + serial + " and expiration time " + std::to_string(expirationTime) + " was imported successfully." << std::endl;

      Gd::db->dispose();
      Gd::db.reset();
      exit(0);
    }

    {
      //Block the signals below during start up
      //Needs to be called after initialization of GD::bl as GD::bl reads the current (default) signal mask.
      sigset_t set{};
      sigemptyset(&set);
      sigaddset(&set, SIGHUP);
      sigaddset(&set, SIGTERM);
      sigaddset(&set, SIGINT);
      sigaddset(&set, SIGABRT);
      sigaddset(&set, SIGSEGV);
      sigaddset(&set, SIGQUIT);
      sigaddset(&set, SIGILL);
      sigaddset(&set, SIGFPE);
      sigaddset(&set, SIGALRM);
      sigaddset(&set, SIGUSR1);
      sigaddset(&set, SIGUSR2);
      sigaddset(&set, SIGTSTP);
      sigaddset(&set, SIGTTIN);
      sigaddset(&set, SIGTTOU);
      if (pthread_sigmask(SIG_BLOCK, &set, nullptr) < 0) {
        std::cerr << "SIG_BLOCK error." << std::endl;
        exit(1);
      }
    }

    // {{{ Load settings
    Gd::out.printInfo("Loading settings from " + Gd::configPath + "main.conf");
    Gd::settings.load(Gd::configPath + "main.conf", Gd::executablePath);
    if (Gd::runAsUser.empty()) Gd::runAsUser = Gd::settings.runAsUser();
    if (Gd::runAsGroup.empty()) Gd::runAsGroup = Gd::settings.runAsGroup();
    if ((!Gd::runAsUser.empty() && Gd::runAsGroup.empty()) || (!Gd::runAsGroup.empty() && Gd::runAsUser.empty())) {
      Gd::out.printCritical(
          "Critical: You only provided a user OR a group for Mellon REST server to run as. Please specify both.");
      exit(1);
    }
    Gd::bl->userId = Gd::bl->hf.userId(Gd::runAsUser);
    Gd::bl->groupId = Gd::bl->hf.groupId(Gd::runAsGroup);
    if ((int32_t)Gd::bl->userId == -1 || (int32_t)Gd::bl->groupId == -1) {
      Gd::bl->userId = 0;
      Gd::bl->groupId = 0;
    }
    // }}}

    if ((chdir(Gd::settings.workingDirectory().c_str())) < 0) {
      Gd::out.printError("Could not change working directory to " + Gd::settings.workingDirectory() + ".");
      exit(1);
    }

    if (_startAsDaemon) startDaemon();
    startUp();

    Gd::bl->threadManager.join(_signalHandlerThread);
    return 0;
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  _exit(1);
}
