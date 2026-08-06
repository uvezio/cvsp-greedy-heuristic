# A Configurable Greedy Heuristic for the Capacitated Vertex Separator Problem

This repository contains a **C++** implementation of a configurable **greedy heuristic** for the **Capacitated Vertex Separator Problem (CVSP)** – a combinatorial optimisation problem on graphs with applications to complex networks – developed as part of my Bachelor's thesis in Management Engineering (2025).

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
- [Requirements](#requirements)
- [Building and Running](#building-and-running)
- [Reproducibility](#reproducibility)
- [Experimental Results](#experimental-results)
- [Known Limitations](#known-limitations)
- [References](#references)
- [Citation](#citation)
- [License](#license)

## Problem Definition

Let $G = (V, E)$ be a simple undirected graph, and let $k$ and $b$ be positive integers. The **Capacitated Vertex Separator Problem (CVSP)** asks for a minimum-cardinality set $S \subseteq V$, called the **separator**, such that the remaining vertices $V \setminus S$ can be partitioned into at most $k$ disjoint subsets, called **shores**, satisfying the following conditions:

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

### Applications to Complex-Network Protection

The CVSP can also provide a simplified model for the protection of complex networks. Under suitable assumptions, removing a vertex may represent immunising or otherwise protecting a node, while the parameters $k$ and $b$ constrain the number and maximum size of the disconnected groups that remain. The objective then reflects the need to limit the cost of the intervention while reducing the impact of contagion processes or other phenomena that propagate through the network.

### Further Reading

For a more detailed treatment of the CVSP – including its theoretical properties, integer-programming formulations, and applications to complex-network protection – see the Bachelor's thesis on which this project is based. Full bibliographic details are provided in [References](#references).

## Heuristic Approach

Since the CVSP is NP-hard, exact solution methods may become computationally expensive as the size of the instance increases. Heuristic approaches trade the guarantee of optimality for the ability to produce good feasible solutions within limited computation times, making them suitable for large instances or time-constrained applications.

Hence, the design of the proposed heuristic is motivated by its potential application to graphs representing real-world networks. Many such networks exhibit small-world characteristics and heterogeneous degree distributions, sometimes associated with scale-free structure. In these settings, **targeted removal of highly connected vertices** may fragment the network more effectively than random removal. The algorithm therefore prioritises vertices that are considered critical according to **local connectivity** measures, such as their degree or the connectivity of their neighbourhood.

### Algorithm Overview

The proposed method is a **constructive greedy heuristic** that builds a candidate separator by iteratively selecting and removing one vertex at a time from the graph.

1. Starting from the original graph, the algorithm examines each connected component whose cardinality exceeds $b$.

2. A vertex is selected according to the chosen configuration and removed from the graph.

3. The resulting subcomponents are then processed **recursively** until every remaining connected component contains at most $b$ vertices.

The set of removed vertices constitutes the candidate separator.

Once the vertex-removal phase is complete, if the number of the resulting connected components does not exceeds $k$, then the candidate separator is also feasible. Otherwise, the feasibility of the candidate separator is assessed through the corresponding BPP instance. Since the BPP is NP-hard, the implementation handles this subproblem using the following heuristic procedure.

1. **Best-Fit Decreasing (BFD)** bin-packing heuristic – The connected components of the residual graph are ordered by decreasing size, and each component is assigned to the shore that leaves the smallest residual capacity; a new shore is created when none of the existing ones has sufficient space.

2. If BFD produces an assignment using at most $k$ shores, the set of removed vertices defines a feasible CVSP solution. If more than $k$ shores are used, however, **this does not necessarily imply that no feasible assignment exists**: since BFD does not guarantee a minimum number of shores, another packing could potentially assign the same components to at most $k$ shores. Only an optimal solution to the corresponding BPP instance could determine whether such an assignment is impossible. In the current heuristic implementation, an additional vertex-removal phase is required in this case but is not implemented, as discussed in [Known Limitations](#known-limitations).

**All algorithm configurations share this structure** and **differ only in the ordered rules** used to select the vertex removed at each iteration.

### Vertex-Selection Rules

At each removal step, the candidate set consists of the vertices belonging to the oversized connected component currently being processed.

For each rule applied by the selected configuration, every current candidate is assigned a score according to the corresponding local connectivity metric. The selection procedure then retains the candidates attaining the maximum score.

Let $N(v)$ denote the neighbourhood of vertex $v$, and let $\deg(v)$ denote its degree in the current residual graph.

| Symbol | Metric | Score | Interpretation |
|:------:|--------|-------|----------------|
| **D** | **Degree** | $\deg(v)$ | Measures the number of direct neighbours of the vertex. |
| **S** | **Sum of adjacent degrees** | $\displaystyle \sum_{u \in N(v)} \deg(u)$ | Measures the overall connectivity of the vertex neighbourhood. |
| **A** | **Maximum adjacent degree** | $\displaystyle \max_{u \in N(v)} \deg(u)$ | Measures the degree of the most highly connected neighbour. |

The scores are recomputed on the current residual graph, so they may change after each vertex removal.

When a configuration contains **multiple rules**, the corresponding scoring metrics are applied sequentially rather than combined into a single score. After each rule, the selection procedure retains only the candidates attaining the maximum score. If more than one candidate remains, the next rule is applied to this reduced set. The process stops as soon as a single vertex remains.

If the complete sequence of rules does not resolve the tie, one of the remaining candidates is selected uniformly at **random**.

### Algorithm Configurations

The implementation provides **six algorithm configurations**, each defined by an ordered sequence of scoring rules and a final random tie-breaking mechanism. The arrow '→' indicates the order in which the rules are applied. In the **R** configuration, selection is entirely random; in all other configurations, random selection is used only when the preceding rules leave more than one candidate.

| Configuration | Selection sequence | Description |
|:-------------:|--------------------|-------------|
| **R** | Random | Applies no scoring metric and selects a candidate uniformly at random. |
| **MD** | D → Random | Uses D as the primary scoring metric. |
| **MS** | S → Random | Uses S as the primary scoring metric. |
| **MDS** | D → S → Random | Uses D as the primary metric and S as the secondary metric. |
| **MSD** | S → D → Random | Uses S as the primary metric and D as the secondary metric. |
| **MDA** | D → A → Random | Uses D as the primary metric and A as the secondary metric. |

The order of the rules is significant. For example, **MDS** and **MSD** use the same two connectivity measures but may select different vertices because their first rule filters the candidate set before the second rule is applied.

The **R** configuration serves as a topology-independent baseline for assessing the contribution of the connectivity-based selection rules.

## Code Architecture

The codebase is entirely written in **C++** and is logically divided into two modules: `Graph`, which handles **graph representation and structural operations**, and `CVSP`, which implements the **problem-specific solving logic**. Each module consists of a header file (`.hpp`) and an implementation file (`.cpp`) defining a single class whose name matches the module, together with a set of supporting free functions.

The dependency is one-way: the `CVSP` module depends on `Graph`, whereas the graph representation remains independent of the problem-specific parameters and solving procedure. This separation of responsibilities was adopted to reduce dependencies and improve the **readability, maintainability, reusability, and extensibility** of the codebase. Correctness and ease of debugging were also central considerations throughout the implementation.

The **benchmark workflow** is coordinated in the entry-point source file (`main.cpp`), which defines the entry point for the project's single executable.

### Graph Module

The `Graph` class represents a simple undirected graph using an **adjacency-list structure**. It is responsible for loading graph instances from **DIMACS files** storing their vertices and edges, and providing the graph-traversal operations required to identify connected components.

Supporting free functions provide index validation, **Depth-First Search (DFS)**, and connected-component extraction over selected subsets of vertices.

### CVSP Module

The `CVSP` class represents a problem instance defined by an input graph and the parameters $k$ and $b$. During each execution, it maintains the adjacency list of the current residual graph, the set of removed vertices, the remaining connected components, and the computation time.

Its `solve()` method coordinates the complete heuristic procedure: it resets the internal state, recursively constructs the candidate separator, applies the bin-packing phase, and verifies the capacity constraints.

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