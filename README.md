Overview:
This Chess Engine utilizes the disservin's chess library for move generation and employs a custom evaluation function to guide the engine's decision-making process. The engine uses several advanced algorithms, including Minimax with Alpha-Beta Pruning, Quiescence Search, and Transposition Tables, to optimize move selection and provide stronger gameplay.

Features:
Move Generation - The engine leverages disservin's chess library for efficient move generation, ensuring that it complies with chess rules while calculating legal moves for any position.

Evaluation Function: A simple evaluation function is implemented, which considers not only material advantage but also some tactical factors such as:
- Doubled Pawns: Penalizes positions where pawns of the same color are stacked on the same file.
- Rooks on Open Files: Rewards positions where rooks are placed on open files or doubled up on the same rank or file, increasing control over the board.
- Piece Mobility: Rewards pieces for being in positions where they can defend more squares. 

Search and Evaluation Algorithms:
- Minimax with Alpha-Beta Pruning
- Quiescence Search
- Transposition Tables

Installation:

Prerequisites:
C++: This engine is written in C++ and requires a compatible compiler (e.g., GCC or Clang).
disservin's chess library is already included in the github file, and downloading the library seperately is not needed. 

Building the Chess Engine
Clone the chess engine repository and build the project:

if ssh:
git clone git@github.com:guo-vincent/Chess_Evaluation_Neural_Network.git
if url: 
https://github.com/guo-vincent/Chess_Evaluation_Neural_Network.git


Usage
The main file to use is test.cpp. 
Run <g++ -std=c++20 -o test test.cpp> to compile, followed by <./test>
Alternatively, you may directly execute the test.exe file located in the folder.

The test file when executed, will write a board object in board.txt. 
The location for which the board is to be outputted can be specified.
After entering a move, click out of the file and back in to let the file refresh.

The file has only been tested on c++20. It is unknown how the engine will perform on older c++ versions.

Other Notes:
This engine is stronger playing as white than as black, though it still makes stupid mistakes.
This is not a strong engine. Don't expect amazing plays from it. 
Quiescence Search has been temporarily disabled for being a buggy. It will be reenabled when the bugs are resolved.

Command Line Usage
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