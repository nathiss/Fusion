#pragma once

#include <nlohmann/json.hpp>

namespace fusion_server {

/**
 * This class is a JSON parser used to parse incomming messages from the
 * WebSocket sessions.
 */
class PackageParser {
 public:
  /**
   * This type is a shortcut.
   */
  using JSON = nlohmann::json;

  /**
   * This method parses the given packge. It returns parsed package.
   * If the package cannot be parsed (e.g. it contains ill-formed JSON),
   * the result has no value.
   *
   * @param[in] package
   *   The package to be parsed.
   *
   * @return
   *  The parsed package or no value, if the package cannot be properly parsed.
   */
  std::optional<JSON> Parse(std::shared_ptr<const std::string> package) noexcept;
};

}  // namespace fusion_server