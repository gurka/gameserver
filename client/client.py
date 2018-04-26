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

def scenario_movement():
    clients = []
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

def scenario_connect_disconnect():
    clients = []
    next_spawn = tick()
    while True:
        current_tick = tick()

        if len(clients) < NUM_CLIENTS and current_tick >= next_spawn:
            # Spawn new player
            client = Client()
            client.disconnect = current_tick + random.randint(500, 4000)
            protocol = Protocol(client)
            clients.append((client, protocol))
            print("Spawning new client which will disconnect in {}ms. Number of clients: {}".format(client.disconnect - current_tick,
                                                                                                    len(clients)))

            if not protocol.login("Alice", "1"):
                sys.exit(0)

            next_spawn = current_tick + 100

        # Handle each client
        for client, protocol in clients:
            while protocol.handle_packet():
                pass

            if current_tick >= client.disconnect:
                if random.randint(0, 1) == 0:
                    # Gracefully logout
                    protocol.logout()
                else:
                    # Just disconnect
                    protocol.disconnect()

        # Remove clients that are disconnected
        tmp = len(clients)
        clients = [(client, protocol) for client, protocol in clients if protocol.connected()]
        if len(clients) != tmp:
            print("Removed disconnected players. Number of clients: {}".format(len(clients)))

        # Figure out how long to sleep
        if clients:
            next_event = min([client.disconnect for client, protocol in clients])
            if len(clients) < NUM_CLIENTS:
                next_event = min([next_event, next_spawn])
        else:
            next_event = next_spawn

        if next_event - current_tick > 0:
            print("Sleeping {}ms".format(next_event - current_tick))
            time.sleep((next_event - current_tick) / 1000.0)
        else:
            print("No sleep: current_tick: {} next_event {}".format(current_tick, next_event))

def scenario_just_login():
    clients = []

    while len(clients) != NUM_CLIENTS:
        client = Client()
        protocol = Protocol(client)
        clients.append((client, protocol))
        print("Spawning new client")
        if not protocol.login("Alice", "1"):
            sys.exit(0)

    while True:
        for client, protocol in clients:
            while protocol.handle_packet():
                pass

        time.sleep(1)

if __name__ == '__main__':
    NUM_CLIENTS = 30

    random.seed()

    scenario_movement()
    #scenario_connect_disconnect()
    #scenario_just_login()
