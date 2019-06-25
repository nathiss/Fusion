# FusionServer

This is the source code of the server for the **Fusion** game.

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
* The value `0` of the `player_id` has special meaning. It identifies the
  information about the player itself.

**Note**: The `players` and `rays` arrays have meaning only if the joining was
successful, otherwise there won't be included and therefore should not be looked
for.

```json
{
  "id": 0,
  "result": "joined|full",
  "players": [
    {
      "player_id": 0,
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
  "id": 0,
  "position": [7.6, 87.2],
  "angle": 67.2 // <0.0; 360.0)
}
```

##### Server's Response

If the update of the player's attributes is valid, the server will set
`successful` field to `true` and broadcast a server's update package to all
other players in the game, otherwise the server will send a response with
`successful` set to `false` and no broadcast will take place., therefore the
client should not update it's attribures immediately and wait for either
a response with mached `id` or a server's update packege.

```json
{
  "id": 0,
  "successful": true
}
```

#### LEAVE package

This package is used to leave the current game. This request to the server is
always successful, therefore the server won't repond to it. The `leave` field
can have any value.

**Note**: If the player hasn't joined to any game and will send this package,
the server will find the connection to be broken and will close it immediately.

```json
{
  "leave": true
}
```


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