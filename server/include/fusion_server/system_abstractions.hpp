#pragma once

#include <functional>
#include <memory>
#include <string>

namespace fusion_server {

/**
 * This is the forward declaration of the WebSocketSession class.
 */
class WebSocketSession;

namespace system_abstractions {

/**
 * This type is used to create delegates that will be called by WebSocket
 * sessions each time, when a new package arrives.
 */
using IncommingPackageDelegate = std::function< void(std::shared_ptr<const std::string>, WebSocketSession*) >;

}  // namespace system_abstractions

}  // namespace fusion_server