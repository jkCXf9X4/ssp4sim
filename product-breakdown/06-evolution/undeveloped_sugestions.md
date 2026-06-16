

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


I can see that the current diff introduces a large number of naming functions and flags that should be unnecessary with the current setup

If we can set all default start values for all components first

then recursively apply parameter bindings for the different levels where the naming of the current level and sublevels are applied on a x.y naming schema

that would mean that parameter bindings on a component level should only be applied on a variable name level

a bottom level sub-system level should be applied on a component.variable

and a top level should only be applied on a component.variable or subsystem.component.variable

This would mean that all system, model, variable, connector would only need one name

implications for implementation

set all default variables for all levels and components

for each level we then build a map of the relevant variables and its name in the current context and apply the parameter binding to that map

help me review and reason around this solution and its implications
