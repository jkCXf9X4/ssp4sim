// Shared helpers for locating runnable SSP fixtures under
// resources/reference_ssp/artifacts/models.
//
// The artifact layout nests the runnable SSP under
//   artifacts/models/<model>/<analysis>/<build>/
// where <build> is the directory that directly contains SystemStructure.ssd.
// Analysis and build names vary per fixture (e.g. "baseline"/"model",
// "test_baseline"/"model", "fast"/"model"), so locating the runnable SSP root
// is a small directory scan rather than a fixed path. This header is the single
// source of that scan so tests do not duplicate it.
#pragma once

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace ssp4sim::tests::reference_ssp {

    /// Project root as baked in by SSP4SIM_PROJECT_ROOT.
    inline fs::path project_root()
    {
        return fs::path(SSP4SIM_PROJECT_ROOT);
    }

    /// Locate the runnable SSP root for a fixture: the <analysis>/<build>
    /// directory that directly contains SystemStructure.ssd under
    /// artifacts/models/<fixture>. Returns the fixture model directory when no
    /// runnable SSP is found.
    inline fs::path fixture_root(const std::string &fixture_name)
    {
        const auto fixture_dir = project_root() / "resources" / "reference_ssp" /
                                 "artifacts" / "models" / fixture_name;
        for (const auto &analysis : fs::directory_iterator(fixture_dir))
        {
            if (!analysis.is_directory())
                continue;
            for (const auto &build : fs::directory_iterator(analysis.path()))
            {
                if (build.is_directory() && fs::exists(build.path() / "SystemStructure.ssd"))
                    return build.path();
            }
        }
        return fixture_dir;
    }

    /// A resource directory under the fixture's runnable SSP root, e.g.
    /// resources/<name>. The resource may be either a directory (a named FMU
    /// resource) or a file; existence is not asserted here.
    inline fs::path fixture_resource(const std::string &fixture_name, const std::string &resource)
    {
        return fixture_root(fixture_name) / "resources" / resource;
    }

    /// First resource directory that contains a modelDescription.xml (an FMU
    /// body), under the fixture's runnable SSP root. Returns the resources dir
    /// when no FMU body is found.
    inline fs::path fixture_fmu_dir(const std::string &fixture_name)
    {
        const auto resources = fixture_root(fixture_name) / "resources";
        for (const auto &entry : fs::directory_iterator(resources))
        {
            if (entry.is_directory() &&
                fs::exists(entry.path() / "modelDescription.xml"))
            {
                return entry.path();
            }
        }
        return resources;
    }

} // namespace ssp4sim::tests::reference_ssp