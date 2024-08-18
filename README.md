Overview
This Chess Engine utilizes the disservin's chess library for move generation and employs a custom evaluation function to guide the engine's decision-making process. The engine uses several advanced algorithms, including Minimax with Alpha-Beta Pruning, Quiescence Search, and Transposition Tables, to optimize move selection and provide stronger gameplay.

Features
Move Generation: The engine leverages disservin's chess library for efficient move generation, ensuring that it complies with chess rules while calculating legal moves for any position.

Evaluation Function: A simple evaluation function is implemented, which considers not only material advantage but also some tactical factors such as:

Doubled Pawns: Penalizes positions where pawns of the same color are stacked on the same file.
Rooks on Open Files: Rewards positions where rooks are placed on open files or doubled up on the same rank or file, increasing control over the board.
Search Algorithms:

Minimax with Alpha-Beta Pruning: The engine uses the Minimax algorithm to search for the best move, enhanced with Alpha-Beta Pruning to reduce the number of nodes evaluated, making it more efficient.
Quiescence Search: To avoid the horizon effect, quiescence search is implemented to extend the search beyond unstable positions, such as capturing moves, ensuring better evaluations of tactical scenarios.
Transposition Tables: Hash tables are used to store previously evaluated positions, speeding up the search by preventing the re-evaluation of positions that have already been analyzed.
Installation
Prerequisites
C++: This engine is written in C++ and requires a compatible compiler (e.g., GCC or Clang).
disservin's chess library: You can clone the library from GitHub and include it in your project.
To install the chess library:

bash
Copy code
git clone https://github.com/disservin/chess.git
cd chess
Include the chess library in your C++ project according to your build system (e.g., CMake or directly in the IDE).

Building the Chess Engine
Clone the chess engine repository and build the project:

bash
Copy code
git clone https://github.com/yourusername/chess-engine.git
cd chess-engine
make
This will compile the chess engine using the provided Makefile. Ensure that the chess library is properly linked during the compilation process.

Usage
Once compiled, the engine can be executed from the command line or integrated into a chess GUI that supports UCI (Universal Chess Interface).

Command Line Usage
bash
Copy code
./chess-engine
The engine will print the best move found for the given position in standard chess notation.

Example
bash
Copy code
Position: r1bqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
Best Move: e2e4
The engine will take a chess position as input, perform a search, and return the best move based on its evaluation and search depth.

Configuration
You can adjust several engine parameters, such as:

Search Depth: Adjust the depth to which the engine will search before evaluating moves.
Evaluation Weights: Customize the weights given to various factors in the evaluation function, such as material, pawn structure, or control of open files.
Contributing
Feel free to submit issues or pull requests on the GitHub repository if you find any bugs or have ideas for improvements.

License
This project is licensed under the MIT License - see the LICENSE file for details.

Acknowledgments
Special thanks to disservin for providing the chess library that powers the move generation, and to the open-source community for their continued contributions to chess programming.
