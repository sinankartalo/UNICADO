# Constraint Analysis

This module generates matching charts for jet and propeller-driven aircraft. It reads the selected case from `config/constraint_analysis_conf.xml`, evaluates the active constraints, selects a feasible design point, and exports the results as CSV files.

## Project context

This module was developed for the term project as an extension of the UNICADO sizing workflow. The UNICADO libraries and the aircraft, aerodynamic, engine, and propeller input data used in the study were provided by the supervisor. The constraint-analysis implementation, case setup, evaluation, and plots were developed within the project.

## Inputs

The analysis uses:

- aircraft and aerodynamic data
- atmosphere and mission data
- a jet-engine or propeller performance deck
- performance requirements and model settings from the XML configuration

## Build

CMake 3.20 or newer, a C++20 compiler, Eigen3, and CGAL 5.6 are required. From the `constraint_analysis` directory:

```bash
cmake -S . -B build
cmake --build build
```

## Run

```bash
./build/constraint_analysis_app [config.xml] [case_ID]
```

On Windows, use `build/constraint_analysis_app.exe`. With no arguments, the program uses `config/constraint_analysis_conf.xml` and the active case defined there.

Results are written to `output/<case_ID>/`. To create the plots:

```bash
python scripts/plot_results.py <case_ID>
```

## Notes

- Jet cases use thrust-to-weight ratio, `T/W`.
- Propeller cases use required shaft power-to-weight ratio, `P/W`, in W/N.
- The propeller matching chart calculates aircraft-level power requirements. Installed-system feasibility also depends on deck coverage, propeller count, geometry, and available shaft power.
- Operating points outside the aerodynamic or propulsion data range are reported instead of extrapolated.
