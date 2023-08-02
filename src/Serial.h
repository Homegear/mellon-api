//
// Created by sathya on 24.03.19.
//

#ifndef MELLON_SERIAL_H
#define MELLON_SERIAL_H

#include <string>
#include <vector>
#include <cstring>
#include <iostream>

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <atomic>

namespace MellonApi {

class Serial
{
public:
    enum class CharacterSize : tcflag_t
    {
        Five = CS5,
        Six = CS6,
        Seven = CS7,
        Eight = CS8
    };

    /**
     * Constructor.
     *
     * @param device The device to use (e. g. "/dev/ttyUSB0")
     * @param baudrate The baudrate (e. g. 115200)
     * @param flags Flags passed to the C function "open". 0 should be fine for most cases. "O_NDELAY" is always added by the constructor. By default "O_RDWR | O_NOCTTY | O_NDELAY" is used.
     */
    Serial(std::string device, int32_t baudrate, int32_t flags);

    /**
     * Destructor.
     */
    virtual ~Serial();

    bool isOpen() { return _fileDescriptor != -1; }
    int fileDescriptor() { return _fileDescriptor; }

    /**
     * Opens the serial device.
     *
     * @param evenParity Enable parity checking using an even parity bit.
     * @param oddParity Enable parity checking using an odd parity bit. "evenParity" and "oddParity" are mutually exclusive.
     * @param characterSize Set the character Size.
     * @param twoStopBits Enable two stop bits instead of one.
     */
    void openDevice(bool evenParity, bool oddParity, CharacterSize characterSize = CharacterSize::Eight, bool twoStopBits = false);

    /**
     * Closes the serial device.
     */
    void closeDevice();

    /**
     * SerialReaderWriter can either be used through events (by implementing ISerialReaderWriterEventSink and usage of addEventHandler) or by polling using this method.
     * @param data The variable to write the returned character into.
     * @param timeout The maximum amount of time to wait in microseconds before the function returns (default: 500000).
     * @return Returns "0" on success, "1" on timeout or "-1" on error.
     */
    int32_t readChar(char& data, uint32_t timeout = 500000);

    /**
     * Writes binary data to the serial device.
     * @param data The data to write. It is written as is without any modification.
     */
    void writeData(const std::vector<uint8_t>& data);

    /**
     * Writes one character to the serial device.
     * @param data The (binary) character to write.
     */
    void writeChar(char data);
protected:
    std::atomic_bool _stop{false};

    int _fileDescriptor = -1;
    std::string _device;
    struct termios _termios{};
    int32_t _baudrate = 0;
    int32_t _flags = 0;
};

}

#endif //IBS_CRYPT_CLIENT_SERIAL_H
