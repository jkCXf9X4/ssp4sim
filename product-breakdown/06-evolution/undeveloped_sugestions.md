

Help me develop this idea into an improvement candidate:
Lets clarify and streamline the responsibilities of the analysis graph vs the graph

the analysis graph should move to an analysis system

The builders are responsible for creating the objects and and connecting up the graph
The objects themselves are responsible for the analysis of itself

The analysis system is responsible to parse any information that is relevant from the ssp and store it in local nodes
- The analysis system is closer to the actual ssp and is used as analysis basis
- its not a graph in itself but stores the analysis object used to set up the graph correctly


The analysis system will be more of a internal representation of the ssp with:

analysis system
 - external analysis connectors
 - optional nested system
 - analysis models
  - internal analysis variables ( to find algebraic loops )
  - variables dependencies ( to find algebraic loops )
 - connections both model-to-model and model-system_boundary


This will require some large restructuring of the existing codebase 


The gains will be:
- the application can parse nested systems
- clear separation of concern, well documented in decisions and architecture
- the analysis system can analyze and hide the complexity of the ssp when creating the simulation graph



Expectations:

The simulation graph builder sets up the models, the storage areas and store any necessary information in local containers or self from the analysis graph 
A nested system should produce an analysis graph that is flattened

Graph analysis is moved into the Analysis graph and simulation graph objects 

in the analysis graph:
- The fmu model graph is derived from the analysis graph
- algebraic loops are derived from the analysis graph

The simulation graph is only models based




---

Enable external inputs for fault injection
- pointer injection in the variable copy step
- should enable direct altering
- should be able to set a future specific time for tight control


Server that act as a receiving partner that is internal in the application
- some xml format for specifying some predefined number 

this injects into the scheduling somehow...


---

1. Move the find_connector to the AnalysisSystem
2. in analysis model:
    // TODO: The feedthrough can pass thru many internal variables before entering out on the other side
    // It can also ba a 1 to many mapping between in and out

3. add som way to print the analysis system for visual inspection by the user