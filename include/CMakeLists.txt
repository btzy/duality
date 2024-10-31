set(TARGET_NAME duality)

# Define the library
add_library(${TARGET_NAME} INTERFACE)

# Allow includes from include/
target_include_directories(${TARGET_NAME}
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:.>
)

set_target_properties(${TARGET_NAME} PROPERTIES VERSION ${PROJECT_VERSION})

if (DUALITY_INSTALL)
  include(GNUInstallDirs)
  install(TARGETS ${TARGET_NAME} EXPORT ${TARGET_NAME}-export PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()
