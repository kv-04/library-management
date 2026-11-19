include(GoogleTest)

add_executable(library_tests
    domain/error_test.cpp
    domain/text_test.cpp
    domain/book_test.cpp
    domain/student_test.cpp
    storage/sqlite_test.cpp
    storage/repositories_test.cpp
    service/library_service_test.cpp
    service/auth_service_test.cpp
    ui/console_test.cpp
    ui/console_app_test.cpp)
target_link_libraries(library_tests PRIVATE library_core GTest::gtest_main)
library_enable_warnings(library_tests)

gtest_discover_tests(library_tests)
