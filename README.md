# roq-algo

API for algorithmic and high-frequency trading (HFT).

This project contains

* Interfaces for implementing algorithmic trading strategies
* Interfaces for implementing exchange matching simulation
* Interfaces for simulation reporting
* Reference implementations

## Design

![Design](/static/images/simulator.png)

## Prerequisites

> Use `stable` for (the approx. monthly) release build.
> Use `unstable` for the more regularly updated development builds.

### Initialize sub-modules

```bash
git submodule update --init --recursive
```

### Create development environment

```bash
scripts/create_conda_env unstable debug
```

### Activate environment

```bash
source opt/conda/bin/activate dev
```

## Build the project

> Sometimes you may have to delete CMakeCache.txt if CMake has already cached an incorrect configuration.

```bash
cmake . && make -j4
```

## License

The project is released under the terms of the BSD-3 license.
