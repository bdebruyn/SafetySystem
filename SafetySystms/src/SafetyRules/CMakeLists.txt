set(sources
   SafetyRules
)

set(headersOnly
   ISafetyRules
)

set(libraries
)

StaticLib_All("${sources}" "${headersOnly}" "${libraries}")
