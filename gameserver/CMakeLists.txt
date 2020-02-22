cmake_minimum_required(VERSION 3.12)

project(gameserver)

option(GAMESERVER "Gameserver Build" OFF)
option(WSCLIENT "WSClient Build" OFF)
if (GAMESERVER AND (NOT WSCLIENT))
  include(gameserver.cmake)
elseif(WSCLIENT AND (NOT GAMESERVER))
  include(wsclient.cmake)
else()
  message(FATAL_ERROR "You need to set one and only one of GAMESERVER and WSCLIENT")
endif()
