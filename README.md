# Hi-Lo Multi-Threaded game implemented with threads, mutexes, and condition variables
# There are three threads, thread 1 and thread 2 act as the players while thread three acts as the referee
# The referee generates a random number to be the target of the game which is from 1 to 100
# Player 1's strategy is to take the average of its max and min bounds which are originally the bounds of the target guess
# Player 2's strategy is to just guess a random number between its max and min bounds
# Each player's guess was stored in a global array
# After each guess the referee informs the players whether their guesses were high, low, or correct and they adjust their bounds accordingly
# This feedback is done through a global array, along with mutexes and condition variables
# A total of 10 games is played and the player with the most wins is declared the winner
