#include <cassert>
#include <iostream>
#include <fstream>
#include "contextlab/metadata/JsonSchemaValidator.hpp"
#include "contextlab/metadata/SchemaRegistry.hpp"

void testSchemaValidation() {
    nlohmann::json schema = {
        {"$schema", "http://json-schema.org/draft-07/schema#"},
        {"type", "object"},
        {"required", {"id", "title"}},
        {"properties", {
            {"id", {{"type", "string"}}},
            {"title", {{"type", "string"}}}
        }}
    };

    contextlab::metadata::JsonSchemaValidator validator(schema);

    nlohmann::json valid_payload = {{"id", "DOC-01"}, {"title", "Test Title"}};
    auto res_ok = validator.validate(valid_payload);
    assert(res_ok.has_value());

    nlohmann::json invalid_payload = {{"id", 123}};
    auto res_err = validator.validate(invalid_payload);
    assert(!res_err.has_value());
    assert(res_err.error().code == contextlab::core::ErrorCode::METADATA_SCHEMA_INVALID);

    std::cout << "✓ testSchemaValidation passed\n";
}

int main() {
    std::cout << "Running Schema Validator Unit Tests...\n";
    testSchemaValidation();
    std::cout << "All Schema Validator Unit Tests PASS!\n";
    return 0;
}
