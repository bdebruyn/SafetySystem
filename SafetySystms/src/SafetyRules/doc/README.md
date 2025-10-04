# SafetyRules — LLM‑Generated Test Coverage Report

This repository demonstrates a workflow for using an LLM to **generate code‑coverage tests** for a hierarchical state machine and to **classify unreachable code paths** as either _defensive_ or _dead code_.

The project contains three key artifacts:

- **State machine spec (PlantUML):** `StartChart.plantuml` — authoritative, human‑readable model for expected behavior.
- **Implementation under test:** `SafetyRules.h` — header‑only C++17 state machine with top‑level states and a submachine.
- **Generated tests:** `Test_SafetyRules.cpp` — GoogleTest suite exercising transitions, ignored events, and callback branches.

> This report summarizes how the tests map to the spec and implementation, where the implementation intentionally diverges from the spec, and which branches appear unreachable by design vs. truly dead.

---

## 1) System Under Test

### 1.1 Top‑level states
`Idle` → `Active` → (`Faulted` or `BuildPlateLoader`) → … (and back).

### 1.2 Loader submachine (within `BuildPlateLoader`)
`OpenDoor` → `DoorOpened` → `BuildPlateLoaded` → completion back to `Active` (on door close).
A fault can escape the submachine to `Faulted` from any substate.

### 1.3 Event/Action Surface (as implemented)
Implementation provides:
- `dispatch(Event)` for runtime events
- `startLoader()` API (instead of an event) to enter the loader submachine
- Entry/exit hooks for top‑level states and entry‑actions for loader substates

---

## 2) Spec vs. Implementation

### 2.1 Relevant excerpt from the PlantUML spec
The spec models:
- Initial `[*] -> Idle`
- `Idle -> Active : evActive`
- `Active -> Faulted : evFault`
- `Active -> BuildPlateLoader : evLoadBuildPlate`
- `BuildPlateLoader` submachine with:
  - `[*] -> OpenDoor`
  - `OpenDoor -> DoorOpened : evDoorOpened`
  - `DoorOpened -> BuildPlateLoaded : evBuildPlateLoaded`
  - `BuildPlateLoaded -> [*] : evDoorClosed` (completion)
  - **Additional branch:** `BuildPlateLoaded -> Faulted : evBuildPlateUnloaded`
- Fault escape: `BuildPlateLoader --> Faulted : evFault`
- Other auxiliary arcs (e.g., `Faulted -> Idle : evIdle`, `Active -up-> Idle : evIdle`) present in the sketch

### 2.2 Mapping to the implementation
The implementation uses a different event vocabulary and one imperative entry method:

| Spec (PlantUML)                  | Implementation (`SafetyRules.h`) |
|----------------------------------|----------------------------------|
| `evActive`                       | `evPowerOn`                      |
| `evIdle`                         | _not implemented_                |
| `evLoadBuildPlate`               | `startLoader()` **API**          |
| `evFault`                        | `evFault`                        |
| `evDoorOpened`                   | `evDoorOpened`                   |
| `evBuildPlateLoaded`             | `evBuildPlateLoaded`             |
| `evDoorClosed`                   | `evDoorClosed`                   |
| `evBuildPlateUnloaded`           | _not implemented_                |

**Deltas of note**

- **No `evIdle` paths** are implemented; the spec's `Active -up-> Idle` and `Faulted -> Idle : evIdle` do **not** exist in code.
- **Loader entry is imperative** (`startLoader()`), not event‑driven (`evLoadBuildPlate`). Functionally equivalent for testing.
- **No `evBuildPlateUnloaded`** transition from `BuildPlateLoaded -> Faulted` is present in code. The only BuildPlateLoader escape implemented is `evFault` (from any substate).

These deltas are important when interpreting “missing coverage” warnings from coverage tools: the tests cannot cover behavior that the code simply doesn’t implement.

---

## 3) Test Suite Overview

- **Framework:** GoogleTest
- **Total tests found:** **46** (`TEST_F(SafetyRulesTest, ...)` blocks)
- **Themes covered:**
  - Top‑level transitions (`Idle ↔ Active`, `Active → Faulted`)
  - Entering loader submachine (`startLoader`) and walking the substate chain
  - Fault‑escape from **each** loader substate to `Faulted`
  - **Ignored events** in every top‑level state and each loader substate
  - **Callback true/false branches** for all top‑level entry/exit hooks and loader entry‑actions by explicitly _clearing_ callbacks to take the false sides
  - **Redundant/duplicate requests** (e.g., calling `startLoader()` twice)

The suite exercises both **happy paths** and **negative paths** to stress “do nothing” branches in `dispatch(...)` and callback guards.

---

## 4) Coverage Expectations (Logical)

Although this report does not include a numeric coverage dump, the suite is structured to cover:

### 4.1 Top‑level transitions
- `Idle --evPowerOn--> Active`
- `Active --evPowerOff--> Idle`
- `Active --evFault--> Faulted`
- `Faulted --evPowerOn--> Active`

### 4.2 Loader submachine (happy path)
- `Active --startLoader()--> BuildPlateLoader/OpenDoor`
- `OpenDoor --evDoorOpened--> DoorOpened`
- `DoorOpened --evBuildPlateLoaded--> BuildPlateLoaded`
- `BuildPlateLoaded --evDoorClosed--> Active` (completion & submachine exit)

### 4.3 Loader fault‑escape (from all substates)
- `OpenDoor --evFault--> Faulted`
- `DoorOpened --evFault--> Faulted`
- `BuildPlateLoaded --evFault--> Faulted`

### 4.4 Ignored events (negative testing)
- Non‑applicable events leave state unchanged in `Idle`, `Active`, `Faulted`, and each loader substate.

### 4.5 Callback branches
- **True branches:** When hooks are set (default in fixture `SetUp`), entry/exit counters increment.
- **False branches:** When hooks are cleared, transitions still occur but **no callbacks fire**, covering the `if (callback)` guards in all locations.

> **Note on spec deltas:** There are no tests for `evIdle` or `evBuildPlateUnloaded` because the implementation does not expose them. A coverage tool that includes the spec in its scope would mark those **uncovered by design**, not as test gaps.

---

## 5) Unreachable Code: Defense vs. Dead

The implementation includes two notable “impossible” branches under correct API usage:

1. **`BuildPlateLoader` with `loader == None` inside `dispatch(...)`**  
   - In `dispatch`, the inner `switch(loader)` includes a `LoaderSub::None`/`default` arm that asserts (defensive).  
   - Given the API flow:
     - Entering the submachine is **always** via `startLoader()` which sets `loader = OpenDoor`.
     - Exiting the submachine sets `loader = None` and immediately transitions **out** of `BuildPlateLoader` (either to `Active` or `Faulted`).  
   - Therefore, while _in_ `BuildPlateLoader`, `loader` cannot be `None` unless the class is mis‑used or corrupted.  
   - **Classification:** **Defensive, unreachable** under valid usage.

2. **`enterLoaderSub(LoaderSub::None)`**  
   - `enterLoaderSub(...)` is only ever called with concrete substates (`OpenDoor`, `DoorOpened`, `BuildPlateLoaded`); the `None/default` arm is present but never targeted by callers.  
   - **Classification:** **Dead code** (presently uncallable by design).

The tests appropriately **do not attempt** to cover these branches. Attempting to do so would require violating the public API’s intended usage (e.g., forcing illegal internal state), which would not improve behavioral confidence and would generate false coverage.

---

## 6) Gaps & Divergences

- **Spec‑only transitions** not implemented in code:
  - `evIdle` arcs (e.g., `Active → Idle`, `Faulted → Idle`) — **not in code**
  - `BuildPlateLoaded → Faulted : evBuildPlateUnloaded` — **not in code**
- **Event vs. Method** for loader entry:
  - Spec: `evLoadBuildPlate`
  - Impl: `startLoader()` imperative call (functionally equivalent for behavior; different for coverage accounting)

If these spec elements are required, the implementation should add corresponding events and transitions; the existing tests can be cloned/adapted to cover them.

---

## 7) How to Run

### 7.1 Build & run tests (example with CMake/Clang/LLVM)
```bash
# Configure & build (adjust paths/toolchain as needed)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j

# Run tests
ctest --test-dir build --output-on-failure
```

### 7.2 Collect coverage (LLVM example)
```bash
# Build with coverage flags (example)
cmake -S . -B build-cov -DCMAKE_BUILD_TYPE=Debug   -DCMAKE_CXX_FLAGS="-fprofile-instr-generate -fcoverage-mapping"

cmake --build build-cov -j

# Execute tests to generate .profraw
LLVM_PROFILE_FILE="coverage.profraw" ctest --test-dir build-cov

# Produce report
llvm-profdata merge -sparse coverage.profraw -o coverage.profdata
llvm-cov report ./build-cov/tests/Test_SafetyRules --instr-profile=coverage.profdata   -object ./build-cov/SafetyRules/libSafetyRules.a
```

> If you prefer `gcovr` + `gcc`, swap the flags (`--coverage`) and tooling accordingly.

---

## 8) Findings (Concise)

- The **behavioral surface of the implementation is thoroughly exercised** by the 46 tests, including happy paths, ignored events, fault escapes, and callback true/false branches.
- Two **unreachable** branches in code are **intentional**:
  - Defensive assert in `BuildPlateLoader` when `loader == None` (should never happen in‑state).
  - `enterLoaderSub(None)` default arm (currently **dead**).
- **Spec deltas** account for any missing coverage of `evIdle` and `evBuildPlateUnloaded` — they are **not implemented** and thus **not coverable** without code changes.
- Transition into loader is by **API** (`startLoader`) instead of an **event**; practically equivalent.

---

## 9) Next Steps

- Decide whether to **implement** spec‑only arcs (`evIdle`, `evBuildPlateUnloaded`) or **remove them from the spec** to eliminate confusion in coverage accounting.
- If you implement them, **clone existing tests** to add coverage:
  - `Faulted --evIdle--> Idle`
  - `Active --evIdle--> Idle` (if kept)
  - `BuildPlateLoaded --evBuildPlateUnloaded--> Faulted`
- Optionally add a small **state‑audit API** (e.g., `bool isConsistent()`) and a **negative test** that simulates misuse (behind a test seam) to demonstrate the value of the defensive assert.

---

## 10) Repo Guide

```
.
├── src/SafetyRules/include/SafetyRules/SafetyRules.h   # Header‑only implementation (SUT)
├── doc/StartChart.plantuml                             # Specification model (source of truth)
└── tests/Test_SafetyRules/Test_SafetyRules.cpp         # GoogleTest suite (LLM‑generated)
```

---

### Author’s Note

This README was generated to accompany an LLM‑assisted testing exercise. A PDF version of the metrics and the gcov file can be found under doc/index.html.pdf and doc/SafetyRules.h.html.pdf. 
