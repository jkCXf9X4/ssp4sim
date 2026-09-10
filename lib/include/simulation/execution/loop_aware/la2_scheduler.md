

Scheduler should

1. resolve strongly connected components
2. setup sequential dag with strongly connected components as single nodes
3. run sequential dag using sequential methods such as Gasss-Seidel
- the step should be the configured macro step

- run the scc parts of the dag in parallel as a part of the sequential dag

Example graph:
A -> B
B -> C
C -> B
C -> D

1. A
2. B and C in parallel
3. D

The parallel part should be able to execute in two different modes
1. linear - each substep is pre-selected before execution, the substeps should run untill the full macro step is passed
2. iterative/factor - the substep is decided by a factor of what time is left to the current macro step. a factor of 0.5 would first take a stet of macro step/2 and then a step of 1/4 and then 1/8 until the output values stabilizes


Access rules

sequential models/ssc should always read latest
data from parallel executed nodes should use start time to ensure deterministic results

So B should get latest from A but start time from C 
- C and B will take multiple steps before moving on to D
- It should always get the latest from A but the start time from C will be according to sub-step start time