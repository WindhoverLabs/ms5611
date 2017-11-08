
add_airliner_app_def(ms5611
    FILE MS5611
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/ms5611_app.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/ms5611_app.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/ms5611_events.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/ms5611_msg.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/ms5611_version.h
   
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/

    USER_DOCS   ${CMAKE_CURRENT_SOURCE_DIR}/user_doxy.in
    DESIGN_DOCS ${CMAKE_CURRENT_SOURCE_DIR}/detail_doxy.in
)

add_airliner_app_unit_test(ms5611-ut
    UTASSERT
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/../unit_test/ms5611_app_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../unit_test/ms5611_app_test.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../unit_test/ms5611_cmds_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../unit_test/ms5611_cmds_test.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../unit_test/ms5611_testrunner.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../unit_test/ms5611_test_utils.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../unit_test/ms5611_test_utils.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/ms5611_app.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/ms5611_app.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/ms5611_events.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/ms5611_msg.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/ms5611_version.h
        
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/
        ${CMAKE_CURRENT_SOURCE_DIR}/../unit_test/
        
    VALGRIND_SUPPRESSION_FILE
        ${CMAKE_CURRENT_SOURCE_DIR}/../unit_test/valgrind.supp
)
