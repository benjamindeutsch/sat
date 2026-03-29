# Diary

### First attempt
My first attempt was implementing the most simple solution where the decision heuristics was to simply select the first unassigned variable.

After implementing the TODOs I tested my implementation on the very small test cases (true.cnf and false.cnf), where everything went fine. I then tried a bigger example and got a segmentation fault. After a bit of debugging I found that I had a bug in the propagate() function, where I returned after the first propagated literal. Then I found another bug in propagate(): I did not use assign() to assign a literal but just pushed the propagated literal to the stack.

After fixing these bugs, the solution worked. Overall the project was not too challenging to implement, since all the steps of the functions to be implemented were given in the comments.

### Adding (slightly) more sophisticated heuristics
I changed the heuristics to pick the unassigned variable that occurs most often in the original formula.

### Adding VSIDS heuristics
Inspired by Minisat, I implemented VSIDS. For that, I added an activities array storing the activity score of each variable. The activity score of a variable is increased if it occurs in a conflict clause. To favour variables that occured in recent conflicts, the value by which the activity scores of variables are increased, is increased periodically.

This heuristics sped up my implementation significantly.