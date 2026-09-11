# Helper function for adding tests
function(add_cl_test name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE contextlab_lib)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

# Unit Tests
add_cl_test(test_extractors unit/test_extractors.cpp)
add_cl_test(test_schema_validator unit/test_schema_validator.cpp)
add_cl_test(test_router unit/test_router.cpp)

# Integration Tests
add_cl_test(test_cl_exp_000_self_ingestion integration/test_cl_exp_000_self_ingestion.cpp)
add_cl_test(test_pdf_ingestion integration/test_pdf_ingestion.cpp)
add_cl_test(test_deepen_lifecycle integration/test_deepen_lifecycle.cpp)
add_cl_test(test_crud_operations integration/test_crud_operations.cpp)

# Web Smoke Test
add_cl_test(test_web_smoke web/test_web_smoke.cpp)
