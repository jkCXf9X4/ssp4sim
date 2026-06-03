

Lets clarify and streamline the responsibilities of the analysis graph vs the graph

The analysis graph is responsible to parse any information that is relevant from the ssp and store it in local nodes
- The graph is closer to the actual ssp and is used as analysis
- nodes exist for model, connectors, connections (to store delay information), internal variables ( to find algebraic loops )

Nested system implication:
 - look at resources/reference_ssp/models/ssp/dcmotor/ssp/SystemStructure.ssd
 - nested systems will add multiple connections between models

Practical implications:
- The graph includes: connectors, connections (to store delay information), internal variables
- The connectors point to the model they relate to: connector.model
- avoid model-to-model graph in this stage


The simulation graph when sets up the models, the storage areas and store any necessary information in local containers or self from the analysis graph 
- Creates the final connection graph: model-to-model from analyzing the analysis graph


The graph builders are responsible for creating and connecting up the graph
The graph objects are responsible for the analysis of itself


This will require some large restructuring of the existing codebase 
The gains will be:
- the application can parse nested systems
- clear separation of concern, well documented in decisions and architecture

