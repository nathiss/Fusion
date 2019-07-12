# Fusion

This is the source code of the server for the **Fusion** game.

## Third-party libraries

* [spdlog](https://github.com/gabime/spdlog) - Very fast, header-only/compiled,
  C++ logging library.
* [nlohmann/json](https://github.com/nlohmann/json) - JSON for Modern C++.
* [Google Test](https://github.com/google/googletest) - Google's C++ test framework.
* [Boost](https://www.boost.org/) - free peer-reviewed portable C++ source libraries
  (requires the Boost 1.67 installed on the system).

## Protocol

This section describes the protocol used in the connunication between the server
and its clients.


### Common part
Each package CAN have the `id` field. This is a random number to identify this
package in the session. The other side of the connection can send a packge with
the same `id` to indicate, that it's a response to the previous one.


### Client -> Server

This section describes packages send by a client.

#### JOIN package

This package is a join request to the server. A Client can be only in one game
at a time. If this packge was sent and the client has already joined to a game,
the server will close the connection imediately.

```json
{
  "id": 0,
  "type": "join",
  "game": "<the game's name>",
  "nick": "<player's nick>"
}
```

##### Server's Response

The `result` field can have to values. `joined` means that, the player has
joined to the game. `full` value means that the requested game is full and
joining to it is not possible. The `players` field is an array of objects. Each
object describes one player. The `rays` field is an array of light rays presnt
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
  "id": 0,
  "result": "joined|full",
  "my_id": 1337,
  "players": [
    {
      "player_id": 0,
      "team_id": 0,
      "nick": "<player's nick>",
      "role": "<player's role>",
      "color": [255, 255, 255],
      "health": 100.0,
      "position": [7.6, 87.2],
      "angle": 67.2 // <0.0; 360.0)
    },
    // ...
  ],
  "rays": [
    {
      "ray_id": 0,
      "source": [7.6, 87.2],
      "destination": [17.6, 187.2],
      "color": [255, 255, 255]
    },
    // ...
  ]
}
```

#### UPDATE package

This package is used to update the position of the player. It should be sent
each time when the player either moved, rotated or fired.

```json
{
  "type": "update",
  "team_id": 0,
  "position": [7.6, 87.2],
  "angle": 67.2 // <0.0; 360.0)
}
```

##### Server's Response

There is no "response" from the server, however the client should not perform
any updates on its own sprite, until it receives an UPDATE package from the
server, which will be broadcasted to all players.

#### LEAVE package

This package is used to leave the current game. This request to the server is
always successful, therefore the server won't repond to it. The `leave` field
can have any value.

```json
{
  "type": "leave"
}
```

##### Server's Response

**Note**: There is no server's response for this request.


### Server -> Client

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
rays didn't change, the array will be empty, but will not be omitted.

```json
{
  "type": "update",
  "players": [
    {
      "player_id": 1,
      "position": [0.0, 0.0],
      "angle": 67.8, // <0.0; 360.0)
      "health": 100.0,
      "joined": true, // only when joined
      "nick": "<new_nick>", // only when joined
      "role": "<new_role>", // only when joined
      "color": [255, 255, 255], // // only when joined
      "left": true // only when left
    },
    // ...
  ],
  "rays": [
    {
      "ray_id": 1,
      "source": [0.0, 0.0],
      "destination": [0.0, 0.0],
      "new": true, // only when a ray is a new one
      "remove": true // only when a ray should be removed
    },
    // ...
  ]
}
```

## License
The MIT License. See `LICENSE.txt` file.
