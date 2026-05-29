


--- 
1
implement the internal node connections in the analysis graph

---
2
Evaluate and implement how to store information regarding variable internal feedthru in the sim graph

---
3

Iterate over algebraic loops during initialization, requires 1 and 2

---
4

New executor Seidel-Jacobi:
 - Executor for getting the best of seidel and jacobi 
 - evaluate loops with tarjan SCC
 - run weakly connected with seidel and strongly with jacobi
 - use same timestep initially


--- 
5
New executor:

- Direct rerun Gauss-Seidel
 - start the models as fast as possible, restart the DAG before the previous finishes

---
6
create a config item for including input storage to the output artifacts, csv or duckdb

---
7 
Restructure the thread pools into separate subfolder with information on usage and tradeoffs for the individual solutions