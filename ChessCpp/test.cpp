#include "chess.hpp"
#include "Eval.hpp"

#include <queue>
#include <chrono>
#include <ctime>
#include <fstream>
#include <limits>
#include <unordered_map>

void tokenize(const std::string move, std::string &from_square, std::string &to_square, chess::PieceType &promotion_piece, int &is_castling)
{
    promotion_piece = chess::PieceType::NONE;
    int move_length = move.length();
    if (move == "O-O")
    {
        from_square = "";
        to_square = "";
        is_castling = -1;
    }
    else if (move == "O-O-O")
    {
        from_square = "";
        to_square = "";
        is_castling = 1;
    }
    else if (move_length == 4)
    {
        from_square = move.substr(0, 2);
        to_square = move.substr(2, 2);
    }
    else if (move_length == 5)
    {
        from_square = move.substr(0, 2);
        to_square = move.substr(2, 2);
        switch (move[4])
        {
        case 'q':
            promotion_piece = chess::PieceType::QUEEN;
            break;
        case 'r':
            promotion_piece = chess::PieceType::ROOK;
            break;
        case 'b':
            promotion_piece = chess::PieceType::BISHOP;
            break;
        case 'n':
            promotion_piece = chess::PieceType::KNIGHT;
            break;
        default:
            std::cerr << "Invalid promotion piece." << std::endl;
            break;
        }
    }
    else
    {
        std::cerr << "Invalid move format." << std::endl;
        from_square = "a1";
        to_square = "a1";
    }
}

struct MoveEval
{
    chess::Move move;
    int eval;
};

bool compareMoves(const MoveEval &a, const MoveEval &b)
{
    return a.eval > b.eval;
}

struct TranspositionEntry
{
    int value;
    int depth;
    bool is_exact;
};

std::unordered_map<std::size_t, TranspositionEntry> transposition_table;

int Minimax(chess::Board &data, int depth, int alpha, int beta, bool maximizing_player)
{
    if (depth == 0 || data.isGameOver().first != chess::GameResultReason::NONE)
    {
        Evaluation e(data, chess::Color::WHITE);
        return e.static_eval();
    }

    std::size_t hash = data.hash();
    if (transposition_table.find(hash) != transposition_table.end())
    {
        const TranspositionEntry &entry = transposition_table[hash];
        if (entry.depth >= depth)
        {
            if (entry.is_exact)
                return entry.value;
            if (maximizing_player && entry.value <= alpha)
                return entry.value;
            if (!maximizing_player && entry.value >= beta)
                return entry.value;
        }
    }

    chess::Movelist moves;
    chess::movegen::legalmoves(moves, data);

    if (moves.empty())
    {
        return maximizing_player ? -std::numeric_limits<int>::infinity() : std::numeric_limits<int>::infinity();
    }

    std::vector<MoveEval> move_evals;
    for (const auto &move : moves)
    {
        data.makeMove(move);
        int eval = Evaluation(data, data.sideToMove()).static_eval();
        move_evals.push_back({move, eval});
        data.unmakeMove(move);
    }

    std::sort(move_evals.begin(), move_evals.end(), compareMoves);

    int eval;
    if (maximizing_player)
    {
        int max_eval = -std::numeric_limits<int>::infinity();
        for (auto &move_eval : move_evals)
        {
            data.makeMove(move_eval.move);
            eval = Minimax(data, depth - 1, alpha, beta, false);
            data.unmakeMove(move_eval.move);
            move_eval.move.setScore(eval);
            max_eval = std::max(max_eval, eval);
            alpha = std::max(alpha, eval);
            if (beta <= alpha)
                break; // Beta cut-off
        }
        transposition_table[hash] = {max_eval, depth, true};
        return max_eval;
    }
    else
    {
        int min_eval = std::numeric_limits<int>::infinity();
        for (auto &move_eval : move_evals)
        {
            data.makeMove(move_eval.move);
            eval = Minimax(data, depth - 1, alpha, beta, true);
            data.unmakeMove(move_eval.move);
            move_eval.move.setScore(eval);
            min_eval = std::min(min_eval, eval);
            beta = std::min(beta, min_eval);
            if (beta <= alpha)
                break; // Alpha cut-off
        }
        transposition_table[hash] = {min_eval, depth, true};
        return min_eval;
    }
}

chess::Move find_best_move(chess::Board &data, int max_depth, chess::Color color)
{
    chess::Move best_move;
    int best_eval = (color == chess::Color::WHITE) ? -std::numeric_limits<int>::infinity() : std::numeric_limits<int>::infinity();

    for (int depth = 1; depth <= max_depth; ++depth)
    {
        int alpha = -std::numeric_limits<int>::infinity();
        int beta = std::numeric_limits<int>::infinity();
        chess::Movelist moves;
        chess::movegen::legalmoves(moves, data);

        if (moves.size() == 1)
            return moves[0];

        chess::Move best_move_for_depth = moves[0];
        int best_eval_for_depth = (color == chess::Color::WHITE) ? -std::numeric_limits<int>::infinity() : std::numeric_limits<int>::infinity();

        for (const auto &move : moves)
        {
            data.makeMove(move);
            int eval;
            if (color == chess::Color::WHITE)
            {
                eval = Minimax(data, depth - 1, alpha, beta, false);
            }
            else
            {
                eval = Minimax(data, depth - 1, alpha, beta, true);
            }
            data.unmakeMove(move);

            if ((color == chess::Color::WHITE && eval > best_eval_for_depth) || (color == chess::Color::BLACK && eval < best_eval_for_depth))
            {
                best_eval_for_depth = eval;
                best_move_for_depth = move;
            }

            if (color == chess::Color::WHITE)
            {
                alpha = std::max(alpha, best_eval_for_depth);
            }
            else
            {
                beta = std::min(beta, best_eval_for_depth);
            }
        }

        // Update global best move
        if ((color == chess::Color::WHITE && best_eval_for_depth > best_eval) || (color == chess::Color::BLACK && best_eval_for_depth < best_eval))
        {
            best_eval = best_eval_for_depth;
            best_move = best_move_for_depth;
        }

        // Update transposition table
        std::size_t hash = data.hash();
        transposition_table[hash] = {best_eval, depth, true};
    }

    return best_move;
}

// Currently just lets you play againist the engine in board.txt.
void run_engine(int depth = 30, std::string outfile = "board.txt")
{
    // Engine configuration variables.
    constexpr char STARTFEN[57] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    int number = 0; // used for determining number of moves made
    std::ofstream outputFile(outfile);
    chess::Board board(STARTFEN);

    // Side selection variables
    std::string side_choice;
    chess::Color player_color;

    // Select side
    std::cout << "Choose your side (white/black): ";
    std::cin >> side_choice;
    if (side_choice == "") 
    {
        std::cerr << "Invalid choice! Defaulting to Black.\n";
        player_color = chess::Color::BLACK;
    }
    else if (side_choice == "white" || side_choice == "White")
    {
        player_color = chess::Color::WHITE;
    }
    else if (side_choice == "black" || side_choice == "Black")
    {
        player_color = chess::Color::BLACK;
    }
    else
    {
        std::cerr << "Invalid choice! Defaulting to Black.\n";
        player_color = chess::Color::BLACK;
    }

    if (outputFile.is_open())
    {
        outputFile << board << '\n';
        outputFile << "Move evaluation: " << std::to_string(Evaluation(board, board.sideToMove()).static_eval()) << '\n';
        while (board.isGameOver().first == chess::GameResultReason::NONE && number <= 1000)
        {
            if (board.sideToMove() != player_color)
            {
                std::cout << "Transposition table size: " << transposition_table.size() << "\n";
                chess::Move move = find_best_move(board, depth, player_color);

                // Check that what the computer played is legal. This should never happen, but this is extra assurance.
                chess::Movelist legal_moves;
                chess::movegen::legalmoves(legal_moves, board);
                bool is_legal = false;

                for (const chess::Move moves : legal_moves)
                {
                    if (moves == move)
                    {
                        is_legal = true;
                        break;
                    }
                }
                if (!is_legal) // Something went really wrong, and we need to reset the state
                { 
                    transposition_table.clear();
                    chess::Move move = find_best_move(board, depth, opposite_color);
                }

                outputFile << board.sideToMove() << "'s move: " << move << '\n';
                std::cout << board.sideToMove() << "'s Move: " << move << " (Move number: " << number << ")\n";
                board.makeMove(move);
                outputFile << "Board fen: " << board.getFen() << '\n';
                outputFile << "Board after move:\n" << board << '\n';
                number++;
            }
            else
            {
                std::string player_input, from_square, to_square;
                chess::PieceType promotion_piece;
                chess::Move player_move;
                int is_castling = 0;

                std::cout << "Enter your move (e.g., e2e4 or O-O/O-O-O): ";
                std::cin >> player_input;

                tokenize(player_input, from_square, to_square, promotion_piece, is_castling);
                if (is_castling == -1)
                {
                    if (player_color == chess::Color::BLACK)
                    {
                        player_move = chess::Move::make<chess::Move::CASTLING>(chess::Square::underlying::SQ_E8, chess::Square::underlying::SQ_H8);
                    }
                    else
                    {
                        player_move = chess::Move::make<chess::Move::CASTLING>(chess::Square::underlying::SQ_E1, chess::Square::underlying::SQ_H1);
                    }
                }
                else if (is_castling == 1)
                {
                    if (player_color == chess::Color::BLACK)
                    {
                        player_move = chess::Move::make<chess::Move::CASTLING>(chess::Square::underlying::SQ_E8, chess::Square::underlying::SQ_A8);
                    }
                    else
                    {
                        player_move = chess::Move::make<chess::Move::CASTLING>(chess::Square::underlying::SQ_E1, chess::Square::underlying::SQ_A1);
                    }
                }
                else
                {
                    chess::Square from = chess::Square(from_square);
                    chess::Square to = chess::Square(to_square);
                    if (board.enpassantSq() == to)
                    {
                        player_move = chess::Move::make<chess::Move::ENPASSANT>(from, to);
                    }
                    else
                    {
                        player_move = (player_input.length() == 4) ? chess::Move().make(from, to) : chess::Move().make(from, to, promotion_piece);
                    }
                    std::cout << "Generated move: " << player_move << '\n'
                              << std::endl;
                }

                chess::Movelist legal_moves;
                chess::movegen::legalmoves(legal_moves, board);
                bool is_legal = false;

                for (const chess::Move move : legal_moves)
                {
                    if (move == player_move)
                    {
                        is_legal = true;
                        break;
                    }
                }

                if (is_legal)
                {
                    board.makeMove(player_move);
                    outputFile << "Your move: " << player_move << '\n';
                    outputFile << "Board fen: " << board.getFen() << '\n';
                    outputFile << "Board after move:\n"
                               << board << '\n';
                    outputFile << "Move evaluation: " << std::to_string(Evaluation(board, board.sideToMove()).static_eval()) << '\n';
                    std::cout << "Your Move: " << player_move << " (Move number: " << number << ")\n"
                              << std::endl;
                }
                else
                {
                    std::cerr << "Illegal move entered. Please try again.\n"
                              << std::endl;
                }
            }
        }

        auto game_result = board.isGameOver();
        if (game_result.first == chess::GameResultReason::STALEMATE ||
            game_result.first == chess::GameResultReason::INSUFFICIENT_MATERIAL ||
            game_result.first == chess::GameResultReason::FIFTY_MOVE_RULE ||
            game_result.first == chess::GameResultReason::THREEFOLD_REPETITION)
        {
            std::cout << "The game ended in a draw.\n";
        }
        else if (game_result.second == chess::GameResult::WIN)
        {
            std::cout << "Black wins!\n";
        }
        else if (game_result.second == chess::GameResult::LOSE)
        {
            std::cout << "White wins!\n";
        }
        else
        {
            std::cout << "Game over with result: " << static_cast<int>(game_result.first) << '\n';
        }
    }
    else
    {
        std::cerr << "Error: Unable to open output file\n";
    }
    outputFile.close();
}

int main()
{
    // args: int max_depth, std::string outputfile
    run_engine();
}