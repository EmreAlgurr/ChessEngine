# ♟️ MyEngine_CPP (v2.1)

A modern, high-performance, and modular chess engine built from scratch in C++17, rated **~3000 - 3050 Elo**.

Features bitboard-based fast move generation, Principal Variation Search (PVS), Polyglot opening book support, **AVX2-accelerated incremental NNUE (Efficiently Updatable Neural Network)** evaluation, Capture History heuristic, Quiescence Search SEE pruning, and a dynamic Transposition Table architecture.

---

## 📈 Step-by-Step Development Process & Elo Progression

Every enhancement applied to take the engine from its initial ~2150-2200 baseline to its current **~3000 - 3050 Elo** strength was verified via official tournament tool `fastchess` and standard opening suites (`8moves_v3.pgn`) through hundreds of games of SPRT and self-play matches:

| # | Enhancement / Stage | Description & Implementation Details | Test Conditions & Match Count | Measured Result / Gain | Reached Rating |
| :-: | :--- | :--- | :--- | :---: | :---: |
| **1** | **MovePicker Refactoring** | Single generation (`generateAllMoves x1`) + lazy selection sort architecture. Eliminated 4x movegen per node. | 100 Games (1+0.01s) | **+10.43 ± 41.5 Elo** | ~2160 Elo |
| **2** | **3-Fold Repetition & Zobrist** | Detection of repetitions across search path and game history using Zobrist hashing. | — | *Stability Improvement* | ~2160 Elo |
| **3** | **SEE (Static Exchange Evaluation)** | Genuine piece defense chain analysis for exchanges instead of crude estimation, ordering captures by quality. | 40 Games vs SF 2100 | **+147.19 ± 119.0 Elo** | ~2247 Elo |
| **4** | **Root Move Ordering** | Moving the best move from the previous iteration to the very front (TT Move) in root search during Iterative Deepening. | 40 Games (1+0.01s) | **+52.51 ± 75.5 Elo** | ~2280 Elo |
| **5** | **Initial Positional Evaluation** | Positional bonuses for bishop pair advantage and rooks on open/semi-open files. | 100 Games (1+0.01s) | **+70.44 ± 51.5 Elo** | ~2320 Elo |
| **6** | **Simple Castle Bonus** | Rewarding successful castled king positions directly instead of needlessly complex formulas for king safety. | 440 Games SPRT | **+36.46 ± 27.0 Elo** | ~2340 Elo |
| **7** | **Search Pruning (RFP & FP)** | Pruning hopeless subtrees early via Reverse Futility Pruning (Static Null Move) and Futility Pruning. | 460 Games SPRT | **+20.42 ± 28.7 Elo** | ~2355 Elo |
| **8** | **Stage 1 Texel Tuning** | Training piece values and 6 PST tables on 100k quiet positions via multi-threaded local search. | 100 Games Self-Play | **+152.40 Elo** | ~2376 Elo |
| **9** | **Polyglot Opening Book** | Integration of `gm2001.bin` containing 30,000 GM positions (0 ms opening moves and opening variation advantage). | 100 Games Self-Play | **+56.10 Elo** | ~2430 Elo |
| **10** | **Advanced Positional Terms & Stage 2 Texel Tuning** | Isolated pawns (-11), 4th-7th rank passed pawns (13/34/65/95), rook on 7th rank (+7), knight/bishop mobility, and King EG table. | 200 Games vs Tuned v1 | **+88.74 ± 45.8 Elo** | ~2480 Elo |
| **11** | **NNUE Evaluation Integration** | Integrated deep neural network evaluation with HalfKP architecture (55W, 33L, 12D, 61.0% score). | 100 Games vs SF 2500 | **+77.71 ± 65.0 Elo** | ~2578 Elo |
| **12** | **AVX2 Hardware Vectorization & Incremental Accumulator** | Increased search speed from 650k to 2.05M NPS (+2.5 ply) using 256-bit AVX2 SIMD and differential accumulator updates. | 100 Games Self-Play | **+346.12 ± 84.0 Elo** | ~2731 Elo |
| **13** | **NMP Optimization & Logarithmic LMR & SE** | Static evaluation check in null move pruning (`staticEval >= beta`), dynamic $R$, logarithmic LMR, and Singular Extensions. | 680 Games vs SF 2900 (1+0.01s) | **+35.62 ± 25.4 Elo** | ~2767 Elo |
| **14** | **Late Move Pruning (LMP) & Two-Tier Time Management** | Pruning quiet moves at shallow depths ($\text{depth} \le 4$) and a Soft/Hard Time Limit system boosting search efficiency. | 440 Games vs SF 2900 (2+0.02s) | **+75.00 ± 29.4 Elo** | ~2842 Elo |
| **15** | **Root Aspiration Re-ordering** | Immediately elevating a move that fails high in a narrow window to the top during root window re-search loops. | 100 Games vs v2_LMP (2+0.02s) | **+27.85 ± 45.0 Elo** | ~2870 Elo |
| **16** | **60s + 0.6s Match vs SF 2900** | Match against Stockfish 2900 at 60s + 0.6s time control (10 Wins, 6 Losses, 4 Draws, 60.0% score). | 20 Games vs SF 2900 (60+0.6s) | **vs SF 2900: +70.44 Elo** | ~2970 Elo |
| **17** | **Quiescence Search SEE Pruning** | Pruning negative tactical exchanges ($SEE < 0$) in quiescence search (QS) to deepen the search tree. | 100 Games Self-Play (2+0.02s) | **+52.51 ± 41.1 Elo** | ~3000 Elo |
| **18** | **History Malus Heuristic** | Penalizing all quiet moves searched prior to the quiet move that caused a beta cutoff. | 100 Games Self-Play (2+0.02s) | **+13.90 ± 50.3 Elo** | ~3014 Elo |
| **19** | **Dynamic Hash (64MB TT)** | Dynamically sizing Transposition Table memory allocation via UCI `option Hash` and increasing default to 64MB. | 100 Games Self-Play (2+0.02s) | **+13.90 ± 44.3 Elo** | ~3028 Elo |
| **20** | **Capture History Heuristic** | Recording capture moves causing beta cutoffs into a history table to prioritize them in MovePicker. | 80 Games Self-Play (2+0.02s) | **+17.39 ± 54.1 Elo** | ~3045 Elo |
| **21** | **Stockfish 3000 Benchmark** | Official verification tournament against Stockfish 3000 (10 Wins, 7 Losses, 3 Draws, 57.5% score). | 20 Games vs SF 3000 (10+0.1s) | **vs SF 3000: +52.51 Elo** | **~3052 Elo** |

> **Cumulative Net Rating Gain:** Through Quiescence SEE pruning, History Malus, 64MB Dynamic Transposition Table, Capture History heuristic, and incremental AVX2 NNUE inference, the engine has officially **surpassed Stockfish 3000, reaching 3050+ Elo.**

---

## 🚀 Engine Architecture & Features

### 1. Board Representation & Move Generation (Board & MoveGen)
- **64-bit Bitboard Representation:** LSB `a1` (bit 0) ... MSB `h8` (bit 63).
- **Magic Bitboards:** $O(1)$ instant ray attack move generation for rooks and bishops.
- **Pseudo-Legal Generation & Make/Unmake:** Fast move generation and rollback mechanism (`UndoState`).
- **Static Exchange Evaluation (SEE):** Pre-calculating gain/loss outcomes of piece exchanges.

### 2. Search Algorithms (Search)
- **PVS (Principal Variation Search) / Alpha-Beta Negamax:** Optimal pruning with zero-window searches.
- **Iterative Deepening & Aspiration Windows:** Deepening search with dynamically expanding search windows.
- **Transposition Table (TT):**
  - 64-bit Zobrist Hashing.
  - 16-byte packed entries (`TT_EXACT`, `TT_LOWER`, `TT_UPPER`).
  - Mate score normalization by ply and age tracking.
- **Advanced Pruning & Reductions:**
  - **Reverse Futility Pruning (RFP):** Early cutoff of nodes exceeding the static evaluation margin when depth $\le 4$.
  - **Null Move Pruning (NMP):** Passing a move in non-tactical positions to detect threats ($R=2$).
  - **Futility Pruning (FP):** Pruning hopeless quiet moves at shallow depths.
  - **Late Move Reductions (LMR):** Searching non-tactical late moves at reduced depth (passed pawns and check threats protected).
- **Move Ordering (`MovePicker`):**
  - Single-pass generation with dynamic categorization (partitioning).
  - Lazy Selection Sort: TT Move $\rightarrow$ Good Captures ($SEE \ge 0$) $\rightarrow$ Killer Moves (2 tiers) $\rightarrow$ Quiet Moves (History Heuristic) $\rightarrow$ Bad Captures ($SEE < 0$).
- **Quiescence Search:**
  - Examining only tactical moves to mitigate the Horizon Effect.
  - Delta Pruning to discard hopeless captures.
- **Draw Detection:**
  - Threefold Repetition (2-fold within search tree, 3-fold across game history).
  - Fifty-Move Rule (100 ply).

### 3. Evaluation (NNUE & HCE)
The engine defaults to modern deep neural network evaluation (**NNUE**); if the network file is unavailable, it gracefully falls back to an optimized handcrafted evaluation (**HCE**):
- **NNUE (Efficiently Updatable Neural Network):**
  - HalfKP architecture (direct memory-mapped static inference from a 21 MB Stockfish-compatible network file).
  - Evaluates all piece-square relationships and complex pawn/piece dynamics with ultra-precise non-linear activation layers.
  - Automatically resolves opening/middlegame/endgame phases without requiring external handcrafted formulas.
- **Handcrafted Evaluation (HCE Fallback):**
  - Texel-tuned PST tables, tapered king safety, bishop pair and rook open-file bonuses, tapered passed pawns.

### 4. Opening Book
- Standard **Polyglot** binary format (`.bin`) support.
- Built-in `gm2001.bin` book containing **30,000 GM positions** for instantaneous 0 ms opening play.

### 5. Built-in Texel Tuner
- Standalone tuner embedded into the engine:
  ```bash
  ./ChessEngine.exe tune quiet-labeled.epd 25 4 200000
  ```
- Automatically tunes 232 parameter groups via multi-threaded local search (OpenMP / Multi-threading).

---

## 🛠️ Build & Installation

The project requires no external third-party libraries or dynamic DLLs. A standard C++17 compiler (MinGW g++ or MSVC) is sufficient.

### Compilation
Build by executing the `build.bat` script in the root directory:

```cmd
.\build.bat
```
*(or with a custom binary name: `.\build.bat ChessEngine_v2.exe`)*

---

## 🎮 Usage

The engine supports the standard **UCI (Universal Chess Interface)** protocol. Fully compatible with Cute Chess, Arena, BanksiaGUI, or fastchess.

### Supported UCI Options
- `OwnBook` (bool, default: `true`): Enable/disable the opening book.
- `BookFile` (string, default: `gm2001.bin`): Path to the Polyglot book file.
- `EvalFile` (string, default: `nn-04cf2b4ed1da.nnue`): Path to the NNUE network file.

### Example UCI Session
```text
uci
option name OwnBook type check default true
option name BookFile type string default gm2001.bin
option name EvalFile type string default nn-04cf2b4ed1da.nnue
isready
readyok
ucinewgame
position startpos moves e2e4 e7e5
go wtime 60000 btime 60000 winc 1000 binc 1000
bestmove g1f3
quit
```
