#!/usr/bin/env python3
import signal
import sys
import time
import random
from threading import Timer

from protocol import Protocol
from world import Item, Creature, Tile

class Client():
    def __init__(self):
        self.player_id = 0
        self.player_position = (0, 0, 0)
        self.tiles = {}
        self.next_move = 0

def print_tiles(client):
    TILE_WATER = 475
    TILE_GROUND = 694

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

def tick():
    return int(round(time.time() * 1000))

if __name__ == '__main__':
    NUM_CLIENTS = 20

    clients = []

    random.seed()

    def signal_handler(*args):
        for client, protocol in clients:
            protocol.logout()
        sys.exit(0)

    signal.signal(signal.SIGINT, signal_handler)

    next_spawn = tick()
    while True:
        current_tick = tick()

        if len(clients) < NUM_CLIENTS and current_tick >= next_spawn:
            # Spawn new player
            client = Client()
            client.next_move = current_tick + random.randint(500, 2000)
            protocol = Protocol(client)
            clients.append((client, protocol))
            print("Number of clients: {}".format(len(clients)))

            if not protocol.login("Alice", "1"):
                sys.exit(0)

            next_spawn = current_tick + 1000

        # Handle each client
        for client, protocol in clients:
            while protocol.handle_packet():
                pass

            if current_tick >= client.next_move:
                protocol.move(random.randint(0, 3))
                client.next_move = current_tick + random.randint(500, 2000)

        # Figure out how long to sleep
        next_event = min([client.next_move for client, protocol in clients])
        if len(clients) < NUM_CLIENTS:
            next_event = min([next_event, next_spawn])

        if next_event - current_tick > 0:
            print("Sleeping {}ms".format(next_event - current_tick))
            time.sleep((next_event - current_tick) / 1000.0)
        else:
            print("No sleep: current_tick: {} next_event {}".format(current_tick, next_event))
