cmake_minimum_required(VERSION 3.8)
project(mellon-api)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES
        src/MellonCommands.h
        src/Gd.cpp
        src/Gd.h
        src/main.cpp
        src/Mellon.cpp
        src/Mellon.h
        src/RestClientInfo.h
        src/RestInfo.h
        src/RestServer.cpp
        src/RestServer.h
        src/Serial.cpp
        src/Serial.h
        src/Settings.cpp
        src/Settings.h
        src/Sqlite3.cpp
        src/Sqlite3.h
        src/REST/Ping.cpp
        src/REST/Ping.h src/Database.cpp src/Database.h src/REST/Ca.cpp src/REST/Ca.h src/REST/Ca/CaKeySlot.cpp src/REST/Ca/CaKeySlot.h src/REST/Ca/CaKeySlot/Sign.cpp src/REST/Ca/CaKeySlot/Sign.h src/Acl.cpp src/Acl.h src/HelperFunctions.cpp src/HelperFunctions.h src/REST/Mellon.cpp src/REST/Mellon.h src/REST/Aes.cpp src/REST/Aes.h src/REST/Aes/AesKeySlot.cpp src/REST/Aes/AesKeySlot.h src/REST/Aes/AesKeySlot/Encrypt.cpp src/REST/Aes/AesKeySlot/Encrypt.h src/REST/Aes/AesKeySlot/Decrypt.cpp src/REST/Aes/AesKeySlot/Decrypt.h src/REST/Crl.cpp src/REST/Crl.h src/REST/Ca/Token.cpp src/REST/Ca/Token.h src/REST/Ca/Token/Create.cpp src/REST/Ca/Token/Create.h)

add_executable(mellon-api ${SOURCE_FILES})

target_link_libraries(mellon-api homegear-base c1-net gnutls gcrypt pthread sqlite3 z)
