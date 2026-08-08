# A Configurable Greedy Heuristic for the Capacitated Vertex Separator Problem

This repository contains a **C++** implementation of a configurable **greedy heuristic** for the **Capacitated Vertex Separator Problem (CVSP)** – a combinatorial optimisation problem on graphs with applications to complex networks – developed as part of [my Bachelor's thesis in Management Engineering (2025)](#ref-thesis).

The proposed algorithm constructs a feasible solution through iterative vertex removal followed by a bin-packing feasibility check, and provides multiple configurations based on different vertex-selection rules. Randomised tie-breaking allows repeated runs to explore different feasible solutions.

The implementation was evaluated on benchmark instances from the literature, comparing the different heuristic configurations with reference results reported for an exact branch-and-cut method.

## Table of Contents

- [Problem Definition](#problem-definition)
  - [Illustrative Example](#illustrative-example)
  - [Applications to Complex Network Protection](#applications-to-complex-network-protection)
- [Heuristic Approach](#heuristic-approach)
  - [Algorithm Overview](#algorithm-overview)
  - [Vertex-Selection Rules](#vertex-selection-rules)
  - [Algorithm Configurations](#algorithm-configurations)
- [Code Architecture](#code-architecture)
  - [Graph Module](#graph-module)
  - [CVSP Module](#cvsp-module)
  - [Benchmark Driver](#benchmark-driver)
  - [Error Handling and Invariants](#error-handling-and-invariants)
- [Repository Structure](#repository-structure)
- [Input and Output](#input-and-output)
  - [DIMACS Graph Instances](#dimacs-graph-instances)
  - [Benchmark Description and Reference Results](#benchmark-description-and-reference-results)
  - [Generated Benchmark Output](#generated-benchmark-output)
- [Requirements](#requirements)
- [Building and Running](#building-and-running)
- [Reproducibility](#reproducibility)
- [Experimental Results](#experimental-results)
  - [Global Performance](#global-performance)
  - [Effect of Repeated Runs](#effect-of-repeated-runs)
  - [MDA Overall Performance](#mda-overall-performance)
  - [MDA Performance Across *k* Values](#mda-performance-across-k-values)
  - [Experimental Data](#experimental-data)
- [Known Limitations](#known-limitations)
- [References](#references)
  - [Bachelor's Thesis](#bachelors-thesis)
  - [Scientific References](#scientific-references)
- [Citation](#citation)
- [License](#license)
- [Use of AI](#use-of-ai)

## Problem Definition

Let $G = (V, E)$ be a simple undirected graph, and let $k$ and $b$ be positive integers. The **Capacitated Vertex Separator Problem (CVSP)** [[1]](#ref-1) asks for a minimum-cardinality set $S \subseteq V$, called the **separator**, such that the remaining vertices $V \setminus S$ can be partitioned into at most $k$ disjoint subsets, called **shores**, satisfying the following conditions:

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

For a more detailed treatment of the CVSP – including its theoretical properties, integer-programming formulations, and applications to complex network protection – see the [Bachelor's thesis](#ref-thesis) on which this project is based.

## Heuristic Approach

Since the CVSP is NP-hard, exact solution methods may become computationally expensive as the size of the instance increases. This computational difficulty motivates the use of heuristic approaches, which trade the guarantee of optimality for the ability to produce good feasible solutions within limited computation times, making them suitable for large instances or time-constrained applications.

Hence, the design of the proposed heuristic is motivated by its potential application to graphs representing real-world networks. Many such networks exhibit small-world characteristics [[2]](#ref-2) and heterogeneous degree distributions, sometimes associated with scale-free structure [[3]](#ref-3). In these settings, **targeted removal of highly connected vertices** may fragment the network more effectively than random removal [[4]](#ref-4). The algorithm therefore prioritises vertices that are considered critical according to local connectivity measures, such as their degree or the connectivity of their neighbourhood.

### Algorithm Overview

The proposed method is a **constructive greedy heuristic** that builds a candidate separator by iteratively selecting and removing one vertex at a time from the graph.

1. Starting from the original graph, the algorithm examines each connected component whose cardinality exceeds $b$.

2. A vertex is selected according to the chosen configuration and removed from the graph.

3. The resulting subcomponents are then processed **recursively** until every remaining connected component contains at most $b$ vertices.

The set of removed vertices constitutes the candidate separator.

Once the vertex-removal phase is complete, if the number of the resulting connected components does not exceed $k$, then the candidate separator is also feasible. Otherwise, its feasibility is assessed through the corresponding BPP instance. Since the BPP is NP-hard, the implementation handles this subproblem using the following heuristic procedure.

1. The connected components of the residual graph are packed using the **Best-Fit Decreasing (BFD)** bin-packing heuristic [[5]](#ref-5): they are ordered by decreasing size, and each component is assigned to the shore that leaves the smallest residual capacity; a new shore is created when none of the existing ones has sufficient space.

2. If BFD produces an assignment using at most $k$ shores, the set of removed vertices defines a feasible CVSP solution. If BFD uses more than $k$ shores, this does not necessarily imply that the candidate separator is infeasible: since BFD does not guarantee the minimum number of shores, another packing could potentially assign the same components to at most $k$ shores. Determining whether such an assignment exists would require an exact feasibility check for the corresponding BPP instance; alternatively, the heuristic could proceed with additional vertex removals. Neither procedure is currently implemented, as discussed in [Known Limitations](#known-limitations).

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

The implementation provides **six algorithm configurations**. Five apply one or more scoring rules sequentially and use random selection only to resolve a remaining tie, whereas **R** applies no scoring metric and selects a candidate uniformly at random.

| Configuration | Selection sequence | Description |
|:-------------:|--------------------|-------------|
| **R** | Random | Applies no scoring metric and selects a candidate uniformly at random |
| **MD** | D → Random | Uses D as the primary scoring metric |
| **MS** | S → Random | Uses S as the primary scoring metric |
| **MDS** | D → S → Random | Uses D as the primary metric and S as the secondary metric |
| **MSD** | S → D → Random | Uses S as the primary metric and D as the secondary metric |
| **MDA** | D → A → Random | Uses D as the primary metric and A as the secondary metric |

The order of the rules is significant. For example, MDS and MSD use the same two connectivity measures but may select different vertices because their first rule filters the candidate set before the second rule is applied.

The R configuration serves as a topology-independent baseline for assessing the contribution of the connectivity-based selection rules.

## Code Architecture

The codebase is written entirely in **C++** and is logically divided into **two modules**:

- `Graph` – handles graph representation and structural operations;
- `CVSP` – implements the problem-specific solving logic.

Each module consists of a header file (`.hpp`) and an implementation file (`.cpp`) defining a single class whose name matches the module, together with a set of supporting free functions.

The dependency is one-way: the `CVSP` module depends on `Graph`, whereas the graph representation remains independent of CVSP-specific parameters and solving logic. This separation of responsibilities improves code readability and reusability while limiting dependencies between the two components.

### Graph Module

The `Graph` class represents a simple undirected graph using an **adjacency-list structure**. It is responsible for loading graph instances from **DIMACS files** (see [Input and Output](#input-and-output)), storing their vertices and edges, and providing the graph-traversal operations required to identify connected components.

Supporting free functions provide index validation, **Depth-First Search (DFS)**, and connected-component extraction over selected subsets of vertices.

### CVSP Module

The `CVSP` class **represents a problem instance** defined by an input graph and the parameters $k$ and $b$. During each execution, it maintains:

- the adjacency list of the current residual graph,
- the set of removed vertices,
- the remaining connected components,
- and the computation time.

Its `solve()` method coordinates the complete heuristic procedure:
1. it resets the internal state,
2. recursively constructs the candidate separator,
3. if necessary, applies the bin-packing procedure,
4. and verifies the capacity constraints.

The module also defines the local connectivity metrics used by the algorithm configurations. The functions `degree()`, `sum_adj_degree()`, and `max_adj_degree()` only compute a numerical score for a given vertex. Candidate comparison, progressive filtering according to the ordered metric sequence, and final random tie-breaking are handled separately by the vertex-selection functions.

### Benchmark Driver

The entry-point source file (`main.cpp`) acts as the benchmark driver for the project. It defines the six implemented algorithm configurations and manages the experimental workflow by loading each benchmark instance and executing every configuration.

For each instance, the driver performs both a single run and **repeated runs within a cumulative time limit**.

It records:
- the separator cardinality and the computation time of the single run;
- the best separator cardinality obtained during the repeated runs.

The collected results are written to a **CSV file** for subsequent analysis (see [Input and Output](#input-and-output)).

### Error Handling and Invariants

The project follows a consistent strategy for distinguishing failures related to external resources from internal logic errors.

- Errors involving external resources – such as missing paths, unsupported file extensions, empty files, or file-opening failures – are reported by **throwing exceptions** accompanied by descriptive error messages.

- Internal consistency conditions are checked through extensive use of `assert` statements. Assertions are used to verify function preconditions, intermediate conditions, postconditions, and **class invariants**, including the consistency of graph data, vertex indices, the residual adjacency list, the separator, and the connected components. When assertions are enabled, detecting an invalid state causes the program to **terminate immediately**, preventing the error from propagating through the computation (see [Known Limitations](#known-limitations)).

Assertions are primarily intended as development-time diagnostic checks and **may be disabled** in Release builds (see [Building and Running](#building-and-running)). They should therefore be understood as a mechanism for detecting programming errors rather than as a substitute for runtime validation of external input.

A more detailed discussion of the implementation is provided in the [Bachelor's thesis](#ref-thesis) on which this project is based. The thesis also contains the complete code listing developed for the study.

## Repository Structure

The repository root is `cvsp-greedy-heuristic/`, which is organised as follows:

```text
cvsp-greedy-heuristic/
├── .gitignore
├── CITATION.cff
├── LICENSE
├── README.md
├── README_assets/
│   └── cvsp_example.png
└── cvsp/
    └── ...
```

The C++ project itself is contained in the `cvsp/` subdirectory. Unless otherwise stated, all paths referring to the C++ project in the following sections are relative to `cvsp/`.

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

The main directories and files have the following purposes:
- `include/` contains the interfaces of the `Graph` and `CVSP` modules;
- `src/` contains their corresponding implementations;
- `main/` contains the entry-point file;
- `instances/` contains the DIMACS graph instances used in the experiments;
- `results/RisultatiDimacsCVSP.csv` contains the benchmark-instance data and reference results used by the program.

After running the executable, the benchmark driver writes its raw output to `results/results.csv`.

All build and execution commands must be run from `cvsp/`. The `build/` directory is generated locally by CMake and is not included in the repository structure shown above (see [Building and Running](#building-and-running)).

Both `results/results.csv` and the `build/` directory are intentionally excluded from version control.

## Input and Output

The experiments reported in this project use only the benchmark instances based on the **40 DIMACS graphs** considered by Furini et al. [[1]](#ref-1), whose computational study also includes other graph sets.

For these instances, the same CVSP parameters are used, and the original reference results file provided by Furini et al. is included as `results/RisultatiDimacsCVSP.csv`.

The current executable is designed to run the complete benchmark and does not receive input paths or CVSP parameters through command-line arguments. It reads the benchmark description with reference results from `results/RisultatiDimacsCVSP.csv` and loads the corresponding graph files from `instances/`.

### DIMACS Graph Instances

The input graphs are stored in `instances/` as files with the `.col.dimacs` suffix.

The `Graph` module supports the following DIMACS line types:
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

Vertices in the provided DIMACS instances are indexed starting from 1. The parser assumes that the input files are well-formed and describe simple undirected graphs conforming to the DIMACS format (see [Known Limitations](#known-limitations)).

The 40 graphs originate from the graph-colouring benchmarks of the Second DIMACS Implementation Challenge [[6]](#ref-6).

### Benchmark Description and Reference Results

The semicolon-separated file `results/RisultatiDimacsCVSP.csv` contains one row for each CVSP benchmark instance. Its columns are:

| Column | Content |
|--------|---------|
| `Instance` | Base name of the corresponding graph file |
| `Vertex` | Number of vertices in the graph |
| `Edge` | Number of edges in the graph |
| `k` | Maximum number of shores |
| `Fascia` | Benchmark category (`small`, `medium`, or `large`) |
| `b` | Maximum cardinality of each shore |
| `Primal` | Best feasible solution value reported by the reference branch-and-cut method |
| `Dual` | Best lower bound reported by the reference method |
| `Gap` | (Primal - Dual) / Dual × 100 |
| `Time` | Computation time reported for the reference method |
| `Status` | `Optimal` if the reference solution is proven optimal, `Feasible` otherwise |

For each row, the benchmark driver constructs the graph path by appending `.col.dimacs` to the value stored in `Instance`. It then reads `k` and `b` to construct the CVSP instance.

The remaining fields preserve the results and metadata reported for the reference branch-and-cut method and are copied to the generated output for comparison.

### Generated Benchmark Output

At the beginning of each execution, any existing `results/results.csv` file is removed and replaced with a newly generated semicolon-separated file.

The output retains all columns from `RisultatiDimacsCVSP.csv` and appends three values for each algorithm configuration:

| Suffix | Content |
|--------|---------|
| `sol` | Separator cardinality obtained from a single run |
| `time (µs)` | Computation time of the single run, measured in microseconds |
| `1s` | Best separator cardinality found through repeated runs within the cumulative one-second limit |

The output contains solution cardinalities rather than the individual vertices belonging to the separators.

Because randomised tie-breaking is used, running the benchmark again may produce different results. The generated `results/results.csv` file is therefore excluded from version control.

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
cmake --build build --config Debug # for a Debug build
cmake --build build --config Release # for a Release build
```

Both commands generate an executable named `computation`. Run the desired configuration from the `cvsp/` directory:

```bash
build/Debug/computation
build/Release/computation
```

Only one of the two executables needs to be run. The **Debug** configuration is intended for development and diagnostic checks, whereas the **Release** configuration should be used for performance measurements and benchmark execution.

The executable processes the complete benchmark without requiring command-line arguments. It reads the graph instances and reference data from their predefined locations and writes the generated raw results to `results/results.csv` (see [Input and Output](#input-and-output)).

The program must be executed from the `cvsp/` directory. Running it from inside `build/` would prevent the relative paths to `instances/` and `results/` from being resolved correctly.

## Reproducibility

The experimental procedure can be reproduced by building the project in **Release** configuration and running the `computation` executable as described above. The original experimental campaign reported in the Bachelor's thesis was conducted under **Windows Subsystem for Linux (WSL)** on a machine equipped with an **Intel Core i5-3470 processor** clocked at **3.20 GHz** and **8 GB of RAM**.

Exact reproduction of the reported results is not guaranteed. Random tie-breaking is performed using a `std::mt19937` engine seeded from `std::random_device`, and the current implementation does not use a fixed random seed. Consequently, different executions may produce different separators.

Moreover, the repeated-run experiment is limited by a **cumulative solve time of one second** rather than by a fixed number of executions. The number of runs performed within this interval, as well as the measured computation times, may therefore depend on the hardware and execution environment.

## Experimental Results

The heuristic was evaluated on **296 CVSP instances** derived from the 40 DIMACS graphs described above. The experiments used the same values of $k$ and $b$ considered by Furini et al. [[1]](#ref-1).

The instances were grouped into three classes according to $k$:

- **small** ($k \in \{4,8,12\}$), containing 120 instances;
- **medium** ($k \in \{16,24,32\}$), containing 117 instances;
- **large** ($k \in \{64,128,256\}$), containing 59 instances.

For all $k$, the $b$ value was fixed to:

```math
b = \left\lceil \frac{|V|}{k} \right\rceil
```

excluding instances for which $b = 1$.

Each of the six algorithm configurations was evaluated both through a **single run** and through **repeated runs within a cumulative one-second limit**.

Solution quality was assessed against the reference values reported by Furini et al. for their exact branch-and-cut algorithm, **C+CV** [[1]](#ref-1). Of the 296 reference instances:

- **182 were solved to proven optimality**, whereas
- for the remaining **114** only the best feasible solution found by C+CV was available.

Consequently, outperforming a reference value is possible for the latter group.

### Global Performance

The following tables summarise the overall performance of the six configurations across all 296 benchmark instances.

For each instance, the solution ratio $\rho$ is defined as the ratio between the separator cardinality obtained by the heuristic and the corresponding C+CV reference value; the tables report its average over all benchmark instances.

#### Single Run

| Configuration | Average $\rho$ | Known optima found | Non-optimal references matched | Non-optimal references improved | Average time (ms) |
|:-------------:|:------:|:------------------:|:----------------------------:|:-------------------------------:|:-----------------:|
| **R**   | 2.09 | 0  | 10 | 1 | 2.026 |
| **MD**  | 1.08 | 72 | 11 | 6 | 1.236 |
| **MS**  | 1.10 | 39 | 11 | 3 | 1.732 |
| **MDS** | 1.08 | 80 | 11 | 7 | 1.269 |
| **MSD** | 1.09 | 60 | 9  | 3 | 1.683 |
| **MDA** | 1.07 | 78 | 13 | 7 | 1.294 |

#### Repeated Runs

| Configuration | Average $\rho$ | Known optima found | Non-optimal references matched | Non-optimal references improved |
|:-------------:|:------:|:------------------:|:----------------------------:|:-------------------------------:|
| **R**   | 1.75 | 10  | 10 | 8  |
| **MD**  | 1.05 | 104 | 36 | 21 |
| **MS**  | 1.07 | 70  | 30 | 9  |
| **MDS** | 1.06 | 89  | 28 | 14 |
| **MSD** | 1.07 | 73  | 25 | 9  |
| **MDA** | 1.05 | 105 | 41 | 23 |

Average computation time is reported only for the single-run setting, since repeated runs use a fixed cumulative one-second budget.

### Effect of Repeated Runs

Repeated runs improve the solution quality of every configuration, confirming the benefit of exploiting random tie-breaking to explore different feasible separators. Overall, **MD and MDA** provide the best results.

By contrast, the purely random **R** configuration performs substantially worse: on a single run, its separators are more than twice the size of the C+CV reference solutions on average. This supports the use of topology-aware vertex-selection criteria, particularly those based on **vertex degree**.

### MDA Overall Performance

Among the tested configurations, **MDA** achieves the strongest overall results.

With repeated runs, its average solution ratio is $\rho = 1.05$, corresponding to separator cardinalities that are, on average, approximately **5% above the C+CV reference values**. In addition:

- it finds the known optimum for **105 of the 182 instances with a proven optimum** (about 58%),

- matches the non-optimal reference solution in **41 of the remaining 114 instances**,

- and improves the reference solution for **23 of the 114 instances without a proven optimum** (about 20%).

Overall, MDA therefore matches or improves the C+CV reference value on **169 of the 296 instances** (about 57%).

### MDA Performance Across *k* Values

The detailed analysis of MDA also shows a clear dependence on $k$: instances with smaller values of $k$ are more challenging for the heuristic, whereas the solutions approach the reference values as $k$ increases.

Considering only instances with a known optimum, the average $\rho$ values are:

| $k$ class | Single run | Repeated runs |
|-----------|:----------:|:-------------:|
| **Small** | 1.16 | 1.13 |
| **Medium** | 1.04 | 1.02 |
| **Large** | 1.01 | 1.01 |

Thus, on the large-*k* instances the separators produced by MDA are, on average, only about **1% above the known optimum**. The lower accuracy observed for small values of $k$ is consistent with the greater difficulty of these instances also reported for exact approaches in the literature.

### Experimental Data

A complete discussion of the experimental results is provided in the [Bachelor's thesis](#ref-thesis).

The manually processed data from the original experimental campaign are available as `benchmark_analysis.xlsx` in the [GitHub release associated with the thesis](https://github.com/uvezio/cvsp-greedy-heuristic/releases/tag/thesis-2025). The workbook was derived from the raw `results/results.csv` output generated during that campaign and is provided as supplementary analysis material rather than being produced by the executable.

## Known Limitations

- Because BFD is heuristic, using more than $k$ shores does not prove that the candidate separator is infeasible. Handling this case would require either an exact feasibility check for the corresponding BPP instance or an additional vertex-removal phase before attempting the packing again. Neither procedure is currently implemented, and the program terminates if the $k$ constraint remains violated after the BFD step. This situation did not occur during the original experimental campaign considered in the thesis.

- Input parsing assumes well-formed DIMACS instances. Structural properties of the provided benchmark files are additionally checked through assertions, but these checks may be disabled in Release builds; the parser is therefore not intended as a robust validator for arbitrary malformed DIMACS input.

- The implementation was also tested in Debug configuration to detect violations of assertions and internal invariants. However, because every algorithm configuration includes randomised tie-breaking, a single Debug execution cannot exercise all possible execution paths and therefore does not provide an absolute guarantee of correct behaviour.

- The provided executable is benchmark-oriented and does not expose graph paths, CVSP parameters, or algorithm configurations through command-line options.

- The computational conclusions are specific to the benchmark family considered in the study: 296 CVSP instances derived from 40 DIMACS graphs. In particular, the observed superiority of degree-based configurations should not be assumed to generalise unchanged to graph classes with substantially different topological properties.

- Finally, although the heuristic contains a stochastic component, the experimental study does not include a formal statistical analysis of its variability. Results are summarised through aggregate performance indicators, but no standard deviations, confidence intervals, or distributions over independent experimental repetitions were computed. The reported comparisons should therefore be interpreted as descriptive results of the experimental campaign rather than as a statistical characterisation of the algorithm's stochastic behaviour.

## References

<a id="ref-thesis"></a>

### Bachelor's Thesis

U. Vezio, *Algoritmi euristici per il Capacitated Vertex Separator Problem: sviluppo e analisi computazionale*. Bachelor's thesis in Management Engineering, University of Bologna, 2025.

The [thesis PDF](https://github.com/uvezio/cvsp-greedy-heuristic/releases/download/thesis-2025/vezio_thesis_cvsp_2025.pdf) and the manually processed experimental results are available in the [associated GitHub release](https://github.com/uvezio/cvsp-greedy-heuristic/releases/tag/thesis-2025).

### Scientific References

<a id="ref-1"></a>

[1] F. Furini, I. Ljubić, E. Malaguti, P. Paronuzzi, [Casting Light on the Hidden Bilevel Combinatorial Structure of the Capacitated Vertex Separator Problem](https://doi.org/10.1287/opre.2021.2110). *Operations Research* 70(4):2399–2420, 2022.

<a id="ref-2"></a>

[2] D. J. Watts, S. H. Strogatz, [Collective dynamics of ‘small-world’ networks](https://doi.org/10.1038/30918). *Nature* 393(6684):440–442, 1998.

<a id="ref-3"></a>

[3] A.-L. Barabási, R. Albert, [Emergence of Scaling in Random Networks](https://doi.org/10.1126/science.286.5439.509). *Science* 286(5439):509–512, 1999.

<a id="ref-4"></a>

[4] R. Albert, H. Jeong, A.-L. Barabási, [Error and attack tolerance of complex networks](https://doi.org/10.1038/35019019). *Nature* 406(6794):378–382, 2000.

<a id="ref-5"></a>

[5] D. S. Johnson, A. Demers, J. D. Ullman, M. R. Garey, R. L. Graham, [Worst-Case Performance Bounds for Simple One-Dimensional Packing Algorithms](https://doi.org/10.1137/0203025). *SIAM Journal on Computing* 3(4):299–325, 1974.

<a id="ref-6"></a>

[6] D. S. Johnson, M. A. Trick, *[Cliques, Coloring, and Satisfiability: Second DIMACS Implementation Challenge, October 11–13, 1993](https://doi.org/10.1090/dimacs/026)*. Volume 26, American Mathematical Society, 1996.

## Citation

If you use this software in academic or research work, please cite this repository and, where relevant, the associated Bachelor's thesis:

U. Vezio, *Algoritmi euristici per il Capacitated Vertex Separator Problem: sviluppo e analisi computazionale*. Bachelor's thesis in Management Engineering, University of Bologna, 2025.

Citation metadata for the software are also provided in [`CITATION.cff`](CITATION.cff).

## License

The source code and original repository documentation are licensed under the [MIT License](LICENSE).

Third-party benchmark materials are not covered by this license and remain subject to their respective original terms. The graph instances in `cvsp/instances/` originate from the Second DIMACS Implementation Challenge [[6]](#ref-6). The file `cvsp/results/RisultatiDimacsCVSP.csv` contains benchmark data and reference results from Furini et al. [[1]](#ref-1) and is included in this repository with permission from the authors.

The Bachelor's thesis and `benchmark_analysis.xlsx`, distributed separately through the associated GitHub release, are provided as supplementary materials and are not covered by the MIT License.

## Use of AI

Generative AI, specifically ChatGPT, was used extensively as a drafting and editorial aid in the preparation of this README. Its role included proposing wording, structural revisions, and consistency improvements. However, the organisation and content of the document were developed under my active direction: I defined its structure, scope, and contents, selected and revised the proposed text, corrected technical inaccuracies, and verified the final document against the implementation and the Bachelor's thesis. The resulting README should therefore be understood as **AI-assisted rather than autonomously AI-generated**.

The Bachelor's thesis and the underlying work documented by this repository are **entirely original**. The thesis text, data analysis, algorithm design, experimental methodology, and source code were produced by me and were not generated by AI. In these activities, AI tools were used only as a consultative aid and for retrospective checks of correctness, without replacing the underlying reasoning, design, implementation, analysis, or authorship.