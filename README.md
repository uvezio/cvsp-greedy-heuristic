# A Configurable Greedy Heuristic for the Capacitated Vertex Separator Problem

This repository contains a **C++** implementation of a configurable **greedy heuristic** for the **Capacitated Vertex Separator Problem (CVSP)** – a combinatorial optimisation problem on graphs with applications to complex networks – developed as part of [my Bachelor's thesis in Management Engineering (2025)](#ref-0).

The proposed algorithm constructs a feasible solution through iterative vertex removal followed by a bin-packing feasibility check, and provides multiple configurations based on different vertex-selection rules. Randomised tie-breaking allows repeated executions to explore different feasible solutions.

The implementation was evaluated on benchmark instances from the literature, comparing the different heuristic configurations with reference results reported for an exact branch-and-cut method.

## Table of Contents

- [Problem Definition](#problem-definition)
  - [Illustrative Example](#illustrative-example)
  - [Applications to Complex-Network Protection](#applications-to-complex-network-protection)
  - [Further Reading](#further-reading)
- [Heuristic Approach](#heuristic-approach)
  - [Algorithm Overview](#algorithm-overview)
  - [Vertex-Selection Rules](#vertex-selection-rules)
  - [Algorithm Configurations](#algorithm-configurations)
- [Code Architecture](#code-architecture)
  - [Graph Module](#graph-module)
  - [CVSP Module](#cvsp-module)
  - [Benchmark Driver](#benchmark-driver)
  - [Error Handling and Invariants](#error-handling-and-invariants)
  - [Further Implementation Details](#further-implementation-details)
- [Repository Structure](#repository-structure)
- [Input and Output](#input-and-output)
  - [DIMACS Graph Instances](#dimacs-graph-instances)
  - [Benchmark Description and Reference Results](#benchmark-description-and-reference-results)
  - [Generated Benchmark Output](#generated-benchmark-output)
- [Requirements](#requirements)
- [Building and Running](#building-and-running)
- [Reproducibility](#reproducibility)
- [Experimental Results](#experimental-results)
- [Known Limitations](#known-limitations)
- [References](#references)
- [Citation](#citation)
- [License](#license)

## Problem Definition

Let $G = (V, E)$ be a simple undirected graph, and let $k$ and $b$ be positive integers. The **Capacitated Vertex Separator Problem (CVSP)** [1](#ref-1) asks for a minimum-cardinality set $S \subseteq V$, called the **separator**, such that the remaining vertices $V \setminus S$ can be partitioned into at most $k$ disjoint subsets, called **shores**, satisfying the following conditions:

- each shore contains at most $b$ vertices;
- no path connects vertices assigned to different shores.

The objective is therefore to **minimise $|S|$**.

Note that a shore may contain multiple connected components, whereas a connected component cannot be split across different shores.

Once a candidate separator has been constructed, checking its feasibility amounts to assigning the sizes of the remaining connected components to at most $k$ bins of capacity $b$. Determining whether such an assignment exists constitutes an instance of the **Bin Packing Problem (BPP)**, in which connected components correspond to items and shores correspond to bins.

The CVSP is **NP-hard**, which motivates the use of heuristic methods when good feasible solutions are required within short computation times.

### Illustrative Example

The figure below illustrates an optimal solution to a CVSP instance with $k = 3$ and $b = 3$, defined on an 11-vertex simple undirected graph. Removing vertices 2 and 8 yields a separator of cardinality two and produces connected components that can be assigned to three shores, each containing at most three vertices.

<div style="text-align: center;">
  <img
    src="README_assets/cvsp_example.png"
    alt="Optimal solution to a CVSP instance with vertices 2 and 8 in the separator"
    width="100%"
  >
</div>

### Applications to Complex Network Protection

The CVSP can also provide a simplified model for the protection of complex networks. Under suitable assumptions, removing a vertex may represent immunising or otherwise protecting a node, while the parameters $k$ and $b$ constrain the number and maximum size of the disconnected groups that remain. The objective then reflects the need to limit the cost of the intervention while reducing the impact of contagion processes or other phenomena that propagate through the network.

### Further Reading

For a more detailed treatment of the CVSP – including its theoretical properties, integer-programming formulations, and applications to complex network protection – see the [Bachelor's thesis](#ref-0) on which this project is based.

## Heuristic Approach

Since the CVSP is NP-hard, exact solution methods may become computationally expensive as the size of the instance increases. Heuristic approaches trade the guarantee of optimality for the ability to produce good feasible solutions within limited computation times, making them suitable for large instances or time-constrained applications.

Hence, the design of the proposed heuristic is motivated by its potential application to graphs representing real-world networks. Many such networks exhibit small-world characteristics [2](#ref-2) and heterogeneous degree distributions, sometimes associated with scale-free structure [3](#ref-3). In these settings, **targeted removal of highly connected vertices** may fragment the network more effectively than random removal [4](#ref-4). The algorithm therefore prioritises vertices that are considered critical according to local connectivity measures, such as their degree or the connectivity of their neighbourhood.

### Algorithm Overview

The proposed method is a **constructive greedy heuristic** that builds a candidate separator by iteratively selecting and removing one vertex at a time from the graph.

1. Starting from the original graph, the algorithm examines each connected component whose cardinality exceeds $b$.

2. A vertex is selected according to the chosen configuration and removed from the graph.

3. The resulting subcomponents are then processed **recursively** until every remaining connected component contains at most $b$ vertices.

The set of removed vertices constitutes the candidate separator.

Once the vertex-removal phase is complete, if the number of the resulting connected components does not exceeds $k$, then the candidate separator is also feasible. Otherwise, the feasibility of the candidate separator is assessed through the corresponding BPP instance. Since the BPP is NP-hard, the implementation handles this subproblem using the following heuristic procedure.

1. **Best-Fit Decreasing (BFD)** bin-packing heuristic [5](#ref-5) – The connected components of the residual graph are ordered by decreasing size, and each component is assigned to the shore that leaves the smallest residual capacity; a new shore is created when none of the existing ones has sufficient space.

2. If BFD produces an assignment using at most $k$ shores, the set of removed vertices defines a feasible CVSP solution. If more than $k$ shores are used, however, this does not necessarily imply that no feasible assignment exists: since BFD does not guarantee the minimum number of shores, another packing could potentially assign the same components to at most $k$ shores. Only an optimal solution to the corresponding BPP instance could determine whether such an assignment is impossible. In the current heuristic implementation, an additional vertex-removal phase is required in this case but is not implemented, as discussed in [Known Limitations](#known-limitations).

All algorithm configurations share this structure and differ only in the ordered rules used to select the vertex removed at each iteration.

### Vertex-Selection Rules

At each removal step, the candidate set consists of the vertices belonging to the oversized connected component currently being processed.

For each **rule** applied by the selected configuration, every current candidate is assigned a **score** according to the corresponding local connectivity metric. The selection procedure then retains the candidates attaining the **maximum** score.

Let $N(v)$ denote the neighbourhood of vertex $v$, and let $\deg(v)$ denote its degree in the current residual graph.

| Symbol | Metric | Score | Interpretation |
|:------:|--------|-------|----------------|
| **D** | **Degree** | $\deg(v)$ | Measures the number of direct neighbours of the vertex |
| **S** | **Sum of adjacent degrees** | $\displaystyle \sum_{u \in N(v)} \deg(u)$ | Measures the overall connectivity of the vertex neighbourhood |
| **A** | **Maximum adjacent degree** | $\displaystyle \max_{u \in N(v)} \deg(u)$ | Measures the degree of the most highly connected neighbour |

The scores are recomputed on the current residual graph, so they may change after each vertex removal.

When a configuration contains **multiple rules**, the corresponding scoring metrics are applied sequentially rather than combined into a single score. After each rule, the selection procedure retains only the candidates attaining the maximum score. If more than one candidate remains, the next rule is applied to this reduced set. The process stops as soon as a single vertex remains.

If the complete sequence of rules does not resolve the tie, one of the remaining candidates is selected uniformly at **random**.

### Algorithm Configurations

The implementation provides **six algorithm configurations**, each defined by an ordered sequence of scoring rules and a final random tie-breaking mechanism. The arrow '→' indicates the order in which the rules are applied.

| Configuration | Selection sequence | Description |
|:-------------:|--------------------|-------------|
| **R** | Random | No scoring metric, selects a candidate uniformly at random. |
| **MD** | D → R | Uses D as the primary scoring metric |
| **MS** | S → R | Uses S as the primary scoring metric |
| **MDS** | D → S → R | Uses D as the primary metric and S as the secondary metric |
| **MSD** | S → D → R | Uses S as the primary metric and D as the secondary metric |
| **MDA** | D → A → R | Uses D as the primary metric and A as the secondary metric |

In the R configuration, selection is entirely random; in all other configurations, random selection is used only when the preceding rules leave more than one candidate.

The order of the rules is significant. For example, MDS and MSD use the same two connectivity measures but may select different vertices because their first rule filters the candidate set before the second rule is applied.

The R configuration serves as a topology-independent baseline for assessing the contribution of the connectivity-based selection rules.

## Code Architecture

The codebase is written entirely in **C++** and is logically divided into **two modules**:
- `Graph` – handles graph representation and structural operations;
- `CVSP` – implements the problem-specific solving logic.

Each module consists of a header file (`.hpp`) and an implementation file (`.cpp`) defining a single class whose name matches the module, together with a set of supporting free functions.

The dependency is one-way: the `CVSP` module depends on `Graph`, whereas the graph representation remains independent of the problem-specific parameters and solving procedure. This separation of responsibilities was adopted to reduce dependencies and improve the readability, maintainability, reusability, and extensibility of the codebase. Correctness and ease of debugging were also central considerations throughout the implementation.

The **benchmark workflow** is coordinated in the entry-point source file (`main.cpp`), which defines the entry point for the project's single executable.

### Graph Module

The `Graph` class represents a simple undirected graph using an **adjacency-list structure**. It is responsible for loading graph instances from **DIMACS files** storing their vertices and edges, and providing the graph-traversal operations required to identify connected components.

Supporting free functions provide index validation, **Depth-First Search (DFS)**, and connected-component extraction over selected subsets of vertices.

### CVSP Module

The `CVSP` class **represents a problem instance** defined by an input graph and the parameters $k$ and $b$. During each execution, it maintains the adjacency list of the current residual graph, the set of removed vertices, the remaining connected components, and the computation time.

Its `solve()` method coordinates the complete heuristic procedure:
1. it resets the internal state,
2. recursively constructs the candidate separator, 
3. if necessary, applies the bin-packing procedure,
4. and verifies the capacity constraints.

The module also defines the local connectivity metrics used by the algorithm configurations. The functions `degree()`, `sum_adj_degree()`, and `max_adj_degree()` only compute a numerical score for a given vertex. Candidate comparison, progressive filtering according to the ordered metric sequence, and final random tie-breaking are handled separately by the vertex-selection functions.

### Benchmark Driver

The entry-point source file (`main.cpp`) acts as the benchmark driver of the project. It defines the six implemented algorithm configurations and manages the experimental workflow by loading each benchmark instance, executing every configuration.

For each instance, the driver performs both a single execution and **repeated executions within a cumulative time limit**. It records the separator cardinality and the computation time of the single run, together with the best separator cardinality obtained during the repeated runs. The collected results are written to a **CSV file** for subsequent analysis.

### Error Handling and Invariants

The project follows a consistent strategy for distinguishing failures related to external resources from internal logic errors.

- Errors involving external resources – such as missing paths, unsupported file extensions, empty files, or file-opening failures – are reported by **throwing exceptions** accompanied by descriptive error messages.

- Internal consistency conditions are checked through extensive use of `assert` statements. Assertions are used to verify function preconditions, intermediate conditions, postconditions, and **class invariants**, including the consistency of graph data, vertex indices, the residual adjacency list, the separator, and the connected components. Detecting an invalid state causes the program to **terminate immediately**, preventing the error from propagating through the computation.

Assertions are primarily intended as development-time diagnostic checks and **may be disabled in Release builds**. They should therefore be understood as a mechanism for detecting programming errors rather than as a substitute for runtime validation of external input.

### Further Implementation Details

A more detailed discussion of the implementation is provided in the Bachelor's thesis on which this project is based. The thesis also contains the complete code listing developed for the study. Full bibliographic details are available in [References](#references).

## Repository Structure

The repository root is `cvsp-greedy-heuristic/`, which is organised as follows:

```text
cvsp-greedy-heuristic/
├── .gitignore
├── README.md
├── README_assets/
│   └── cvsp_example.png
├── thesis_results/
│   └── benchmark_analysis.xlsx
└── cvsp/
    └── ...
```

`thesis_results/benchmark_analysis.xlsx` contains the manually processed results and the resulting analysis from the experimental campaign conducted for the thesis. This file is not generated by the executable.

The C++ project itself is contained in the `cvsp/` subdirectory. Unless otherwise stated, all paths referring to the C++ project in the following sections are relative to `cvsp/`.

The main directories and files have the following purposes:
- `include/` contains the interfaces of the `Graph` and `CVSP` modules;
- `src/` contains their corresponding implementations;
- `main/` contains the entry-point file;
- `instances/` contains the DIMACS graph instances used in the experiments;
- `results/RisultatiDimacsCVSP.csv` contains the benchmark-instance data and reference results used by the program.

```text
cvsp/
├── .clang-format
├── CMakeLists.txt
├── include/
│   ├── cvsp.hpp
│   └── graph.hpp
├── src/
│   ├── cvsp.cpp
│   └── graph.cpp
├── main/
│   └── main.cpp
├── instances/
│   ├── anna.col.dimacs
│   ├── david.col.dimacs
│   └── ...
└── results/
    └── RisultatiDimacsCVSP.csv
```

After running the executable, the benchmark driver writes its raw output to `results/results.csv`.

All build and execution commands must be run from `cvsp/`. The `build/` directory is generated locally by CMake and is not included in the repository structure shown above.

Both `results/results.csv` file and the `build/` directory are intentionally excluded from version control.

## Input and Output

The current executable is designed to run the complete benchmark and does not receive input paths or CVSP parameters through command-line arguments. It reads the benchmark description from `results/RisultatiDimacsCVSP.csv` and loads the corresponding graph files from `instances/`.

### DIMACS Graph Instances

The input graphs are stored in `instances/` as files with the `.col.dimacs` suffix. The `Graph` module supports the following DIMACS line types:

- lines beginning with `c` are treated as comments and ignored;
- the problem line `p edge <vertices> <edges>` specifies the number of vertices and edges;
- each line `e <u> <v>` defines an undirected edge between vertices $u$ and $v$.

For example:

```text
c Example graph
p edge 4 3
e 1 2
e 2 3
e 3 4
```

Vertices in the provided DIMACS instances are indexed starting from 1. The parser assumes that the input files are well formed and describe simple undirected graphs conformly to the DIMACS format.

### Benchmark Description and Reference Results

The semicolon-separated file `results/RisultatiDimacsCVSP.csv` contains one row for each CVSP benchmark instance. Its original columns are:

| Column | Content |
|--------|---------|
| `name` | Base name of the corresponding graph file |
| `vertices` | Number of vertices in the graph |
| `edges` | Number of edges in the graph |
| `k` | Maximum number of shores |
| `category` | Benchmark category associated with the instance |
| `b` | Maximum cardinality of each shore |
| `primal` | Reference feasible solution value |
| `dual` | Reference lower bound |
| `gap` | Reported optimality gap |
| `time` | Computation time reported for the reference method |
| `status` | Status of the reference solution |

For each row, the benchmark driver constructs the graph path by appending `.col.dimacs` to the value stored in `name`. It reads $k$ and $b$ to construct the CVSP instance and checks that the vertex and edge counts match the corresponding DIMACS file.

The remaining fields preserve the results and metadata reported for the reference branch-and-cut method and are copied to the generated output for comparison.

### Generated Benchmark Output

At the beginning of each execution, any existing `results/results.csv` file is removed and replaced with a newly generated semicolon-separated file.

The output retains all columns from `RisultatiDimacsCVSP.csv` and appends three values for each algorithm configuration:

| Suffix | Content |
|--------|---------|
| `sol` | Separator cardinality obtained from a single execution |
| `time (µs)` | Computation time of the single execution, measured in microseconds |
| `1s` | Best separator cardinality found through repeated executions within the cumulative one-second limit |

The output contains solution cardinalities rather than the individual vertices belonging to the separators.

Because randomised tie-breaking is used, running the benchmark again may produce different separator cardinalities. The generated `results/results.csv` file is therefore excluded from version control.

## Requirements

The project has no third-party library dependencies and relies only on the C++ standard library.

Building the project requires:

- a **C++20-compatible compiler**;
- **CMake 3.28** or later;
- **Ninja**, when following the build procedure documented in the next section.

## Building and Running

The project uses **CMake** as its build system and **Ninja** as the build tool. The commands below use the **Ninja Multi-Config** generator, allowing both the Debug and Release configurations to be built from the same build directory.

From the `cvsp/` directory, the following commands can be used to generate the `build/` directory and compile the executables in either configuration:

```bash
cmake -S . -B build -G "Ninja Multi-Config"
cmake --build build --config Debug
cmake --build build --config Release
```

Both commands generate an executable named `computation`. Run the desired configuration from the `cvsp/` directory:

```bash
build/Debug/computation
build/Release/computation
```

Only one of the two executables needs to be run. The **Debug** configuration is intended for development and diagnostic checks, whereas the **Release** configuration should be used for performance measurements and benchmark execution.

The executable processes the complete benchmark without requiring command-line arguments. It reads the graph instances and reference data from their predefined locations and writes the generated raw results to `results/results.csv`.

The program must be executed from the `cvsp/` directory. Running it from inside `build/` would prevent the relative paths to `instances/` and `results/` from being resolved correctly.

## References

<a id="ref-0"></a>

U. Vezio (2025) *Algoritmi euristici per il Capacitated Vertex Separator Problem: sviluppo e analisi computazionale*. Bachelor's thesis in Management Engineering, University of Bologna.

<a id="ref-1"></a>

[1] Fabio Furini, Ivana Ljubić, Enrico Malaguti, Paolo Paronuzzi (2022) Casting Light on the Hidden Bilevel Combinatorial Structure of the Capacitated Vertex Separator Problem. *Operations Research* 70(4):2399-2420. https://doi.org/10.1287/opre.2021.2110

<a id="ref-2"></a>
[2] Watts, D., Strogatz, S. Collective dynamics of ‘small-world’ networks. *Nature* **393**, 440–442 (1998). https://doi.org/10.1038/30918

<a id="ref-3"></a>
[3] Albert-László Barabási, Réka Albert, Emergence of Scaling in Random Networks. *Science* **286**, 509-512 (1999). https://doi.org/10.1126/science.286.5439.509

<a id="ref-4"></a>
[4] Albert, R., Jeong, H. & Barabási, AL. Error and attack tolerance of complex networks. *Nature* **406**, 378–382 (2000). https://doi.org/10.1038/35019019

<a id="ref-5"></a>
[5] D. S. Johnson, A. Demers, J. D. Ullman, M. R. Garey, R. L. Graham. Worst-Case Performance Bounds for Simple One-Dimensional Packing Algorithms. *SIAM Journal on Computing* 3(4):299-325 (1974). https://doi.org/10.1137/0203025

Dopo aver riletto tutto attentamente, ti allego la mia versione modificata e completa, che devi riguardare per sollevare tutte le criticità presenti, ovviamente con buonsenso e senza essere eccessivamente puntiglioso.