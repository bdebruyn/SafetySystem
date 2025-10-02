set(sources
   RefinedSafetyRules
)

set(headersOnly
   IRefinedSafetyRules
)

set(libraries
)

StaticLib_All("${sources}" "${headersOnly}" "${libraries}")
