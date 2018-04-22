#!/usr/bin/env python3
import signal
import sys
import random
from threading import Timer

from protocol import Protocol
from world import Item, Creature, Tile

CHARACTER_NAME = "Alice"
PASSWORD = "1"

TILE_WATER = 475
TILE_GROUND = 694

class Client():
    def __init__(self):
        self.player_id = 0
        self.player_position = (0, 0, 0)
        self.tiles = {}


def print_tiles(client):
    x, y, z = client.player_position
    for y_ in range(y - 7, y + 7):
        for x_ in range(x - 9, x + 9):
            tile = client.tiles[(x_, y_)]
            if tile.creatures:
                for creature in tile.creatures:
                    if creature.creature_id == client.player_id:
                        print('P', end='')
                        break
                else:
                    print('C', end='')
            elif tile.items:
                print('I', end='')
            elif tile.ground.type_id == TILE_WATER:
                print('#', end='')
            elif tile.ground.type_id == TILE_GROUND:
                print('.', end='')
            else:
                print('?', end='')

        print()


if __name__ == '__main__':
    random.seed()

    client = Client()
    protocol = Protocol(client)

    if not protocol.login(CHARACTER_NAME, PASSWORD):
        sys.exit(0)

    print_tiles(client)

    def signal_handler(*args):
        protocol.logout()
        sys.exit(0)

    signal.signal(signal.SIGINT, signal_handler)

    def handle_timer():
        while protocol.handle_packet():
            pass

        protocol.move(random.randint(0, 3))
        timer = Timer(random.uniform(0.2, 2.0), handle_timer)
        timer.start()

    handle_timer()
