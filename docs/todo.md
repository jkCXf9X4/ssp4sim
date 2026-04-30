##
Evaluate https://vcpkg.io/en/package/sqlitecpp for data persistence

use in memory cache and push to sqllite for persistence


Replacing busy-polling plus flag-scanning with event-driven handoff
    When FmuModel::pre or post publish a small snapshot descriptor or packed sample into a bounded queue.

    async: simulation never waits.
    step_snapshot: simulation waits only until a snapshot block is enqueued.

    How to avoid copying the data?


  - Separate capture from serialization:
    Recorder thread 1 collects snapshots into blocks.
    Writer thread 2 serializes blocks to CSV.


  - do not require per-step disk durability; only complete final results are required.
  - CSV remains the user-facing artifact, but internal staging may be binary or block-oriented.


Possible configuration items

    simulation.recording.consistency = async|step_snapshot
    simulation.recording.capture = outputs|inputs_and_outputs
    simulation.recording.queue_capacity = <int> bounded, default sized for a few seconds of samples


---


Analyze internal connections, this can be used for:


in Jacobi
- The data should be set in a specific order if there are direct feed thru inside the FMUs

- In these cases the set_real has secondary effects and the outputs should be set directly

- This isn't really pure Jacobi but some kind of hybrid


Move the set inputs / retrieve outputs outside the fmu_model to enable this


---

Check out 

- https://bauklimatik-dresden.de/mastersim/help/MasterSim_manual_en.html

1.9.3. Newton

iteration loop:
  in first iteration, compute Newton matrix via difference-quotient approximation

  loop over all slaves:
    set all input values
    tell slave to take a step

  loop over all slaves:
    retrieve all output values

  solve equation system
  compute modifications of variables

  perform convergence check

Can this be used for co-simulation?


---

std::byte *SignalStorage::get_item(std::size_t area, std::size_t index) noexcept

is slow, create a flat vector to limit the number of pointer directions
