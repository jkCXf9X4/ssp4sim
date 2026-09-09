



Evaluate and implement
Be cautious when delegating tasks that could affect the same files

----


Remove the recorder from the executor

its no longer needed

recorder should be injected to the models earlier

---


lib/include/simulation/graph_executor/graph_executor.cpp be a pure macro step executor

split into a macro and a realtime macro step executor


---

the read target resolver should be set during the pipeline and 

lets start with nailing down the design first before we patch anything
---

I have started to refactor the top layers of the simulation engine, can you help me propagate the changes downward and fill in the missing sections
The design docs might not be up to date, update these as well if needed during your fix

A few notes: 
- all executors should now take ownership of invocable passed to it, this should be changed for all executors
- The stepdata responsibilities have been altered and some have been broken out to the executor/resolver

Lets start small by patching all executors according to the macro time executors as references