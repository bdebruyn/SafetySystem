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
      std::string commit { "df7f602" };
      std::string dev { "Bill de Bruyn" };
      std::string tag { "<no tag>" };
      std::string timestamp { "2025-10-02 17:51:55.783024-05:00" };
      std::string version { "main:(df7f602):2025-10-02 17:51:55.783024-05:00: <no tag> Bill de Bruyn: SEI:1.0.0" };
      std::string conanName { "SEI" };
      std::string conanVersion { "1.0.0" };
};
