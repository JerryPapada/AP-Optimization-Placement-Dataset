# Wi-Fi Access-Point Placement Dataset

Simulation and data-processing files for the project on optimizing access-point placement for indoor user localization.

## Contents

- `new_sim.cc`: ns-3 Wi-Fi/building simulation source.
- `add_building.py`, `clean.py`, and `fix_feed.py`: data transformation utilities.
- `processed_scenarios.csv` and `transformed_simulations.csv`: committed scenario/simulation artifacts.

The C++ source imports custom NYU propagation and channel-condition modules in addition to ns-3 modules.

## Requirements and reproducibility

A compatible ns-3 workspace with those custom modules is required. The Python utilities require the libraries they import, including pandas and JSON/CSV handling. There is no root requirements file or single runner.

Some scripts use absolute paths from the original development machine. Update those paths and inspect expected delimiters/input columns before running them. This README does not claim regenerated data or simulation results.
