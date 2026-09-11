#pragma once

#include <string>
#include <string_view>

namespace contextlab::domain {

enum class Authority {
    DECLARED,
    DERIVED,
    EXTERNAL
};

[[nodiscard]] constexpr std::string_view authorityToString(Authority auth) noexcept {
    switch (auth) {
        case Authority::DECLARED: return "author_declared";
        case Authority::DERIVED:  return "system_derived";
        case Authority::EXTERNAL: return "external_imported";
    }
    return "unknown";
}

[[nodiscard]] constexpr Authority stringToAuthority(std::string_view str) noexcept {
    if (str == "author_declared" || str == "DECLARED" || str == "declared") return Authority::DECLARED;
    if (str == "system_derived" || str == "DERIVED" || str == "derived") return Authority::DERIVED;
    if (str == "external_imported" || str == "EXTERNAL" || str == "external") return Authority::EXTERNAL;
    return Authority::DECLARED;
}

enum class ValidationState {
    VALID,
    INVALID,
    UNKNOWN_SCHEMA,
    UNVALIDATED
};

[[nodiscard]] constexpr std::string_view validationStateToString(ValidationState state) noexcept {
    switch (state) {
        case ValidationState::VALID:          return "VALID";
        case ValidationState::INVALID:        return "INVALID";
        case ValidationState::UNKNOWN_SCHEMA: return "UNKNOWN_SCHEMA";
        case ValidationState::UNVALIDATED:    return "UNVALIDATED";
    }
    return "UNKNOWN";
}

[[nodiscard]] constexpr ValidationState stringToValidationState(std::string_view str) noexcept {
    if (str == "VALID") return ValidationState::VALID;
    if (str == "INVALID") return ValidationState::INVALID;
    if (str == "UNKNOWN_SCHEMA") return ValidationState::UNKNOWN_SCHEMA;
    return ValidationState::UNVALIDATED;
}

} // namespace contextlab::domain
