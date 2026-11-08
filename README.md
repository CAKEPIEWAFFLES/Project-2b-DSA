# GatorBoxd

## Build and run

Use CMake and a C++17 compiler. From the project folder, check that `ml-1m` contains `movies.dat` and `ratings.dat`, then run:

```sh
cmake -S . -B build
cmake --build build --config Release
./build/gatorboxd ml-1m
```

If your build puts the executable in `build/Release`, use `./build/Release/gatorboxd ml-1m` from the project folder. On Windows, the executable is `gatorboxd.exe`.

You can also build with g++:

```sh
g++ -std=c++17 -O2 -DHAVE_COWATCH_INDEX cli.cpp core.cpp graph.cpp inverted_index.cpp -o gatorboxd
./gatorboxd ml-1m
```

Use menu option `3` to switch between Graph, Index, BruteForce, and Popularity. Use option `2` to get recommendations. Use option `5` to benchmark the methods.

To run the built-in checks, pass `--test` instead of `ml-1m`. For example, `./build/gatorboxd --test`.
