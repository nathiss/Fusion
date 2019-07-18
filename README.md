# Fusion

[![Build Status](https://travis-ci.org/nathiss/Fusion.svg?branch=master)](https://travis-ci.org/nathiss/Fusion)
[![MIT license](https://img.shields.io/badge/License-MIT-blue.svg)](https://lbesson.mit-license.org/)

This is the source code of the server for the **Fusion** game.

## Third-party libraries

* [spdlog](https://github.com/gabime/spdlog) - Very fast, header-only/compiled,
  C++ logging library.
* [nlohmann/json](https://github.com/nlohmann/json) - JSON for Modern C++.
* [Google Test](https://github.com/google/googletest) - Google's C++ test framework.
* [Boost](https://www.boost.org/) - free peer-reviewed portable C++ source libraries
  (requires [1.67](https://www.boost.org/users/history/version_1_67_0.html) or
  higher version of Boost installed on the system).

## Build

You can build the server by yourself or build a docker image that contains the
server. Either way please **edit**  respectively `config.json` or
`docker-config.json` file. Section below describes all fields of the configuration
file.

1. Clone the repo and go to its root directory.
```bash
  $ git clone https://github.com/nathiss/Fusion.git
  $ cd Fusion
```

2. Build the docker image.
```bash
  $ docker build --tag nathiss/fusion_server .
```

3. After its successfully build run the image container.
```bash
  $ # Attached to console.
  $ docker run -it -p <host_port>:<port_config> -v <host_dir>:<log_dir_cofnig> nathiss/fusion_server
  $
  $ # Detached
  $ docker run -d -p <host_port>:<port_config> -v <host_dir>:<log_dir_cofnig> nathiss/fusion_server
```

## Configuration

[JSON](https://tools.ietf.org/html/rfc7159) format is used in configuration file.
The file can created anywhere, but remember to change the path in `Dockerfile`.

A valid config file needs to meet these requirements:

* The root of the config file has to be an object.
* `"listener"` field is required and its value must be an object.
    * `"interface"` - an interface on which server will listen for connections (**required**).
        * *Value `"0.0.0.0"` means server will listen on all interfaces.*
    * `"port"` - a port on which server will listen for connections (**required**).
    * `"max_ququed_connections"` - a maximum number of queued connections (**required**).
        * *Value `128` is recommended.*

* `"number_of_additional_threads"` - a number of additional threads (**required**).
    * *Value `0` means no additional threads.*
    * *Value `-1` means server will create `std::thread::hardware_concurrency() - 1` threads.*

* `"logger"` field is optional and its value must be an object.
    * `"root"` - path to log directory (**optional**).
    * `"extension"` - extension for log files (**optional**).
        * *Value **have to** begin with `"."`.*
    * `"level"` - the default log level (**optional**).
        * Possible values of this field:
            * `"trace"`
            * `"debug"`
            * `"info"`
            * `"warn"`
            * `"error"`
            * `"critical"`
    * `"pattern"` - a pattern for log messages (**optional**).
        * *See [spdlog: Custom formatting](https://github.com/gabime/spdlog/wiki/3.-Custom-formatting)
          for more information.*
    * `"register_by_default"` - an indication whether of not new logger should
    be registered in global registry (**optional**).
    * `"flush_every"` - an amount of seconds after all logger would be flushed
    (**optional**).
        * *Note that this value applies to all registers globally.*

## Protocol

This section describes the protocol used in the communication between the server
and its clients.


### Client -> Server

This section describes packages send by a client.

#### JOIN package

This package is a join request to the server. A Client can be only in one game
at a time. If this package was sent and the client has already joined to a game,
the server will close the connection immediately.

```json
{
  "type": "join",
  "game": "<the game's name>",
  "nick": "<player's nick>"
}
```

##### Server's Response

The `result` field can have to values. `joined` means that, the player has
joined to the game. `full` value means that the requested game is full and
joining to it is not possible. The `players` field is an array of objects. Each
object describes one player. The `rays` field is an array of light rays present
in the game at the current moment. It can be empty if there are no rays.

* The client HAS TO save the values of `player_id` fields, all further updates
  about players will contain `player_id` field to identify them.
* The client HAS TO save the values of `ray_id` fields, all further updates
  about rays will contain `ray_id` field to identify them.
* The `my_id` field's value is the id of this client.

**Note**: The `players` and `rays` arrays have meaning only if the joining was
successful, otherwise there won't be included and therefore should not be looked
for.

```json
{
  "type": "join-result",
  "result": "joined|full",
  "my_id": 1337,
  "players": [
    {
      "player_id": 9001,
      "team_id": 0,
      "nick": "<player's nick>",
      "color": [255, 255, 255],
      "health": 100.0,
      "position": [7.6, 87.2],
      "angle": 67.2
    }
  ]
}
```

#### UPDATE package

This package is used to update the position of the player. It should be sent
each time when the player move or rotate.

```json
{
  "type": "update",
  "direction": 12,
  "angle": 67.8
}
```

##### Server's Response

There is no "response" from the server, however the client should not perform
any updates on its own, until it receives an UPDATE package from the server,
which will be broadcasted to all players.

#### LEAVE package

This package is used to leave the current game. This request to the server is
always successful, therefore the server won't respond to it. The `leave` field
can have any value.

```json
{
  "type": "leave"
}
```

##### Server's Response

**Note**: There is no server's response for this request.



### ----(THIS SECTION IS OUTDATED)---- Server -> Client

This section describes the packages send by the server.

### UPDATE package

The `players` array contains players objects present in the game, that have
updated their attributes. If no player has updated its attributes, the array
will be empty, but will not be omitted.
If a player either has joined or left, the player object will contain additional
fields, described below.
The values of `joined` and `left` fields have no meaning and should not be
checked.

The `rays` array contains rays objects of the game.
The values of `new` and `remove` fields have no meaning and should not be
checked. If a ray object has neither `new` nor `remove` field it means that, an
existing ray has been changed. If the game doesn't have any rays or the existing
rays did not change, the array will be empty, but will not be omitted.

```json
{
  "type": "update",
  "players": [
    {
      "player_id": 1,
      "position": [0.0, 0.0],
      "angle": 67.8, 
      "health": 100.0,
      "joined": true,
      "nick": "<new_nick>",
      "role": "<new_role>",
      "color": [255, 255, 255],
      "left": true
    }
  ]
}
```

## License
The MIT License. See `LICENSE.txt` file.
