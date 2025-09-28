#pragma once

#include <string>
#include <iostream>

namespace cct 
{
   class Simple
   {
      void print(const std::string& name)
      {
         std::cout << "Hello " << name << std::endl;
      }
   };
}
