#pragma once

#include <expected>
#include <type_traits>
#include "contextlab/core/Error.hpp"

namespace contextlab::core {

template <typename T>
using Result = std::expected<T, Error>;

template <typename T>
[[nodiscard]] Result<std::decay_t<T>> makeOk(T&& value) {
    return Result<std::decay_t<T>>(std::forward<T>(value));
}

inline Result<void> makeOk() {
    return Result<void>();
}

inline std::unexpected<Error> makeError(ErrorCode code, std::string message, nlohmann::json details = nlohmann::json::object(), bool recoverable = true) {
    return std::unexpected(Error{
        .code = code,
        .message = std::move(message),
        .details = std::move(details),
        .recoverable = recoverable
    });
}

} // namespace contextlab::core
