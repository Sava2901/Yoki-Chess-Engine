# Yoki Chess Engine

A high-performance, modular C++ chess engine designed for CLI invocation in web backends. The engine implements the UCI (Universal Chess Interface) protocol and provides both a chess engine binary and a move validation utility.

## Features

### Engine Binary (UCI-Compatible)
- **UCI Protocol Support**: Full compatibility with UCI-compliant chess GUIs
- **Advanced Search**: Iterative deepening with alpha-beta pruning
- **Quiescence Search**: Enhanced tactical position evaluation
- **Move Ordering**: MVV-LVA (Most Valuable Victim - Least Valuable Attacker) for captures
- **Transposition Table**: Efficient position caching for improved performance
- **Bitboard Representation**: Fast and memory-efficient board representation
- **Time Management**: Configurable search time limits
- **Evaluation Function**: Material, piece-square tables, king safety, and mobility

### Move Validator Binary
- **FEN Position Analysis**: Load positions from FEN strings
- **Move Legality Checking**: Validate moves according to chess rules
- **Legal Move Generation**: List all legal moves from a given position
- **Interactive Mode**: Command-line interface for testing
- **Batch Processing**: Validate multiple moves programmatically

## Building the Project

### Prerequisites
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.12 or higher

### Build Instructions

```bash
# Clone the repository
git clone <repository-url>
cd Yoki-Chess-Engine

# Create build directory
mkdir build
cd build

# Configure and build
cmake ..
cmake --build . --config Release

# Install (optional)
cmake --install .
```

### Build Targets
- `yoki-engine`: Main UCI-compatible chess engine
- `yoki-validator`: Move validation utility
- `yoki-core`: Core chess logic library (static)

## Usage

### Chess Engine (UCI Mode)

```bash
# Start the engine in UCI mode
./bin/yoki-engine

# With debug output
./bin/yoki-engine --debug

# Show help
./bin/yoki-engine --help
```

#### UCI Commands
The engine supports standard UCI commands:
- `uci` - Initialize UCI mode
- `isready` - Check if engine is ready
- `position startpos` - Set starting position
- `position fen <fen-string>` - Set position from FEN
- `go depth <depth>` - Search to specified depth
- `go movetime <ms>` - Search for specified time
- `stop` - Stop current search
- `quit` - Exit engine

### Move Validator

```bash
# Validate a specific move
./bin/yoki-validator --validate "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" e2e4

# List all legal moves
./bin/yoki-validator --list-moves "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

# Interactive mode
./bin/yoki-validator --interactive

# Show help
./bin/yoki-validator --help
```

#### Interactive Mode Commands
- `validate <fen> <move>` - Validate a move
- `list <fen>` - List legal moves
- `quit` - Exit interactive mode

## Architecture

### Core Modules

#### Board Representation (`board.h/cpp`)
- Bitboard-based position representation
- FEN string parsing and generation
- Move making/unmaking with full state restoration
- Zobrist hashing for position identification
- Castling rights and en passant handling

#### Move Generation (`movegen.h/cpp`)
- Pseudo-legal and legal move generation
- Piece-specific move generation (pawns, knights, bishops, rooks, queens, kings)
- Special moves: castling, en passant, promotions
- Attack detection and check validation

#### Search Engine (`search.h/cpp`)
- Alpha-beta pruning with iterative deepening
- Quiescence search for tactical positions
- Transposition table with Zobrist hashing
- Move ordering (MVV-LVA, killer moves, history heuristic)
- Time management and search depth control

#### UCI Protocol (`uci.h/cpp`)
- Complete UCI command parsing and handling
- Engine options management (Hash size, Threads, etc.)
- Position setup from FEN or move sequences
- Search coordination and result reporting

#### Utilities (`utils.h/cpp`)
- Logging system with configurable levels
- Performance timing and profiling
- String manipulation utilities
- File I/O helpers
- Chess-specific utility functions
- Configuration management

## Design Principles

### Performance
- **Zero Dependencies**: No third-party libraries for maximum portability
- **Bitboard Operations**: Efficient bit manipulation for fast move generation
- **Memory Efficiency**: Minimal memory footprint with optimized data structures
- **Cache-Friendly**: Data structures designed for CPU cache efficiency

### Portability
- **Cross-Platform**: Works on Windows, Linux, and macOS
- **Standard C++17**: Uses only standard library features
- **Compiler Agnostic**: Compatible with major C++ compilers

### Modularity
- **Separation of Concerns**: Clear separation between engine logic and UCI protocol
- **Reusable Components**: Core chess logic can be used independently
- **Clean Interfaces**: Well-defined APIs between modules

## Configuration

### Engine Options (UCI)
- `Hash` (1-1024 MB): Transposition table size
- `Threads` (1-64): Number of search threads (currently single-threaded)
- `Ponder` (true/false): Pondering support

### Build Configuration
- `CMAKE_BUILD_TYPE`: Debug or Release
- `CMAKE_INSTALL_PREFIX`: Installation directory

## Testing

### Manual Testing
```bash
# Test engine with a simple position
echo -e "uci\nposition startpos moves e2e4\ngo depth 5\nquit" | ./bin/yoki-engine

# Test move validation
./bin/yoki-validator --validate "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" e2e4
```

### Integration with Chess GUIs
The engine can be used with any UCI-compatible chess GUI:
- Arena Chess GUI
- ChessBase
- Scid vs. PC
- Cute Chess

## Future Extensions

### Planned Features
- **Multi-threading**: Parallel search implementation
- **Opening Book**: Integration with opening databases
- **Endgame Tablebase**: Syzygy tablebase support
- **Neural Network Evaluation**: NNUE-style evaluation
- **Advanced Time Management**: More sophisticated time allocation
- **Pondering**: Think on opponent's time
- **Chess960 Support**: Fischer Random Chess variant

### Performance Improvements
- **Magic Bitboards**: Faster sliding piece move generation
- **Late Move Reductions**: Reduce search depth for unlikely moves
- **Null Move Pruning**: Skip moves to detect zugzwang
- **Aspiration Windows**: Narrow search windows for better pruning

## Contributing

### Code Style
- Follow existing naming conventions
- Use meaningful variable and function names
- Add comments for complex algorithms
- Maintain const-correctness

### Performance Considerations
- Profile before optimizing
- Prefer algorithms over micro-optimizations
- Consider cache locality in data structure design
- Use appropriate data types for the task

## License

[Add your license information here]

## Acknowledgments

- Chess Programming Wiki for algorithms and techniques
- UCI Protocol specification
- Various open-source chess engines for inspiration


src/
├── engine/               # Pure engine logic
│   ├── Engine.cpp
│   ├── Engine.hpp
│   ├── Search.cpp
│   ├── Search.hpp
│   ├── Evaluation.cpp
│   ├── Evaluation.hpp
│
├── board/                # Board representation and rules
│   ├── Board.cpp
│   ├── Board.hpp
│   ├── Move.cpp
│   ├── Move.hpp
│   ├── FenUtils.cpp
│   └── FenUtils.hpp
│
├── devtools/             # Developer CLI tools and visualizers
│   ├── BoardVisualizer.cpp
│   └── BoardVisualizer.hpp
│
├── io/                   # PGN / FEN / UCI parsers (optional later)
│   ├── PGNReader.cpp
│   ├── PGNReader.hpp
│   └── ...
│
├── main/                 # Test runners / CLI app
│   ├── main.cpp          # Entry point (main method)
│   └── test.cpp          # Dev playground
