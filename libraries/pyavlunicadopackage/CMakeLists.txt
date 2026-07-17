# Add the package to the package list for exporting the target
# and propagate the resulting list back to the parent scope
list( APPEND PYTHON_TARGETS ${CMAKE_CURRENT_LIST_DIR} )
set( PYTHON_TARGETS ${PYTHON_TARGETS} PARENT_SCOPE )

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR} DESTINATION lib)
