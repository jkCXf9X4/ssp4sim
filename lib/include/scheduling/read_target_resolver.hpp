#pragma once

#include "invocable.hpp"
#include "model_fmu.hpp"

#include "signal/storage.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

// Read-target resolver — see breakdown/03-implementation/modules/data_access_read_target_resolver_v2.md
// Streamlined initial implementation:
//   - constructed against the graph models only (std::vector<Invocable*>)
//   - one read function that returns "the index or the viable time" depending on mode
//   - access policy (mode/delay/offset/clamp/unlinked/committed gate) concealed inside
// Determinism contract (design docs): single committed-watermark truth (M1b), reads clamped
// to the committed frontier (M1a/D2/D7), unlinked reads stale-only (UC-14), mark_committed
// is the sole writer with release-store ordering after value bytes are visible (D17).
//
// This header is self-contained: it does NOT drag in the internal resolution core
// (read_target_core.hpp). The core depends on this header's public types
// (ResolvedRead / ResolverConfig) and is linked into the resolver's .cpp only.
namespace ssp4sim::scheduling
{
    namespace detail
    {
        /// The pluggable read-target resolution policy. Fully defined in
        /// read_target_core.hpp; this header only forward-declares it because the shell
        /// needs nothing but the pointer (it stores and forwards the chosen resolver's
        /// mark_committed / resolve without dereferencing it).
        class ReadResolver;
    }

    /// What one connection may read this frame. Exactly one of `area` / `time` is
    /// meaningful, selected by `is_area`:
    ///   is_area == true  -> `area`   is the storage slot to read (no search at all)
    ///   is_area == false -> `time`   is the viable, frontier-clamped reference (search <= it)
    /// `valid == false` means "producer not committed yet" — caller keeps init / skips.
    struct ResolvedRead
    {
        bool valid = false;
        bool is_area = false;
        std::size_t area = 0;
        std::uint64_t time = 0;
        std::uint64_t generation = 0;
    };

    /// Behaviour knobs. Defaults implement the documented determinism contract.
    struct ResolverConfig
    {
        bool clamp_stale_shortfall = true;        // M1a: clamp reference to committed frontier
        bool check_staleness_signature = true;    // M1c: keep generation on every read
        std::int32_t max_lookback = 0;            // D8: 0 = derived from storages (not enforced here yet)
    };

    /// Resolves, per connection, "what to read" for a model at schedule runtime, returning
    /// either a pinned area index (Index mode / unlinked stale read) or a viable,
    /// frontier-clamped time. Construct once against the graph models; the executor calls
    /// mark_committed(); the read path calls resolve(). Both are forwarded to the injected
    /// `detail::ReadResolver` (see below), so the access policy is pluggable.
    class ReadTargetResolver
    {
    public:
        /// Walks the models once: registers one ModelStatus per model, maps each
        /// output_area storage to its owning model, and snapshots each model's connections
        /// as edges (index-aligned with model->connections). Non-FmuModel invocables are
        /// skipped.
        ///
        /// `resolver` selects the resolution policy. nullptr selects the default
        /// (detail::macro_step_start_time_resolver()); it is borrowed, not owned — the
        /// shared stateless resolver instances from read_target_core.hpp are the canonical
        /// choices.
        ReadTargetResolver(std::vector<ssp4sim::graph::Invocable *> models,
                           detail::ReadResolver *resolver = nullptr,
                           ResolverConfig cfg = {});

        ~ReadTargetResolver();

        /// Advance a producer's committed frontier. Call ONLY after the producer's output
        /// bytes are fully written and visible (D17 release-store). No-op for producers the
        /// resolver has not registered.
        void mark_committed(ssp4sim::graph::Invocable *producer,
                            std::uint64_t output_time,
                            std::size_t area);

        /// The one read function: for target `model`, incoming connection `connection_idx`,
        /// returns the index to read OR the viable time to search, per mode. Invalid if the
        /// model/connection is unknown or the producer has not committed yet.
        /// No `input_time` is needed: the Latest mode resolves to the latest committed index.
        ResolvedRead resolve(ssp4sim::graph::Invocable *model,
                             std::size_t connection_idx,
                             std::uint64_t step_start,
                             std::uint64_t step_end);

        /// Drives the whole read path for one target model: for each of its incoming
        /// connections, resolves the read target and copies the value (and forwarded
        /// derivatives) from the producer storage into the model's input storage area.
        /// Replaces the retired ConnectionInfo::retrieve_model_inputs; the resolution
        /// policy comes from this resolver's per-model edges (index-aligned with
        /// `connections`).
        void copy_model_inputs(ssp4sim::graph::FmuModel *target,
                               std::size_t target_area,
                               std::uint64_t step_start,
                               std::uint64_t step_end);

    private:
        // Opaque implementation state; all resolver-private tables live in the .cpp.
        // Declared here (incomplete) so the public class exposes zero private layout;
        // defined (completed) inside read_target_resolver.cpp, where it is the ONLY state.
        struct State;
        State *s_;
    };
}