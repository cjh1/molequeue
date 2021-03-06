add_library(mqconnection message.cpp connection.h)
set_target_properties(mqconnection PROPERTIES AUTOMOC TRUE)
target_link_libraries(mqconnection ${QT_LIBRARIES})

add_library(mqconnectionlistener connectionlistener.h)
set_target_properties(mqconnectionlistener PROPERTIES AUTOMOC TRUE)
target_link_libraries(mqconnectionlistener ${QT_LIBRARIES})

set(hdrs
  connection.h
  connectionlistener.h
  connectionlistenerfactory.h
  message.h)

generate_export_header(mqconnection EXPORT_FILE_NAME mqconnectionexport.h)
generate_export_header(mqconnectionlistener EXPORT_FILE_NAME mqconnectionlistenerexport.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_compiler_export_flags(molequeue_export_flags)
set_property(TARGET mqconnection APPEND
  PROPERTY COMPILE_FLAGS ${molequeue_export_flags})
set_property(TARGET mqconnectionlistener APPEND
  PROPERTY COMPILE_FLAGS ${molequeue_export_flags})

list(APPEND hdrs
  "${CMAKE_CURRENT_BINARY_DIR}/mqconnectionexport.h"
  "${CMAKE_CURRENT_BINARY_DIR}/mqconnectionlistenerexport.h")

install(FILES ${hdrs} DESTINATION "${INSTALL_INCLUDE_DIR}/molequeue/transport")
install(TARGETS mqconnection mqconnectionlistener
  EXPORT "MoleQueueTargets"
  RUNTIME DESTINATION ${INSTALL_RUNTIME_DIR}
  LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
  ARCHIVE DESTINATION ${INSTALL_ARCHIVE_DIR}
  )

add_subdirectory(localsocket)

# Are we using ZeroMQ
if(USE_ZERO_MQ)
  add_subdirectory(zeromq)
endif()
