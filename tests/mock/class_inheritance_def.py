class Animal:
    name: str

class Dog(Animal):
    tails: int

    def __init__(self, name: str):
        self.name = name