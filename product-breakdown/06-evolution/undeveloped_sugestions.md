

Lets clarify and streamline the responsibilities of the analysis graph vs the graph

The graph builders are responsible for creating and connecting up the graph
The graph objects are responsible for the analysis of itself

The analysis graph is responsible to parse any information that is relevant from the ssp and store it in local nodes
- The graph is closer to the actual ssp and is used as analysis basis
- nodes exist for model, connectors, connections (to store delay information), internal variables ( to find algebraic loops )

Nested system implication:
 - look at resources/reference_ssp/models/ssp/dcmotor/ssp/SystemStructure.ssd
 - nested systems might add multiple connections between models if they pass a sub-system boundary

The simulation graph builder sets up the models, the storage areas and store any necessary information in local containers or self from the analysis graph 


Expectations:

A nested system should produce an analysis graph that is flattened

Analysis graph:
- The analysis graph includes: connectors, connections (to store delay information), internal variables
- The connectors point to the model they relate to: connector.model
- avoid model-to-model graph in the analysis builder stage

The analysis graph is a pure graph with connectors ( that may reference model Nodes ) , connections (to store delay information), internal variables ( to find algebraic loops ) nodes

Graph analysis is moved into the Analysis graph and simulation graph objects 

in the analysis graph:
- The fmu model graph is derived from the analysis graph
- algebraic loops are derived from the analysis graph

The simulation graph is only models based

This will require some large restructuring of the existing codebase 


The gains will be:
- the application can parse nested systems
- clear separation of concern, well documented in decisions and architecture



---

Enable external inputs for fault injection
- pointer injection in the variable copy step
- should enable direct altering
- should be able to set a future specific time for tight control


Server that act as a receiving partner that is internal in the application
- some xml format for specifying some predefined number 

this injects into the scheduling somehow...


---

Add use-cases for allowing a user to inject variable changes on ether output or input ,depending on propagation characteristics. 
It should enable direct effect or to set a future specific time when it should be applied for tighter control
The injection should be able to:
1. do a direct override
2. inject a phase shift
3. inject a kx+m signal change

The method and requirements regarding how the interface to the application/simulation engine is not yet set. c