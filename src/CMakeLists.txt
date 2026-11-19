# Everything except main() lives in a static library, so tests can link against it.
add_library(library_core STATIC
    domain/error.cpp
    domain/text.cpp
    domain/book.cpp
    domain/student.cpp
    storage/sqlite.cpp
    storage/schema.cpp
    storage/sqlite_repositories.cpp
    service/library_service.cpp
    service/sodium_password_hasher.cpp
    service/auth_service.cpp
    ui/console.cpp
    ui/console_app.cpp)
target_include_directories(library_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(library_core PUBLIC cxx_std_20)
target_link_libraries(library_core PUBLIC tl::expected PRIVATE SQLite::SQLite3 sodium)
library_enable_warnings(library_core)

add_executable(library_app main.cpp)
target_link_libraries(library_app PRIVATE library_core)
library_enable_warnings(library_app)
