
add_executable(tests testbase.cpp admxmltest.cpp)
target_include_directories(tests PRIVATE "${BBCAT_COMMON_DIR}/include")
target_link_libraries(tests bbcat-fileio${LINKTYPE} bbcat-adm${LINKTYPE} bbcat-dsp${LINKTYPE} bbcat-base${LINKTYPE})

# create custom target to run tests
add_custom_target(test ALL
				  DEPENDS tests
				  COMMAND tests --use-colour no
				  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
