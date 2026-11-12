# Wi-Fi Access-Point Placement and Localization Dataset

Simulation and data-processing material for Wi-Fi access-point placement and indoor localization. An ns-3 C++ scenario generates simulation data; Python utilities transform and clean CSV artifacts used by the project.

## Files and data flow

- new_sim.cc — ns-3 building/Wi-Fi simulation source.
- add_building.py — building/scenario data preparation helper.
- clean.py and fix_feed.py — CSV cleanup/transformation utilities.
- processed_scenarios.csv and transformed_simulations.csv — committed scenario/simulation artifacts.

The C++ source imports custom NYU propagation and channel-condition modules in addition to ns-3 modules. Those modules and the matching ns-3 workspace are external prerequisites.

## Requirements

- A compatible ns-3 workspace
- The custom propagation/channel-condition modules referenced by new_sim.cc
- Python 3 and the libraries imported by the data utilities, including pandas where used
- Input CSVs with the columns and delimiters expected by each script

There is no top-level requirements file or unified runner. Some Python scripts use absolute paths from the original development machine; update them for the local checkout before execution.

## Reproduction workflow

1. Build the simulation source in the external ns-3 tree with its custom modules.
2. Record the scenario parameters and save generated output CSVs.
3. Check each processing script's expected input path and column schema.
4. Run the relevant transformation/cleanup scripts and keep raw and processed data separate.

The committed CSVs are project artifacts, but the repository does not document their complete provenance or guarantee a one-command regeneration flow.