#include <utility>

#include <fusion_server/package_parser.hpp>

namespace fusion_server {

auto PackageParser::Parse(std::string package) const noexcept
    -> std::optional<JSON> {
  try {
    return JSON::parse(std::move(package));
  }
  catch(...) {
    return {};
  }
}

}  // namespace fusion_server