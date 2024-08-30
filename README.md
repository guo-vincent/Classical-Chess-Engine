# Chess Engine Overview

This Chess Engine leverages the disservin's chess library for move generation and uses a custom evaluation function to guide the decision-making process. It employs advanced algorithms such as Minimax with Alpha-Beta Pruning, Quiescence Search, and Transposition Tables to optimize move selection and enhance gameplay.

## Features

### Move Generation
- **Chess Library**: Utilizes disservin's chess library for efficient and accurate move generation, ensuring compliance with chess rules.

### Evaluation Function
A custom evaluation function considers both material advantage and tactical factors, including:
- **Doubled Pawns**: Penalizes positions where pawns of the same color are stacked on the same file.
- **Rooks on Open Files**: Rewards positions where rooks are on open files or doubled on the same rank/file, increasing board control.
- **Piece Mobility**: Rewards pieces for occupying positions that maximize their control over the board.

### Search and Evaluation Algorithms
- **Minimax with Alpha-Beta Pruning**
- **Quiescence Search** (Temporarily Disabled)
- **Transposition Tables**

## Installation

### Prerequisites
- **C++ Compiler**: Requires a C++20 compatible compiler (e.g., GCC or Clang).
- **Chess Library**: disservin's chess library is included in the repository, so no separate download is needed.

### Building the Chess Engine
Clone the repository and build the project:

```bash
# If using SSH:
git clone git@github.com:guo-vincent/Chess_Evaluation_Neural_Network.git

# If using HTTPS:
git clone https://github.com/guo-vincent/Chess_Evaluation_Neural_Network.git

### Usage

#### Running the Engine

To Compile:
The main file to use is `test.cpp`.**
Run the following command to compile the engine:
  ```bash
  g++ -std=c++20 -o test test.cpp

The test file when executed, will write a board object in board.txt. 
The location for which the board is to be outputted can be specified.
After entering a move, click out of the file and back in to let the file refresh.

The file has only been tested on c++20. It is unknown how the engine will perform on older c++ versions.

Other Notes:
This engine is stronger playing as white than as black, though it still makes stupid mistakes.
This is not a strong engine. Don't expect amazing plays from it. 
Quiescence Search has been temporarily disabled for being a buggy. It will be reenabled when the bugs are resolved.

Command Line Usage:
In bash:
cd /path_to_this_project/ChessCpp
{compiler test.cpp}
The engine will print the best move it finds for the given position in standard chess notation.

Configuration
To be added later. Currently in progress.

License
This project is licensed under the MIT License - see the LICENSE file for details.

Acknowledgments
Special thanks to disservin for providing the chess library that powers the move generation, and to the open-source community for their continued contributions to chess programming. The source code to their library can be found at {https://github.com/Disservin/chess-library}.
Note that some edits were made to the chess library (implementing hashing functionality with chess::Piece and chess::PieceType) so the file in this engine does not match the original chess library file.