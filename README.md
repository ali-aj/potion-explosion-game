# Potion Explosion Game

Welcome to the Potion Explosion game! This is a console-based game where players brew magical potions by strategically selecting colored balls from a dispenser. The objective is to brew the most potent potions and earn the highest score.

## How to Play

### Setup
1. Compile and run the provided C++ code to start the game.
2. Enter the names of two players when prompted.

### Gameplay
- Players take turns to:
  - Choose potion tiles.
  - Select balls from the dispenser.
  - Brew potions by matching the required ingredients with the selected balls.
- Each potion has its own set of required ingredients and a corresponding score.
- Players earn points by brewing potions successfully.

### Winning
- The game ends when all potions are brewed.
- The player with the highest score wins the game.

## Rules
1. Each turn, players can:
   - Select potion tiles if they have less than 2 potions.
   - Pick a ball from the dispenser.
   - Brew potions using the available balls.
2. Balls selected from the dispenser can cause chain reactions, creating opportunities for more points.
3. Players must match the required ingredients of a potion to brew it successfully.
4. The game ends when all potions are brewed.

## Dependencies
- This game is written in C++ and requires a C++ compiler to run.
- Ensure you have the C++ compiler installed on your machine.
- Make sure you have a Linux operating system (preferably Ubuntu).

## How to Run
1. Download game.cpp file on your system.
2. Navigate to the folder where game.cpp is downloaded.
3. Right-click and "Open as terminal".
4. Run the following commands on your terminal:
    - g++ -o potion_explosion game.cpp -lrt
    - ./potion_explosion

## Example Gameplay
```
$ ./potion_explosion

Welcome to Potion Explosion!

Enter Player 1's name: Alice
Enter Player 2's name: Bob

Press Enter to start...

...
```

## Enjoy the Game!
Get ready to brew some magical potions and compete against your friends. Have fun playing Potion Explosion!
