# A Configurable Greedy Heuristic for the Capacitated Vertex Separator Problem

This repository contains a **C++** implementation of a configurable greedy heuristic for the **Capacitated Vertex Separator Problem (CVSP)** – a combinatorial optimisation problem on graphs with applications to complex networks – developed as part of my Bachelor's thesis in Management Engineering.

The proposed algorithm constructs a feasible solution by iteratively removing critical vertices from the graph and provides multiple configurations based on different vertex-selection rules. Randomised tie-breaking allows repeated executions to explore different feasible solutions.

The implementation was evaluated on benchmark instances from the literature, comparing the different heuristic configurations with reference results reported for an exact branch-and-cut method.

## Table of Contents

- [Problem Definition](#problem-definition)
- [Heuristic Approach](#heuristic-approach)
  - [Algorithm Overview](#algorithm-overview)
  - [Vertex-Selection Rules](#vertex-selection-rules)
  - [Algorithm Configurations](#algorithm-configurations)
- [Code Architecture](#code-architecture)
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

Let \(G=(V,E)\) be a simple undirected graph, and let \(k\) and \(b\) be positive integers. The **Capacitated Vertex Separator Problem (CVSP)** asks for a minimum-cardinality set \(S \subseteq V\), called the **separator**, such that the remaining vertices \(V \setminus S\) can be partitioned into \(r \leq k\) disjoint subsets, called **shores**, satisfying the following conditions:

- each shore contains at most \(b\) vertices;
- no path connects vertices assigned to different shores.

The objective is therefore to minimise \(|S|\).

A shore may contain multiple connected components, whereas a connected component cannot be split across different shores. Hence, once a candidate separator has been constructed, checking its feasibility amounts to assigning the sizes of the remaining connected components to at most \(k\) bins of capacity \(b\). This assignment constitutes an instance of the **Bin Packing Problem (BPP)**.


The CVSP is **NP-hard**, which motivates the use of heuristic methods when good feasible solutions are required within short computation times.

### Illustrative Example

The figure below shows an optimal solution to a CVSP instance with \(k=3\) and \(b=3\), defined on an 11-vertices simple undirected graph. Removing vertices 2 and 8 yields a separator of cardinality two and produces connected components that can be assigned to three shores, each containing at most three vertices.

<div style="text-align: center;"> <img src="README_assets/cvsp_example.png" alt="Optimal solution to a CVSP instance with vertices 2 and 8 in the separator" width="50%">

### Applications to Complex-Network Protection

The CVSP can also provide a simplified model for the protection of complex networks. Under suitable assumptions, removing a vertex may represent immunising or otherwise protecting a node, while the parameters \(k\) and \(b\) constrain the number and maximum size of the disconnected groups that remain. The objective then reflects the need to limit the cost of the intervention while reducing the impact of contagion processes, or other phenomena that propagate through the network.

### Further Reading

For a more detailed treatment of the CVSP – including its theoretical properties, integer-programming formulations, and interpretation or further examples in the context of complex-network protection – see the Bachelor's thesis on which this project is based. Full bibliographic details are provided in [References](#references).