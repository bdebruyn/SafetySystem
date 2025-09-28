set(target "GitVersion")

message(STATUS "Lib ${target}")

set(include_path "${CMAKE_CURRENT_SOURCE_DIR}/include/${target}")
set(source_path  "${CMAKE_CURRENT_SOURCE_DIR}/src")

set(headers
   ${include_path}/GitVersion.h
)

set(sources
   ${source_path}/GitVersion.cpp
)

add_library(${target} STATIC
   ${sources}
   ${headers}
)

target_include_directories(${target}
   PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/include

   PRIVATE
   INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include/${target}>
)

if (SDK)
   target_link_libraries(${target}
      PRIVATE
   
      PUBLIC
      
      INTERFACE
   )
else ()
   target_link_libraries(${target}
      PRIVATE
   
      PUBLIC
      
      INTERFACE
   )
endif ()
    