# Propeller equation basis and code traceability

This note documents the equations used by the propeller constraint-analysis path.
It distinguishes published equations, algebraic rearrangements, supplied propeller
map data, and project assumptions.

## Primary reference

J. D. Mattingly, W. H. Heiser, and D. T. Pratt, *Aircraft Engine Design*,
2nd ed., AIAA Education Series, 2002, ISBN 1-56347-538-3.

Relevant locations:

- Appendix K, p. 591, Eq. (K.2): propeller shaft power, efficiency, thrust,
  and flight-speed relation.
- Appendix L, pp. 609-621: propeller momentum theory and real-propeller
  behavior.
- Appendix L, p. 617, Eqs. (L.17)-(L.18): real-propeller thrust and power
  correlation coefficients.
- Appendix L, p. 618, Eq. (L.20): advance ratio and propeller-map use.
- Appendix L, pp. 615-616: blade relative Mach-number discussion.
- Appendix L, pp. 620-621: constant-speed and variable-pitch operation.

The standard nondimensional coefficient convention is independently summarized
by the University of Illinois Applied Aerodynamics Group, UIUC Propeller Data
Site: https://m-selig.ae.illinois.edu/props/propDB.html

## Symbols and units

| Symbol | Meaning | SI unit |
|---|---|---|
| `V` | true airspeed at the propeller | m/s |
| `n` | propeller rotational speed | rev/s |
| `RPM` | propeller rotational speed | rev/min |
| `D` | propeller diameter | m |
| `rho` | air density | kg/m^3 |
| `J` | advance ratio | dimensionless |
| `C_T` | thrust coefficient | dimensionless |
| `C_P` | power coefficient | dimensionless |
| `eta` | propeller efficiency | dimensionless |
| `T` | propeller thrust | N |
| `P` | propeller shaft power | W |
| `a` | local speed of sound | m/s |

## 1. Rotational speed conversion

```text
n = RPM / 60
```

This is a unit conversion from revolutions per minute to revolutions per second.
It is implemented in `propeller_constraint_analysis::evaluate`.

## 2. Advance ratio

```text
J = V / (n D)
```

Source: Mattingly et al., Appendix L, Eq. (L.20), p. 618.

The code calculates `J` from speed, RPM, and diameter. The pair
`(pitch, J)` identifies a constant-pitch slice and an operating point in the
supplied propeller deck. Linear interpolation then returns `C_T`, `C_P`, and
`eta`. The coefficients are not invented or tuned by the constraint tool.

Code path:

```text
propeller_constraint_analysis::evaluate
  -> interpolate_propeller_pitch_slice
  -> deck values C_T, C_P, eta
```

## 3. Thrust coefficient and thrust

The standard SI nondimensional definition is

```text
C_T = T / (rho n^2 D^4)
```

and its algebraic rearrangement used in the code is

```text
T = C_T rho n^2 D^4 .
```

Source basis: Mattingly et al., Appendix L, Eq. (L.17), p. 617, gives the
real-propeller thrust correlation with the same `rho n^2 D^4` scaling, expressed
using the book's customary-unit normalization. The code uses the standard SI
dimensionless convention used by the supplied deck and by the UIUC Propeller
Data Site.

Dimensional check:

```text
(kg/m^3) (1/s^2) (m^4) = kg m/s^2 = N
```

## 4. Power coefficient and shaft power

The standard SI nondimensional definition is

```text
C_P = P / (rho n^3 D^5)
```

and its algebraic rearrangement used in the code is

```text
P = C_P rho n^3 D^5 .
```

Source basis: Mattingly et al., Appendix L, Eq. (L.18), p. 617, gives the
real-propeller power correlation with the same `rho n^3 D^5` scaling, expressed
using the book's customary-unit normalization. The code uses SI units.

Dimensional check:

```text
(kg/m^3) (1/s^3) (m^5) = kg m^2/s^3 = W
```

## 5. Propeller efficiency

Mattingly Appendix K, p. 591, states the propeller power balance as

```text
eta P = T V .
```

Therefore,

```text
eta = T V / P .
```

Substituting the definitions of `J`, `C_T`, and `C_P` gives

```text
eta = J C_T / C_P .
```

The code reads `eta` from the supplied deck rather than recomputing it. A direct
check of all 72 deck rows against `eta = J C_T / C_P` produced a maximum absolute
difference of approximately `1.41e-8`. Thus the deck columns are internally
consistent to numerical precision.

At zero advance ratio, useful propulsive power `T V` is zero, so `eta = 0`
even when static thrust and shaft power are nonzero.

## 6. Conversion from thrust loading to shaft-power loading

The airborne Mattingly constraint calculation first returns required thrust loading
`T/W`. The propeller analysis converts this to shaft-power loading using

```text
P/W = (T/W) (P/T) .
```

This is an exact algebraic identity. Using the coefficient equations,

```text
P/T = n D (C_P/C_T)
```

and therefore

```text
P/W = (T/W) n D (C_P/C_T) .
```

The implementation uses the equivalent evaluated quantities

```text
P/W = (T/W) (shaft_power_W / thrust_N)
```

so `C_T` and `C_P` come from the same interpolated deck operating point.
Equivalently, where `V > 0`, the Mattingly power balance gives
`P/W = (T/W) V/eta`.

## 7. Helical blade-tip Mach check

The current code uses the kinematic approximation

```text
V_tip,rot = pi D n
V_tip,helical = sqrt(V^2 + V_tip,rot^2)
M_tip = V_tip,helical / a .
```

The axial and rotational velocity components are perpendicular, hence the
root-sum-square helical speed. Mattingly Appendix L, pp. 609 and 615-616,
explains that propeller blade relative Mach number exceeds flight Mach and that
near-sonic tip operation causes shock and drag losses.

Model limitation: this project check neglects induced axial velocity, swirl,
installation inflow distortion, and blade-section effects. It is a screening check,
not a detailed blade-element or CFD calculation. The configured limit
`M_tip <= 0.95` is a supervisor/project requirement, not a universal limit quoted
from Mattingly.

## 8. Pitch handling

The propeller deck contains constant-pitch slices at 15, 30, and 45 degrees.

- During integrated takeoff, the code evaluates every pitch slice whose `J`
  domain contains the current speed and selects the lowest `P/T` operating
  point. There is no manual takeoff-pitch input.
- For cruise, climb, turn, and acceleration, the configured continuous pitch
  selects the deck slice and `J` selects the interpolated position within it.

This is consistent with Mattingly Appendix L, pp. 620-621: a governor can vary
blade pitch to match engine load at constant RPM, while changes in speed move
the operating point through advance ratio.

## 9. Takeoff integration

Propeller takeoff is not obtained by inserting a constant efficiency into a closed
formula. For every ground-roll speed step, the implementation:

1. calculates stall and takeoff speed from `W/S`, density, and `C_L,max`;
2. calculates `J`;
3. evaluates valid pitch slices from the supplied deck;
4. obtains `T` and `P`;
5. converts trial installed `P/W` to available `T/W` through deck `P/T`;
6. integrates acceleration to ground-roll distance;
7. uses bisection to find the `P/W` that meets the runway requirement.

This preserves the speed dependence of the supplied propeller map.

## 10. Assumptions that must remain visible

- The supplied `prop.csv` is treated as authoritative analysis data.
- Interpolation is allowed only within an existing constant-pitch `J` range;
  extrapolation is rejected.
- Diameter and RPM are prescribed configuration inputs, not optimized variables.
- Continuous pitch is prescribed; takeoff pitch is selected from available deck
  slices.
- The same deck is used at all altitudes; density scaling is applied through the
  coefficient equations.
- Compressibility corrections beyond the helical tip-Mach feasibility check are
  not modeled.
- Installation losses, gearbox efficiency, and engine-power lapse are not yet
  included in the propeller-map conversion.
