cninja_require(static-release)
cninja_require(linker-warnings=no)

set_cache(BUILD_SHARED_LIBS OFF)
set_cache(CMAKE_INSTALL_MESSAGE NEVER)

set_cache(SCORE_DEPLOYMENT_BUILD 1)
set_cache(SCORE_INSTALL_HEADERS ON)
set_cache(OSSIA_STATIC_EXPORT ON)
