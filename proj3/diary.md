# Diary

### First attempt
Implemented analyze, backjump and new main loop: segfault on the first problem where unit propagation is not enough. Fixed segfault.
Problem: added the learned clause before backjumping -> fixed full2.cnf
now full3.cnf infinite loop

Adding vsids brought add128 time down from 91.31 to 0.98, but prime65537 time up from 1.78 to 33.50. Fix: I had a bug where I did not bump the activity of the uip literal. THis brought prime65537 time down to 0.36.

Minimization -> straight forward