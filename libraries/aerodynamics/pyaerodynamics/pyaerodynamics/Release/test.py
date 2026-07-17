import pyaerodynamics as aero
import csv
from pathlib import Path

def test_aircraft_initialization_non_linearized():
    file = "polar.xml"   # adjust path — see note below

    settings = aero.TrimSettings()
    settings.method = "non_linearized"

    aircraft = aero.Aircraft(file, settings)

    conditions = {
        "Mach": 0.667,
        "h": 8000.0,
        "alpha": 3.76,
    }

    main_wing = aircraft.components["main_wing"]

    CL_wing = main_wing.get_property("CL", conditions)
    print(f"main wing CL = {CL_wing}")

    cmac_wing = main_wing.chord
    print(f"main wing chord = {cmac_wing}")
    
    CM_wing = main_wing.get_property("CM", conditions)
    print(f"main wing CM = {CM_wing}")

    CD_wing = main_wing.get_property("CD", conditions)
    print(f"main wing CD = {CD_wing}")
    
    TN_deck = "V2527-A5_FN.csv"

    BLOCK_SIZE = 29

    mach_vec = []
    altitude_vec = []
    thrust_setting_vec = []
    thrust_value_vec = []  # list of dicts, one per row -> types::PropertyType

    mach_numbers = []
    thrust_setting = 0.0
    data_row_count = 0

    with open(TN_deck, newline="") as f:
        reader = csv.reader(f, delimiter=";")
        for tokens in reader:
            if not tokens:
                continue
            # drop trailing empty tokens from stray delimiters, mirror the C++ tokenizing
            tokens = [t for t in tokens if t != ""] if len(tokens) and tokens[-1] == "" else tokens
            if len(tokens) < 2:
                continue

            is_header_row = (data_row_count % (BLOCK_SIZE + 1) == 0)

            if is_header_row:
                # col[0] = thrust setting, col[1..] = Mach numbers
                thrust_setting = float(tokens[0])
                mach_numbers = [float(t) for t in tokens[1:]]
            else:
                # col[0] = altitude, col[1..] = thrust values
                altitude = float(tokens[0])
                for i in range(1, len(tokens)):
                    if (i - 1) >= len(mach_numbers):
                        break
                    mach_vec.append(mach_numbers[i - 1])
                    altitude_vec.append(altitude)
                    thrust_setting_vec.append(thrust_setting)
                    thrust_value_vec.append({"TN": float(tokens[i])})

            data_row_count += 1

    # --- Build interpolation ---
    lhs = {
        "mach": mach_vec,
        "altitude": altitude_vec,
        "setting": thrust_setting_vec,
    }
    variables = ["mach", "altitude", "setting"]

    engine = aero.Interpolation(lhs, thrust_value_vec, variables)

    conditions = {
        "mach": 0.5,
        "altitude": 10000.0,
        "setting": 0.7,
    }

    T = engine("TN", conditions)
    print(f"Thrust (TN) = {T}")


if __name__ == "__main__":
    test_aircraft_initialization_non_linearized()