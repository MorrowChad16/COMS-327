I added fire sources. 1-5 randomly spawn and show the surrounding area as a hint into whats there, so you can decide to move there
We assume the PC has an initial torch that gives it the 5x5 radius of viewing range, so thats why we can light things on fire

I got rid of the multi-key possibilites for PC. down, left, right and up only use the num keys for to open up the commands for other actions
Got rid of diagonal movement to free key's for other commands

Debris can be lit on fire with 'l' when on top of '`'

Added text for explanation of the game when entering it

Added a shooting command by pressing 's'. it expands your visible range by 1 to view monsters around you. You can move around, 
similar to teleport, monster look up, etc. Click 's' to shoot that monster with your ranged weapon. Esc brings you back to the game.
Escaping does cost you a move because in reality you lost time bringing up your bow and trying to shoot the monster

Added a dodge ability. An int variable was added to the Character class. Instantiated with 0 and incremented with equiped weapons.
A random number 1-1000 and if it's greater than the characters dodge number then the attack hits. Attacks hit majority of the time

In the base game we assume the PC has a torch of a 5x5 radius. 

Added traps. You pick them up just like regular objects. You can't equip the item, but when you drop it you equip it.
It's represented by a 'T'. It's White when it is not set and Red when it is set. If you or a monster runs over it it takes off damage to the
HP and eliminates the piece.

The command 'v' doesn't do much right now, but it's a redesigned inventory layout. It'll eventually takeover 'i' and 'e' but right now its just 
there as a gimmick. You can click space to enter each area

Game is incredibly easy to win. Needs balancing with mosnter damage and item damage because monsters aren't the smartest things