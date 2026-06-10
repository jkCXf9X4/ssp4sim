

I have started implementing IMP-038 but the initial architectural responsibility turned up to be too interconnected for efficient development

Instead i have a followup adjustment that insert a separate analysis graph layer in between the analysis system and simulation graph, much like it was before but with a separate pre layer for parsing the ssp

analysis system layer have remissibility for parsing and inferring setup directives from the ssp/ssd
- parsing the ssp
- setting up initial values according to parameter sets
- enable model to subsystem model connection
- tree/object hierarch
- only parse what is needed 
- the objects may be responsible for sub-object creation ( as they are designed now )

analysis graph layer is responsible for:
- set up graphs for SSC analysis
- algebraic loop identification
- graph/node based
- the objects may be responsible for sub-object creation ( as they are designed now )


Help me reason around this proposal and improve upon it

---

Enable external inputs for fault injection
- pointer injection in the variable copy step
- should enable direct altering
- should be able to set a future specific time for tight control


Server that act as a receiving partner that is internal in the application
- some xml format for specifying some predefined number 

this injects into the scheduling somehow...


---



void override_start_values(system)
{

    for subsystem in system.systems
    {
        override_start_values(subsystem)
    }

    for component in system.component
    {
        auto component_mappings = ext::ssp1::ssv::get_start_value_mappings(component);
        override component start values
    }
    auto system_mappings = ext::ssp1::ssv::get_start_value_mappings(system);
    override system start values
}

    


---

add test coverage in the c++ test suite for analyzing parameter bindings to get a faster feedback regarding degradation 
- ensure that nested usecases with system, sub-system and component bindings are covered

nested systems are not yet supported but mark them as xfail to enure that tracking is provided