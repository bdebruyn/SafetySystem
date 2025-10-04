set(tests
   Test_SafetyRules
)

set(libraries
   SafetyRules
)

UnitTest_All("${tests}" "${libraries}")
