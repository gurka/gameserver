DIRECTION_NORTH = 0
DIRECTION_EAST  = 1
DIRECTION_SOUTH = 2
DIRECTION_WEST  = 3

class Creature():
    def __init__(self, creature_id, name):
        self.creature_id = creature_id
        self.name = name

    def __str__(self):
        return "Creature id: {} name: {}".format(self.creature_id, self.name)

    def __repr__(self):
        return str(self)

class Item():
    def __init__(self, type_id):
        self.type_id = type_id

    def __str__(self):
        return "Item type_id: {}".format(self.type_id)

    def __repr__(self):
        return str(self)

class Tile():
    def __init__(self):
        self.ground = None
        self.items = []
        self.creatures = []

    def __str__(self):
        return "Tile ground: {} items: {} creatures: {}".format(self.ground, self.items, self.creatures)

    def __repr__(self):
        return str(self)
