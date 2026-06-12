

Enable external inputs for fault injection
- pointer injection in the variable copy step
- should enable direct altering
- should be able to set a future specific time for tight control


Server that act as a receiving partner that is internal in the application
- some xml format for specifying some predefined number 

this injects into the scheduling somehow...


---

add test cases in the python suite to verify data of the simulation to check that connections and data propagation between models works as expected 


---


Lets continue and simplify the analysis system, analysis graph and simulation graph workflow

Reflecting on the solution, it is a bit nested and the separation is not as clean as visioned

If we can make the distinction between the parts cleaner

Create an improvement from evaluating this architecture change:

Analysis system, model, connector, connection, and internal variables are graph nodes inheriting from utils::graph::Node

Analysis system builder creates a hierarchical tree, just as it does presently 

Connectors should only be present where they are applicable, system.connectors should only contain system boundary connectors and model.connectors should only contain model connectors for easier matching

Analysis graph builder (factory) do a text based matching and connects the nodes into multiple analysis graphs

separate graphs for:
model to model graph for simulation model execution order
connector, connection, internal variable graph for finding the algebraic loops

Simulation graph builder takes the analysis objects with tree and graphs and constructs simulation model objects and final execution graph

Lets also clean up AD-003, 004 and 005 -> remove them and incorporate a summary of the information of previous solutions in a new full covering decision 