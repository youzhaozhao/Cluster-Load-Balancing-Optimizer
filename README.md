# Cluster Load Balancing Optimizer

### Hybrid Heuristic Optimization and Bandwidth-Constrained Migration Simulation

<p align="center">
  <img src="https://img.shields.io/badge/C++-17-blue?style=for-the-badge&logo=c%2B%2B" alt="C++ version">
  <img src="https://img.shields.io/badge/Algorithm-Simulated%20Annealing-red?style=for-the-badge">
  <img src="https://img.shields.io/badge/Graph-Floyd--Warshall-green?style=for-the-badge">
  <img src="https://img.shields.io/badge/Optimization-NP--Hard-orange?style=for-the-badge">
</p>

<p align="center">
  <b>⚙️ A Hybrid Optimization Framework for Server Cluster Load Balancing</b><br>
  <i>Capacity-aware allocation, global optimization, and bandwidth-constrained migration scheduling</i>
</p>

---

## 📖 Overview

This project implements a **server cluster load balancing optimizer** designed to address:

* Uneven server load distribution
* High task migration cost
* Limited network bandwidth constraints

The problem is formulated as a **combinatorial NP-hard optimization problem** under multiple constraints:

1. Node capacity constraints
2. Migration cost minimization
3. Bandwidth-limited task scheduling

To solve this, we design a **three-stage hybrid optimization pipeline**:

1. **All-pairs shortest path precomputation (Floyd–Warshall)**
2. **Greedy initialization**
3. **Time-controlled simulated annealing**
4. **Discrete-event migration simulation with bandwidth contention**

The system separates **planning (static optimization)** and **execution (dynamic scheduling)**, enabling both theoretical optimal cost computation and realistic migration time simulation.

---

## 🧱 System Architecture

```
Input Data
   │
   ▼
Floyd–Warshall (O(N³))
   │
   ▼
Greedy Allocation Initialization
   │
   ▼
Simulated Annealing Optimization
   │
   ▼
Bandwidth-Constrained Migration Simulation
   │
   ▼
Final Allocation + Migration Log
```

---

## 🧠 Algorithm Design

### 1️⃣ Preprocessing: Floyd–Warshall

* Computes global shortest paths for all node pairs.
* Converts repeated path search into O(1) cost lookup.
* Maintains a `next_hop` routing table for path reconstruction.

Time Complexity:
[
O(N^3), \quad N \le 50
]


### 2️⃣ Greedy Initialization

To avoid poor random starting states:

* Sort tasks by descending demand.
* Assign each task to the lowest-cost feasible node.
* Respect capacity constraints.

This generates a high-quality initial feasible solution.


### 3️⃣ Simulated Annealing (Global Optimization)

To escape local optima:

* Randomly perturb task assignments.
* Accept worse solutions probabilistically using the Metropolis criterion.
* Apply temperature cooling.
* Use **time-based termination** instead of fixed iterations.
* Implement a reheating mechanism to avoid premature freezing.

Acceptance rule:

[
P = \exp(-\Delta E / T)
]

This hybrid strategy consistently improves upon greedy-only solutions.


### 4️⃣ Discrete-Event Migration Simulation

Planning determines optimal start/end nodes.

Execution simulates real-world constraints:

* Time is discretized into steps.
* Each link has limited bandwidth.
* Tasks compete for link access.
* Congestion introduces queueing delay.
* Migration logs are recorded.

This stage ensures:

* Total cost remains optimal.
* Total migration time reflects real network contention.

---

## 📊 Experimental Example

For the provided benchmark:

| Scenario                    | Total Migration Cost | Migration Rounds |
| --------------------------- | -------------------- | ---------------- |
| Baseline (Greedy + Direct)  | 18                   | 1 (idealized)    |
| Optimized (SA, infinite BW) | 15                   | 1                |
| Optimized + Bandwidth Limit | 15                   | 4                |

Key observations:

* Simulated annealing reduces cost from 18 → 15.
* Bandwidth constraints increase migration time but not total cost.
* Multiple globally optimal solutions may exist.

---

## 🚀 Compilation & Execution

### Compile

```bash
g++ -std=c++17 -O2 main.cpp -o optimizer
```

### Run

```bash
./optimizer < input.txt
```

---

## 📂 Project Structure

```
cluster-load-balancing-optimizer/
│
├── main.cpp
├── README.md
├── reprot.pdf
└── sample_input.txt
```

---

## 📈 Output Format

1. Task allocation details
2. Final node load distribution
3. Total migration cost
4. Total migration time steps
5. Migration logs (time-step based)

Log format:

```
time_step task_id from_node to_node
```

Example:

```
1 2 1 3
```

Meaning:

At time step 1, task 2 moves from node 1 to node 3.

---


## 📄 License

This project is licensed under the MIT License – see the [LICENSE](LICENSE) file for details.

---

<p align="center">
  <i>
    For more details, please refer to the 
    <a href="report.pdf">full report</a>.
  </i>
</p>
