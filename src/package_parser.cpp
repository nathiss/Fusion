#include <fusion_server/package_parser.hpp>

namespace fusion_server {

auto PackageParser::Parse(const std::string& package) noexcept
    -> std::optional<JSON> {
  try {
    return JSON::parse(package);
  }
  catch(...) {
    return {};
  }
}

}  // namespace fusion_server