/**
 * @file package_verifier.hpp
 *
 * This module is a part of Fusion Server project.
 * It declares the PackageVerifier class.
 *
 * (c) 2019 by Kamil Rusin
 */

#pragma once

#include <utility>

#include <fusion_server/package_parser.hpp>

namespace fusion_server {

/**
 * This class is a middle-ware between WebSocketSession and
 * IncommingPackageDelegate. It is used to verify is the given package is:
 * - a valid JSON
 * - a valid game package (e.g. it has to have a "type" field)
 * - a valid package of its type (after checing its type it check if is
 *   contains all other field required by its type, as well).
 */
class PackageVerifier {
 public:
  /**
   * This method returns a pair of an indication whether or not the verification
   * was successful and a JSON object which is either a parsed package or
   * an error message.
   *
   * @param[in] raw_package
   *   The raw package read from a client.
   *
   * @return
   *   A pair of an indication whether or not the verification was successful
   *   and a JSON object which is either a parsed package or an error message is
   *   returned.
   */
  std::pair<bool, PackageParser::JSON>
  Verify(const std::string& raw_package) const noexcept;

 private:
  /**
   * This method returns a JSON object contains an error message for the client
   * informing about not valid JSON syntax.
   *
   * @return
   *   A JSON object contains an error message for the client informing about
   *   not valid JSON syntax is returned.
   */
  PackageParser::JSON MakeNotValidJSON() const noexcept;

  /**
   * This method returns a JSON object contains an error message for the client
   * informing that "type" field is not present is the received package.
   *
   * @return
   *   A JSON object contains an error message for the client informing that
   *   "type" field is not present is the received package is returned.
   */
  PackageParser::JSON MakeTypeNotFound() const noexcept;

  /**
   * This method returns a JSON object contains an error message for the client
   * informing that a "JOIN" package was ill-formed.
   *
   * @return
   *   A JSON object contains an error message for the client informing that
   *   a "JOIN" package was ill-formed is returned.
   */
  PackageParser::JSON MakeNotValidJoin() const noexcept;

  /**
   * This method returns a JSON object contains an error message for the client
   * informing that a "UPDATE" package was ill-formed.
   *
   * @return
   *   A JSON object contains an error message for the client informing that
   *   a "UPDATE" package was ill-formed is returned.
   */
  PackageParser::JSON MakeNotValidUpdate() const noexcept;

  /**
   * This method returns a JSON object contains an error message for the client
   * informing that a "LEAVE" package was ill-formed.
   *
   * @return
   *   A JSON object contains an error message for the client informing that
   *   a "LEAVE" package was ill-formed is returned.
   */
  PackageParser::JSON MakeNotValidLeave() const noexcept;

  /**
   * This method returns a JSON object contains an error message for the client
   * informing that a package was unidentified.
   *
   * @return
   *   A JSON object contains an error message for the client informing that
   *   package was unidentified is returned.
   */
  PackageParser::JSON MakeUnidentified() const noexcept;

  /**
   * This is a package parser.
   */
  PackageParser package_parser_;
};

}  // namespace fusion_server
