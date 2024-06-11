A template project for creating your own strateiges.

The project includes

* Bare-minimum implementation needed to support strategy development

  * Supports live and simulation.

* A static library (named `tools`)

  * Allows you to build testable logic separate from the actual strategy.

* Test target

  * Using Catch2

* Benchmark target

  * Using Google benchmark


## Prerequisites

> Use `stable` for (the approx. monthly) release build.
> Use `unstable` for the more regularly updated development builds.

### Initialize sub-modules

```bash
git submodule update --init --recursive
```

### Create environment (Mambaforge)

```bash
scripts/create_conda_env.sh unstable debug
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

## Building your own conda package

```bash
scripts/build_conda_package.sh stable
```
