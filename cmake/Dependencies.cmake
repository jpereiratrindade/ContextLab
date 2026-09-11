include(FetchContent)

# 1. nlohmann_json
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

# 2. json-schema-validator
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    nlohmann_json_schema_validator
    GIT_REPOSITORY https://github.com/pboettch/json-schema-validator.git
    GIT_TAG        2.3.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(nlohmann_json_schema_validator)

# 3. cpp-httplib
set(HTTPLIB_REQUIRE_OPENSSL OFF CACHE BOOL "" FORCE)
set(HTTPLIB_REQUIRE_ZLIB OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG        v0.18.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(httplib)

# 4. SQLite3
find_package(SQLite3 REQUIRED)

# 5. qpdf
find_package(qpdf CONFIG QUIET)
if(NOT qpdf_FOUND)
    find_library(QPDF_LIB NAMES qpdf libqpdf libqpdf.so.30 libqpdf.so PATHS /usr/lib64 /usr/lib /usr/local/lib)
    if(NOT QPDF_LIB AND EXISTS "/usr/lib64/libqpdf.so.30")
        set(QPDF_LIB "/usr/lib64/libqpdf.so.30")
    endif()
    FetchContent_Declare(
        qpdf_src
        GIT_REPOSITORY https://github.com/qpdf/qpdf.git
        GIT_TAG        v12.3.2
        GIT_SHALLOW    TRUE
    )
    FetchContent_GetProperties(qpdf_src)
    if(NOT qpdf_src_POPULATED)
        FetchContent_Populate(qpdf_src)
    endif()

    add_library(qpdf_target INTERFACE)
    target_include_directories(qpdf_target INTERFACE ${qpdf_src_SOURCE_DIR}/include)
    if(QPDF_LIB)
        target_link_libraries(qpdf_target INTERFACE ${QPDF_LIB})
    endif()
    add_library(qpdf::qpdf ALIAS qpdf_target)
    set(qpdf_FOUND TRUE)
endif()
