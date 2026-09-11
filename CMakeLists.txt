cmake_minimum_required(VERSION 3.28)
project(ContextLab VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler options
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O2)
endif()

# Dependencies
include(cmake/Dependencies.cmake)

# Library target
add_library(contextlab_lib STATIC
    src/core/Sha256.cpp
    src/core/Logger.cpp
    src/core/Config.cpp
    src/metadata/JsonSchemaValidator.cpp
    src/metadata/SchemaRegistry.cpp
    src/ingest/ContentAddressableStore.cpp
    src/ingest/MarkdownExtractor.cpp
    src/ingest/JsonMetadataExtractor.cpp
    src/ingest/LatexExtractor.cpp
    src/ingest/TxtExtractor.cpp
    src/ingest/PdfExtractor.cpp
    src/ingest/IngestionPipeline.cpp
    src/persistence/Database.cpp
    src/persistence/Migrations.cpp
    src/persistence/Repository.cpp
    src/retrieval/RetrievalRouter.cpp
    src/retrieval/SearchEngine.cpp
    src/analysis/SelectiveDeepener.cpp
    src/application/ContextLabService.cpp
    src/web/HttpServer.cpp
)

target_include_directories(contextlab_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(contextlab_lib PUBLIC
    nlohmann_json::nlohmann_json
    nlohmann_json_schema_validator
    httplib::httplib
    SQLite3::SQLite3
)

if(TARGET qpdf::qpdf)
    target_link_libraries(contextlab_lib PUBLIC qpdf::qpdf)
endif()

# Main executable
add_executable(contextlab src/main.cpp)
target_link_libraries(contextlab PRIVATE contextlab_lib)

# Testing
enable_testing()
add_subdirectory(tests)
