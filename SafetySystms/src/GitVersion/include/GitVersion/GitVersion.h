#pragma once
#include <string>

class GitVersion
{
   public:
      std::string getInfo()
      {
          return version;
      }
      
      std::string branch { "main" };
      std::string commit { "5add3c9" };
      std::string dev { "Bill de Bruyn" };
      std::string tag { "<no tag>" };
      std::string timestamp { "2025-09-29 15:37:56.746749-05:00" };
      std::string version { "main:(5add3c9):2025-09-29 15:37:56.746749-05:00: <no tag> Bill de Bruyn: SEI:1.0.0" };
      std::string conanName { "SEI" };
      std::string conanVersion { "1.0.0" };
};
