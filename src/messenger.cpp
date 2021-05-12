#include "messenger.h"

Messenger& Messenger::GetInstance() {
  static Messenger instance;
  return instance;
}

void Messenger::OutputToFile(std::string filepath) {
  std::cout << "Implement me!" << std::endl;
}

void Messenger::ErrorMessage(std::string message, std::string filename) {
  throw std::runtime_error("ERROR ->" + filename + " " + message);
}


void Messenger::ErrorMessage(std::string message) {
  throw std::runtime_error("ERROR: " + message);
}


void Messenger::Log(std::string msg) {
  std::cout << "Implement me!" << std::endl;
}

void Messenger::Log(std::string msg, std::string filename)
{ 
  std::cout << "Implement me!" << std::endl;
}

void Messenger::LogToConsole(std::string msg) {
  std::cout << msg << std::endl;
}
