#include "chess.hpp"
#include "Eval.hpp"

#include <queue>
#include <chrono>
#include <ctime>
#include <fstream>
#include <limits>
#include <unordered_map>

constexpr char STARTFEN[57] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

void tokenize(const std::string move, std::string &from_square, std::string &to_square, chess::PieceType &promotion_piece, int &is_castling) {
    promotion_piece = chess::PieceType::NONE;
    int move_length = move.length();
    if (move == "O-O") {
        from_square = "";
        to_square = "";
        is_castling = -1;
    } else if (move == "O-O-O") {
        from_square = "";
        to_square = "";
        is_castling = 1;
    } else if (move_length == 4) {
        from_square = move.substr(0, 2);
        to_square = move.substr(2, 2);
    } else if (move_length == 5) {
        from_square = move.substr(0, 2);
        to_square = move.substr(2, 2);
        switch (move[4]) {
            case 'q': promotion_piece = chess::PieceType::QUEEN; break;
            case 'r': promotion_piece = chess::PieceType::ROOK; break;
            case 'b': promotion_piece = chess::PieceType::BISHOP; break;
            case 'n': promotion_piece = chess::PieceType::KNIGHT; break;
            default: std::cerr << "Invalid promotion piece." << std::endl; break;
        }
    } else {
        std::cerr << "Invalid move format." << std::endl;
        from_square = "a1";
        to_square = "a1";
    }
}

struct MoveEval {
    chess::Move move;
    int eval;
};

bool compareMoves(const MoveEval &a, const MoveEval &b) {
    return a.eval > b.eval; // Higher evaluation comes first
}

struct TranspositionEntry {
    int value;
    int depth;
    bool is_exact;
};

std::unordered_map<std::size_t, TranspositionEntry> transposition_table;

// THIS NEEDS A LOT OF WORK
std::vector<chess::Move> generate_noisy_moves(const chess::Movelist &moves, const chess::Board &data) {
    std::vector<chess::Move> noisy_moves;
    for (const auto &move: moves) {
        if (data.isCapture(move) || move.typeOf() == chess::Move::PROMOTION) {
            noisy_moves.push_back(move);
            continue;
        }
        chess::Board new_board = data;
        new_board.makeMove(move);
        if (new_board.inCheck()) {
            noisy_moves.push_back(move);
        }
    }
    return noisy_moves;
}

// Will be fixed once generate_noisy_moves is fixed.
int QuiescenceSearch(chess::Board &data, int alpha, int beta, bool maximizing_player) {
    Evaluation evale(data, data.sideToMove());
    int stand_pat = evale.static_eval();

    if (data.isGameOver().first != chess::GameResultReason::NONE) {
        return stand_pat;
    }

    std::size_t hash = data.hash();
    if (transposition_table.find(hash) != transposition_table.end()) {
        const TranspositionEntry &entry = transposition_table[hash];
        if (entry.is_exact) return entry.value;
        if (maximizing_player && entry.value <= alpha) return entry.value;
        if (!maximizing_player && entry.value >= beta) return entry.value;
    }

    if (maximizing_player) {
        if (stand_pat >= beta) {
            return stand_pat; // Beta cut-off
        }
        alpha = std::max(alpha, stand_pat);
    } else {
        if (stand_pat <= alpha) {
            return stand_pat; // Alpha cut-off
        }
        beta = std::min(beta, stand_pat);
    }

    chess::Movelist moves;
    chess::movegen::legalmoves(moves, data);
    std::vector<chess::Move> noisy_moves = generate_noisy_moves(moves, data);

    if (noisy_moves.empty()) {
        return stand_pat;
    }

    if (maximizing_player) {
        int max_eval = alpha;
        for (auto &move: noisy_moves) {
            data.makeMove(move);
            int eval = QuiescenceSearch(data, alpha, beta, false);
            data.unmakeMove(move);
            move.setScore(eval);
            max_eval = std::max(max_eval, eval);
            alpha = std::max(alpha, eval);
            if (alpha >= beta) break; // Beta cut-off
        }
        return max_eval;
    } else {
        int min_eval = beta;
        for (auto &move: noisy_moves) {
            data.makeMove(move);
            int eval = QuiescenceSearch(data, alpha, beta, true);
            data.unmakeMove(move);
            move.setScore(eval);
            min_eval = std::min(min_eval, eval);
            beta = std::min(beta, eval);
            if (alpha >= beta) break; // Alpha cut-off
        }
        return min_eval;
    }
}

int Minimax(chess::Board &data, int depth, int alpha, int beta, bool maximizing_player) {
    if (depth == 0 || data.isGameOver().first != chess::GameResultReason::NONE) {
        // Evaluation e(data);
        // return e.static_eval();
        return QuiescenceSearch(data, alpha, beta, maximizing_player);
    }

    std::size_t hash = data.hash();
    if (transposition_table.find(hash) != transposition_table.end()) {
        const TranspositionEntry &entry = transposition_table[hash];
        if (entry.depth >= depth) {
            if (entry.is_exact) return entry.value;
            if (maximizing_player && entry.value <= alpha) return entry.value;
            if (!maximizing_player && entry.value >= beta) return entry.value;
        }
    }

    chess::Movelist moves;
    chess::movegen::legalmoves(moves, data);

    if (moves.empty()) {
        return maximizing_player ? -std::numeric_limits<int>::infinity() : std::numeric_limits<int>::infinity();
    }

    std::vector<MoveEval> move_evals;
    for (const auto &move: moves) {
        data.makeMove(move);
        int eval = Evaluation(data, data.sideToMove()).static_eval();
        move_evals.push_back({move, eval});
        data.unmakeMove(move);
    }

    std::sort(move_evals.begin(), move_evals.end(), compareMoves);

    int eval;
    if (maximizing_player) {
        int max_eval = -std::numeric_limits<int>::infinity();
        for (auto &move_eval: move_evals) {
            data.makeMove(move_eval.move);
            eval = Minimax(data, depth - 1, alpha, beta, false);
            data.unmakeMove(move_eval.move);
            move_eval.move.setScore(eval);
            max_eval = std::max(max_eval, eval);
            alpha = std::max(alpha, eval);
            if (beta <= alpha) break; // Beta cut-off
        }
        transposition_table[hash] = {max_eval, depth, true};
        return max_eval;
    } else {
        int min_eval = std::numeric_limits<int>::infinity();
        for (auto &move_eval: move_evals) {
            data.makeMove(move_eval.move);
            eval = Minimax(data, depth - 1, alpha, beta, true);
            data.unmakeMove(move_eval.move);
            move_eval.move.setScore(eval);
            min_eval = std::min(min_eval, eval);
            beta = std::min(beta, min_eval);
            if (beta <= alpha) break; // Alpha cut-off
        }
        transposition_table[hash] = {min_eval, depth, true};
        return min_eval;
    }
}

chess::Move find_best_move(chess::Board &data, int max_depth, chess::Color color) {
    chess::Move best_move;
    int best_eval;
    if (color == chess::Color::WHITE) {
        int best_eval = -std::numeric_limits<int>::infinity(); 
    } else {
        int best_eval = std::numeric_limits<int>::infinity(); 
    }

    for (int depth = 1; depth <= max_depth; ++depth) {
        int alpha = -std::numeric_limits<int>::infinity();
        int beta = std::numeric_limits<int>::infinity();
        chess::Movelist moves;
        chess::movegen::legalmoves(moves, data);

        if (moves.size() == 1) return moves[0];

        chess::Move best_move_for_depth = moves[0];
        int best_eval_for_depth = -std::numeric_limits<int>::infinity();

        for (const auto &move: moves) {
            data.makeMove(move);
            int eval;
            if (color == chess::Color::WHITE){
                eval = Minimax(data, depth-1, alpha, beta, false);
            } else {
                eval = Minimax(data, depth-1, alpha, beta, true);
            }
            data.unmakeMove(move);

            if (eval >= best_eval_for_depth) {
                best_eval_for_depth = eval;
                best_move_for_depth = move;
            }

            alpha = std::max(alpha, best_eval_for_depth);
        }

        // Update global best move
        if (best_eval_for_depth >= best_eval) {
            best_eval = best_eval_for_depth;
            best_move = best_move_for_depth;
        }

        // Update transposition table
        std::size_t hash = data.hash();
        transposition_table[hash] = {best_eval, depth, true};
    }

    return best_move;
}

// Currently just lets you play againist the engine in t.txt.
int main() {
    int number = 0;
    std::ofstream outputFile("t.txt");
    // chess::Board board("1b6/8/8/5qb1/5p2/7K/6P1/k4n2 w KQkq - 0 1");
    chess::Board board(STARTFEN);
    int depth = 10;
    if (outputFile.is_open()) {
        outputFile << board << '\n';
        outputFile << "Move evaluation: " << std::to_string(Evaluation(board, board.sideToMove()).static_eval()) << '\n';
        while (board.isGameOver().first == chess::GameResultReason::NONE && number <= 1000) {
            if (board.sideToMove() == chess::Color::WHITE) {
                std::cout << "Transposition table size: " << transposition_table.size() << "\n";
                chess::Move move = find_best_move(board, depth, chess::Color::WHITE);
                std::cout << "Move found: " << move << std::endl;
                board.makeMove(move);
                outputFile << "White's move: " << move << '\n';
                outputFile << "Board fen: " << board.getFen() << '\n';
                outputFile << "Move evaluation: " << std::to_string(move.score()) << '\n';
                std::cout << "Move evaluation: " << std::to_string(move.score()) << '\n';
                outputFile << "Board after move:\n" << board;
                number++;
                std::cout << "White Move: " << move << " (Move number: " << number << ")\n\n";
            } else {
                std::string player_input, from_square, to_square;
                chess::PieceType promotion_piece;
                chess::Move player_move;
                int is_castling = 0;

                std::cout << "Enter your move (e.g., e2e4 or O-O/O-O-O): ";
                std::cin >> player_input;
                
                tokenize(player_input, from_square, to_square, promotion_piece, is_castling);
                if (is_castling == -1) {
                    player_move = chess::Move::make<chess::Move::CASTLING>(chess::Square::underlying::SQ_E8, chess::Square::underlying::SQ_H8);
                    std::cout << "Generated move: kingside castle.\n";
                } else if (is_castling == 1) {
                    player_move = chess::Move::make<chess::Move::CASTLING>(chess::Square::underlying::SQ_E8, chess::Square::underlying::SQ_A8);
                    std::cout << "Generated move: queenside castle.\n";
                } else {
                    chess::Square from = chess::Square(from_square);
                    chess::Square to = chess::Square(to_square);
                    if (board.enpassantSq() == to) {
                        player_move = chess::Move::make<chess::Move::ENPASSANT>(from, to);
                    } else {
                        player_move = (player_input.length() == 4) ? chess::Move().make(from, to): chess::Move().make(from, to, promotion_piece);
                    }
                    std::cout << "Tokenized move: from " << from_square << " to " << to_square;
                    if (promotion_piece != chess::PieceType::NONE) {
                        std::cout << " with promotion to " << static_cast<char>(promotion_piece);
                    }
                    std::cout << std::endl;
                    std::cout << "Generated move: " << player_move << std::endl;
                }
                    
                chess::Movelist legal_moves;
                chess::movegen::legalmoves(legal_moves, board);
                bool is_legal = false;

                for (const chess::Move move : legal_moves) {
                    if (move == player_move) {
                        is_legal = true;
                        break;
                    }
                }

                if (is_legal) {
                    board.makeMove(player_move);
                    outputFile << "Your move: " << player_move << '\n';
                    outputFile << "Board fen: " << board.getFen() << '\n';
                    outputFile << "Board after move:\n" << board << '\n';
                    outputFile << "Move evaluation: " << std::to_string(Evaluation(board, board.sideToMove()).static_eval()) << '\n';
                    std::cout << "Your Move: " << player_move << " (Move number: " << number << ")\n\n";
                } else {
                    std::cerr << "Illegal move entered. Please try again.\n";
                }
            } 
        }

        auto game_result = board.isGameOver();
        if (game_result.first == chess::GameResultReason::STALEMATE || 
            game_result.first == chess::GameResultReason::INSUFFICIENT_MATERIAL ||
            game_result.first == chess::GameResultReason::FIFTY_MOVE_RULE ||
            game_result.first == chess::GameResultReason::THREEFOLD_REPETITION) {
            std::cout << "The game ended in a draw.\n";
        } else if (game_result.second == chess::GameResult::WIN) {
            std::cout << "Black wins!\n";
        } else if (game_result.second == chess::GameResult::LOSE) {
            std::cout << "White wins!\n";
        } else {
            std::cout << "Game over with result: " << static_cast<int>(game_result.first) << '\n';
        }
    } else {
        std::cerr << "Error: Unable to open output file\n";
        return 1;
    }
    outputFile.close();
    return 0;
}


                // Paste this in the else section if one wants the bot to play as black
                /*
                auto new_move = find_best_move(board, depth, chess::Color::BLACK);
                outputFile << "Black's move: " << new_move << '\n';
                board.makeMove(new_move);
                outputFile << "Board fen: " << board.getFen() << '\n';
                outputFile << "Board after move:\n" << board;
                outputFile << "Move evaluation: " << std::to_string(Evaluation(board).static_eval()) << '\n';
                std::cout << "Black Move: " << new_move << " (Move number: " << number << ")\n";
                */