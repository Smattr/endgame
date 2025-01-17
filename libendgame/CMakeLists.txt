add_library(endgame
  src/input.c
  src/io.c
  src/output.c
)

target_include_directories(endgame
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    src
)

install(TARGETS endgame EXPORT endgameConfig
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(
  EXPORT endgameConfig
  FILE endgameConfig.cmake
  NAMESPACE endgame::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/endgame
)
