# Planetary-Explorer

An interactive, menu-driven C++ console program for exploring our solar
system: the Sun, the eight planets, five well-known dwarf planets, a
selection of notable moons, and several famous comets.

Everything is self-contained in `main.cpp` using only the C++ standard
library — no internet connection, external files, or third-party
libraries required.

## Features

- Main menu to jump straight to the Sun, Planets, Dwarf Planets, Moons,
  or Comets
- Full detail pages including **distance from the Sun** (AU and km),
  **orbital period**, **planetary day (rotation period)**, diameter,
  surface gravity, moon counts, discovery info, and a list of
  interesting facts
- Drill down from any planet straight into its notable moons
- **Search** by name (partial matches supported)
- **Compare** any two bodies side-by-side
- **Random fact** generator pulled from the entire dataset
- Retrograde rotations (Venus, Uranus, Triton, etc.) are called out
  explicitly, and tidally-locked moons are flagged automatically

## Build

Requires a C++17 compiler (e.g. g++ or clang++):

```sh
g++ -std=c++17 -O2 -Wall -o planetary_explorer main.cpp
```

## Run

```sh
./planetary_explorer
```

Then follow the on-screen menu — enter a number to select an option,
and `0` to go back or exit at any menu.

## Notes on the data

- Distances and periods are long-term averages; real orbits are
  elliptical, so actual values vary somewhat over time.
- Only a representative selection of moons is included per planet;
  each planet's page shows its total known-moon count separately
  (a number that keeps growing as new moons are discovered).
- A negative rotation period denotes retrograde rotation — the body
  spins opposite to the direction it orbits.
