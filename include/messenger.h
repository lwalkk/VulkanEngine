#pragma once

#include "headers.h"
#include <string>

class Messenger {
public:

  Messenger(Messenger& copy) = delete;

  static Messenger& GetInstance();

  void OutputToFile(std::string filepath);
  void ErrorMessage(std::string message);
  void ErrorMessage(std::string message, std::string filename);
  void Log(std::string msg);
  void Log(std::string msg, std::string filename);
  void LogToConsole(std::string msg);
  
  void operator=(const Messenger&) = delete;

private:
  Messenger();

};
