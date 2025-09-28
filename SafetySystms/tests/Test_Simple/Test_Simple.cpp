#include "gtest/gtest.h"

#include "Simple/Simple.h"

#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include <fstream>
#include <thread>
#include <vector>

#include <iostream>
using namespace std;
using namespace std::chrono;
using namespace cct;

namespace Test_Simple_Namespace 
{

   struct Test_Simple : public virtual ::testing::Test
   {
   
      Test_Simple() {}
      virtual ~Test_Simple() {}
      void SetUp() {}
      void TearDown() {}
   };
   
   TEST_F(Test_Simple, simple)
   {
      using namespace cct;
      Simple sut;
   }

}
