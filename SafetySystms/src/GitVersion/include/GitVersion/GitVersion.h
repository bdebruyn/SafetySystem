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
      std::string commit { "d577de6" };
      std::string dev { "Bill de Bruyn" };
      std::string tag { "<no tag>" };
      std::string timestamp { "2025-09-28 16:05:26.757071-05:00" };
      std::string version { "main:(d577de6):2025-09-28 16:05:26.757071-05:00: <no tag> Bill de Bruyn: SEI:1.0.0" };
      std::string conanName { "SEI" };
      std::string conanVersion { "1.0.0" };
};
