
Increase testabillity of build_simulation_graph

The recorder is only used in GraphBuilder.
If we break out the recorder this unit will be easier to test

Evaluate how the SharedConfig should be adapted to create a default if the file is not available to enable easier testing as well



            