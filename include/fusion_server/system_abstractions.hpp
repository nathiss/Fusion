/**
 * @file system_abstractions.hpp
 *
 * This module is a part of Fusion Server project.
 * It declares the common abstractions of the server.
 *
 * Copyright 2019 Kamil Rusin
 */


#pragma once

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include <fusion_server/json.hpp>

namespace fusion_server {

/**
 * This is the forward declaration of the WebSocketSession class.
 */
class WebSocketSession;

namespace system_abstractions {

/**
 * This is the package type used in both WebSocket and HTTP sessions.
 */
using Package = const std::string;

/**
 * This type is used to create delegates that will be called by WebSocket
 * sessions each time, when a new package arrives.
 *
 * @param[in] package
 *   The package received from the client.
 *
 * @param[in] session
 *   The session connected to the client.
 */
using IncommingPackageDelegate = std::function< void(const json::JSON&  package, WebSocketSession* session) >;

}  // namespace system_abstractions

}  // namespace fusion_server
