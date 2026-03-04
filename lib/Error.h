#pragma once

#include <stdexcept>
#include <string>

namespace dinorisc {

class Error : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

class ELFError : public Error {
public:
  using Error::Error;
};

class DecodingError : public Error {
public:
  using Error::Error;
};

class UnsupportedInstructionError : public Error {
public:
  using Error::Error;
};

class EncodingError : public Error {
public:
  using Error::Error;
};

class LoweringError : public Error {
public:
  using Error::Error;
};

class RuntimeError : public Error {
public:
  using Error::Error;
};

} // namespace dinorisc
