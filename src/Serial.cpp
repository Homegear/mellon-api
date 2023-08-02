//
// Created by sathya on 24.03.19.
//

#include <fstream>
#include "Serial.h"
#include "Gd.h"

namespace MellonApi {

Serial::Serial(std::string device, int32_t baudrate, int32_t flags) {
  _device = device;
  _baudrate = baudrate;
  _flags = flags;
  if (_flags == 0) _flags = O_RDWR | O_NOCTTY | O_NDELAY;
  else _flags |= O_NDELAY;
}

Serial::~Serial() {
  closeDevice();
}

void Serial::openDevice(bool evenParity, bool oddParity, CharacterSize characterSize, bool twoStopBits) {
  if (_fileDescriptor != -1) return;

  _fileDescriptor = open(_device.c_str(), _flags);
  if (_fileDescriptor == -1) {
    Gd::out.printError("Couldn't open device \"" + _device + "\": " + strerror(errno));
    return;
  }

  struct flock lock{};
  lock.l_type = F_WRLCK;
  lock.l_start = 0;
  lock.l_whence = SEEK_SET;
  lock.l_len = 0;
  //Blocks until we get lock or a signal is caught.
  if (fcntl(_fileDescriptor, F_SETLKW, &lock) == -1) {
    Gd::out.printError("Device is already in use.");
    return;
  }

  tcflag_t baudrate;
  switch (_baudrate) {
    case 50:baudrate = B50;
      break;
    case 75:baudrate = B75;
      break;
    case 110:baudrate = B110;
      break;
    case 134:baudrate = B134;
      break;
    case 150:baudrate = B150;
      break;
    case 200:baudrate = B200;
      break;
    case 300:baudrate = B300;
      break;
    case 600:baudrate = B600;
      break;
    case 1200:baudrate = B1200;
      break;
    case 1800:baudrate = B1800;
      break;
    case 2400:baudrate = B2400;
      break;
    case 4800:baudrate = B4800;
      break;
    case 9600:baudrate = B9600;
      break;
    case 19200:baudrate = B19200;
      break;
    case 38400:baudrate = B38400;
      break;
    case 57600:baudrate = B57600;
      break;
    case 115200:baudrate = B115200;
      break;
    case 230400:baudrate = B230400;
      break;
    case 460800:baudrate = B460800;
      break;
    case 500000:baudrate = B500000;
      break;
    case 576000:baudrate = B576000;
      break;
    case 921600:baudrate = B921600;
      break;
    case 1000000:baudrate = B1000000;
      break;
    case 1152000:baudrate = B1152000;
      break;
    case 1500000:baudrate = B1500000;
      break;
    case 2000000:baudrate = B2000000;
      break;
    case 2500000:baudrate = B2500000;
      break;
    case 3000000:baudrate = B3000000;
      break;
    default:Gd::out.printError("Couldn't setup device \"" + _device + "\": Unsupported baudrate.");
      return;
  }
  memset(&_termios, 0, sizeof(termios));
  _termios.c_cflag = baudrate | (tcflag_t)characterSize | CREAD;
  if (evenParity) _termios.c_cflag |= PARENB;
  if (oddParity) _termios.c_cflag |= PARENB | PARODD;
  if (twoStopBits) _termios.c_cflag |= CSTOPB;
  _termios.c_iflag = 0;
  _termios.c_oflag = 0;
  _termios.c_lflag = 0;
  _termios.c_cc[VMIN] = 1;
  _termios.c_cc[VTIME] = 0;
  cfsetispeed(&_termios, baudrate);
  cfsetospeed(&_termios, baudrate);
  if (tcflush(_fileDescriptor, TCIFLUSH) == -1) {
    Gd::out.printError("Couldn't flush device " + _device);
    return;
  }
  if (tcsetattr(_fileDescriptor, TCSANOW, &_termios) == -1) {
    Gd::out.printError("Couldn't set device settings for device " + _device);
    return;
  }

  int flags = fcntl(_fileDescriptor, F_GETFL);
  if (!(flags & O_NONBLOCK)) {
    if (fcntl(_fileDescriptor, F_SETFL, flags | O_NONBLOCK) == -1) {
      Gd::out.printError("Couldn't set device to non blocking mode: " + _device);
      return;
    }
  }
}

void Serial::closeDevice() {
  _stop = true;
  if (_fileDescriptor != -1) {
    close(_fileDescriptor);
    auto lockfile = "/tmp/LCK.." + _device.substr(_device.find_last_of('/') + 1);
    unlink(lockfile.c_str());
  }
  _fileDescriptor = -1;
}

int32_t Serial::readChar(char &data, uint32_t timeout) {
  ssize_t i;
  fd_set readFileDescriptor;
  while (!_stop) {
    if (_fileDescriptor == -1) {
      Gd::out.printError("Error: File descriptor is invalid.");
      return -1;
    }
    FD_ZERO(&readFileDescriptor);
    FD_SET(_fileDescriptor, &readFileDescriptor);
    //Timeout needs to be set every time, so don't put it outside of the while loop
    timeval timeval{};
    timeval.tv_sec = timeout / 1000000;
    timeval.tv_usec = timeout % 1000000;
    i = select(_fileDescriptor + 1, &readFileDescriptor, nullptr, nullptr, &timeval);
    switch (i) {
      case 0: //Timeout
        return 1;
      case 1:break;
      default:
        //Error
        closeDevice();
        return -1;
    }
    i = read(_fileDescriptor, &data, 1);
    if (i == -1 || i == 0) {
      if (i == -1 && errno == EAGAIN) continue;
      closeDevice();
      return -1;
    }
    return 0;
  }
  return -1;
}

void Serial::writeData(const std::vector<uint8_t> &data) {
  if (_fileDescriptor == -1) {
    Gd::out.printError("Couldn't write to device \"" + _device + "\", because the file descriptor is not valid.");
    return;
  }
  if (data.empty()) return;
  int32_t bytesWritten = 0;
  ssize_t i;
  while (bytesWritten < (signed)data.size()) {
    i = write(_fileDescriptor, (char *)data.data() + bytesWritten, data.size() - bytesWritten);
    if (i == -1) {
      if (errno == EAGAIN) continue;
      Gd::out.printError("Error writing to serial device \"" + _device + "\" (3, " + strerror(errno) + ").");
      return;
    }
    bytesWritten += i;
  }
}

void Serial::writeChar(char data) {
  if (_fileDescriptor == -1) {
    Gd::out.printError("Couldn't write to device \"" + _device + "\", because the file descriptor is not valid.");
    return;
  }
  int32_t bytesWritten = 0;
  ssize_t i;
  while (bytesWritten < 1) {
    i = write(_fileDescriptor, &data, 1);
    if (i == -1) {
      if (errno == EAGAIN) continue;
      Gd::out.printError("Error writing to serial device \"" + _device + "\" (3, " + std::to_string(errno) + ").");
      return;
    }
    bytesWritten += i;
  }
}

}