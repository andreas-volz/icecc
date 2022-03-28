Iscript Examples
----------------

This directory includes some examples of several scripts to 
demonstrate a tiny bit of what iscript editing can do. The .txt 
files are the actual editable scripts and must be compiled
into an iscript.bin file and included in a custom mpq archive
to use them. The scripts use the old opcode and animation names
so classic_icecc.exe must be used to compile them.

Bumpin SCVs: This is the script in the manual's tutorial. It 
makes SCVs bounce.

Fast Ghost: Very simple hack to make the ghost move a little
faster (or rather, take larger steps).

Marine Storm: Another simple hack to make the marine attack with
Psi Storm in addition to his regular gauss rifle.

Dancing Firebat: A little more complicated script that makes the
firebat occasionally break out a dance when idle. This script
really demonstrates how you can use random jumps to create
random behavior.


One other simple hack that I did before was to take a movie,
extract all the frames from it and put them sequentially into
a GRP file. Then use an iscript animation to play the GRP back
sequentially in the game -- voila, instant in-game-movie. :)
That hack isn't here because movie GRPs tend to be big.


(Examples by Jeff Pang, readme modified by ShadowFlare because of
the compiled scripts not being included here)