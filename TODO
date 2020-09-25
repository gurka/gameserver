General:
  * Improve shutdown
    - When shutdown is detected (ctrl + c),
      stop all servers/connections/gameenginequeue/timer and rerun io_service
    - io_service should return eventually, then we can delete everything and exit
    - Don't use io_service.stop()

  * Fix TODOs in the code

  * We have three things where Items can be located:
    - Tile (World)
    - Equipment (Player, GameEngine)
    - Container (ContainerManager, GameEngine)

    All these have canAddItem, addItem and removeItem.
    addItem and removeItem should return success/failure (bool),
    so that we do not duplicate or make Items disappear.

  * All three "containers" (Tile, Equipment, Container)
    should work with and hold 'const Thing*'s. Equipment and
    Container should reject Creature Things.

    Actual Item objects are owned and managed by ItemManager
    Actual Creature objects are owned and managed by... GameEngine?

  * Find more good warning flags to enable

  * Remove io from logger and config

  * Cleanup network module
    - Separate server/client in namespaces
    - Handle different backends better and easier

ContainerManager:
  * Add support for weight calculation

WSClient:
  * Use global position for xdiv,ydiv
  * Add chat/message box and input field/button to send
  * Add arrow key handling to move creature
  * Fix namespaces chaos

WSClient-replay:
  * Spacebar to pause replay
  * left/right to speed down/up
  * lots of bugs xD

Docker:
  * Troubleshoot emscripten cache issue (always downloading SDL lib)
  * Create docker image(s) with make target
  * (non-Docker): see if we can merge regular and wsclient cmake config
    and just skip wsclient if emscripten is not available
