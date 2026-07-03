

Investigate a new executor

it should analyze the DAG and establish dependency loops 
for example 
Models A, B, C, D

A->B
B->C
C->B
C->D

it should use a sequential method to run A->(group BC)->D
The group BC should, since its a loop and cant be executed sequentially be run in parallel with a smaller timestep.
Lets for the sake of testing utilize a timestep that is t/size of the loop to ensure that all messages have a chance to pass thrue

many different options for this is possible
- one large step
- many small steps
- depend it on the loop size
- one large to almost get to the end and then many small once to allow information to pass thru the chain ( Im having problems seeing how the dynamics might be affected by this )


lets test run it on the embrace system as a start but later utilize the modelica signal fmus to get a more predictable picture of how signals behave  
            