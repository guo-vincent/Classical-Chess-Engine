#ifndef EVAL_HPP
#define EVAL_HPP

#include "chess.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

namespace { // To avoid polluting the global namespace
    using Bitboard = chess::Bitboard;
    using Color = chess::Color;
    using PieceType = chess::PieceType;
    using File = chess::File;
    using Rank = chess::Rank;
}

// Need to add ability for pieces to check the possibility of being captured as part of the eval function, king safety, and rooks on the seventh.
// Then I'll work on evaluating king safety.

// @brief A namespace with many useful bit shift functions.
namespace BitOp
{
    /*
    Shifts the bitboard left. Bits at the leftmost file are erased.
    @param bitboard chess::Bitboard object */
    Bitboard shift_left(Bitboard bitboard)
    {
        return bitboard >> 1 & ~Bitboard(File::FILE_H);
    }
    /*
    Shifts the bitboard right. Bits at the rightmost file are erased.
    @param bitboard chess::Bitboard object */
    Bitboard shift_right(Bitboard bitboard)
    {
        return bitboard << 1 & ~Bitboard(File::FILE_A);
    }
    /*
    Shifts all bits backwards. White pieces are shifted down, Black pieces are shifted up.
    Does not take into consideration bit collisions.
    @param bitboard chess::Bitboard object
    @param color chess::Color type
    */
    Bitboard shift_backward(Bitboard bitboard, Color color)
    {
        return (color == Color::WHITE) ? bitboard >> 8 : bitboard << 8;
    }
    /*
    Shifts all bits down (towards the white pieces).
    @param bitboard chess::Bitboard object */
    Bitboard shift_down(Bitboard bitboard)
    {
        // From white's perspective:
        return shift_backward(bitboard, Color::WHITE);
    }
    /*
    Shifts all bits forwards. White pieces are shifted up, Black pieces are shifted down.
    Does not take into consideration bit collisions.
    @param bitboard chess::Bitboard object
    @param color chess::Color type
    */
    Bitboard shift_forward(Bitboard bitboard, Color color)
    {
        return (color == Color::WHITE) ? bitboard << 8 : bitboard >> 8;
    }
    /*
    Shifts all bits up (towards the black pieces).
    @param bitboard chess::Bitboard object */
    Bitboard shift_up(Bitboard bitboard)
    {
        // From white's perspective:
        return shift_forward(bitboard, Color::WHITE);
    }
    /*
    Expands the bits of a bitboard such that each bit set to 1 in the original
    bitboard will set all surrounding bits (diagonally, horizontally, and vertically) to 1.
    @param bitboard chess::Bitboard object*/
    Bitboard expand_bits(Bitboard bitboard)
    {
        Bitboard left_shift = shift_left(bitboard);
        Bitboard right_shift = shift_right(bitboard);
        Bitboard up_shift = shift_up(bitboard);
        Bitboard down_shift = shift_down(bitboard);
        Bitboard up_left = shift_left(up_shift);
        Bitboard up_right = shift_right(up_shift);
        Bitboard down_left = shift_left(down_shift);
        Bitboard down_right = shift_right(down_shift);

        return bitboard | left_shift | right_shift | up_shift | down_shift | up_left | up_right | down_left | down_right;
    }
    /*
    Expands the bits of a bitboard such that each bit set to 1 in the original
    bitboard will set all surrounding bits (diagonally, horizontally, and vertically) to 1. Then sets the original evaluated bit to 0.
    Not meant for use with bitboards that have more than one bit as unpredictable results may occur.
    @param bitboard chess::Bitboard object*/
    Bitboard get_surrounding_bits(Bitboard bitboard)
    {
        Bitboard left_shift = shift_left(bitboard);
        Bitboard right_shift = shift_right(bitboard);
        Bitboard up_shift = shift_up(bitboard);
        Bitboard down_shift = shift_down(bitboard);
        Bitboard up_left = shift_left(up_shift);
        Bitboard up_right = shift_right(up_shift);
        Bitboard down_left = shift_left(down_shift);
        Bitboard down_right = shift_right(down_shift);

        return ~bitboard | left_shift | right_shift | up_shift | down_shift | up_left | up_right | down_left | down_right;
    }
};

// Other helper functions:
namespace Helper
{
    /* Returns true if the intersection of two bitboards contains no 1's.
       @param b1 1st chess::Bitboard object for comparison.
       @param b2 2nd chess::Bitboard object for comparison. */
    static bool is_empty(Bitboard b1, Bitboard b2)
    {
        return (b1 & b2).count() < 1;
    }

    /* Returns true if the intersection of two bitboards contains a 1.
       @param b1 1st chess::Bitboard object for comparison.
       @param b2 2nd chess::Bitboard object for comparison. */
    static bool any(Bitboard b1, Bitboard b2)
    {
        return !is_empty(b1, b2);
    }

    /// @brief Returns a hashmap of the pieces attacking the square.
    /// @param board The chess board
    /// @param square The square to check for attacks
    /// @param color The color of the attacking pieces
    /// @return A hashmap of the pieces attacking the square and their counts
    static std::unordered_map<PieceType, int> isAttackedCount(const chess::Board &board, chess::Square square, Color color)
    {
        std::unordered_map<PieceType, int> attackers_count;

        // Pawn attackes
        if (chess::attacks::pawn(~color, square) & board.pieces(PieceType::PAWN, color))
            attackers_count[PieceType::PAWN]++;

        // Knight attacks
        if (chess::attacks::knight(square) & board.pieces(PieceType::KNIGHT, color))
            attackers_count[PieceType::KNIGHT]++;

        // King attacks
        if (chess::attacks::king(square) & board.pieces(PieceType::KING, color))
            attackers_count[PieceType::KING]++;

        // Check for bishop attacks
        if (chess::attacks::bishop(square, board.occ()) & board.pieces(PieceType::BISHOP, color))
            attackers_count[PieceType::BISHOP]++;

        // Check for rook attacks
        if (chess::attacks::rook(square, board.occ()) & board.pieces(PieceType::ROOK, color))
            attackers_count[PieceType::ROOK]++;

        // Check for queen attacks
        if (chess::attacks::queen(square, board.occ()) & board.pieces(PieceType::QUEEN, color))
            attackers_count[PieceType::QUEEN]++;

        return attackers_count;
    }

    /// @brief Sums up the total number of attackers
    /// @param map the unordered_map of attackers, called by isAttackedCount
    int total_attackers(const std::unordered_map<PieceType, int> &map)
    {
        int k = 0;
        for (auto count : map)
            k += count.second;
        return k;
    }
};

class Evaluation
{
private:
    chess::Board data;
    Color side;
    Bitboard white_pawns;
    Bitboard black_pawns;
    Bitboard white_bishops;
    Bitboard black_bishops;
    Bitboard white_knights;
    Bitboard black_knights;
    Bitboard white_rooks;
    Bitboard black_rooks;
    Bitboard white_queens;
    Bitboard black_queens;
    Bitboard white_king;
    Bitboard black_king;
    Bitboard white_pieces;
    Bitboard black_pieces;
    Bitboard all_pieces;

    // Black chess-piece/position boards:
    std::array<int, 64> black_pawn_table = {
        0, 0, 0, 0, 0, 0, 0, 0,
        50, 50, 50, 50, 50, 50, 50, 50,
        10, 10, 20, 30, 30, 20, 10, 10,
        5, 5, 10, 25, 25, 10, 5, 5,
        0, 0, 0, 20, 20, 0, 0, 0,
        5, -5, -10, -10, -10, -10, -5, 5,
        5, 10, 10, -20, -20, 10, 10, 5,
        0, 0, 0, 0, 0, 0, 0, 0};
    std::array<int, 64> black_knight_table = {
        -50, -40, -30, -30, -30, -30, -40, -50,
        -40, -20, 0, 0, 0, 0, -20, -40,
        -30, 0, 10, 15, 15, 10, 0, -30,
        -30, 5, 15, 20, 20, 15, 5, -30,
        -30, 0, 15, 20, 20, 15, 0, -30,
        -30, 5, 10, 15, 15, 10, 5, -30,
        -40, -20, 0, 5, 5, 0, -20, -40,
        -50, -40, -30, -30, -30, -30, -40, -50};
    std::array<int, 64> black_bishop_table = {
        -20, -10, -10, -10, -10, -10, -10, -20,
        -10, 0, 0, 0, 0, 0, 0, -10,
        -10, 0, 5, 10, 10, 5, 0, -10,
        -10, 5, 5, 10, 10, 5, 5, -10,
        -10, 0, 10, 10, 10, 10, 0, -10,
        -10, 10, 10, 10, 10, 10, 10, -10,
        -10, 10, 0, 0, 0, 0, 10, -10,
        -20, -10, -10, -10, -10, -10, -10, -20};
    std::array<int, 64> black_rook_table = {
        0, 0, 0, 0, 0, 0, 0, 0,
        5, 10, 10, 10, 10, 10, 10, 5,
        -5, 0, 0, 0, 0, 0, 0, -5,
        -5, 0, 0, 0, 0, 0, 0, -5,
        -5, 0, 0, 0, 0, 0, 0, -5,
        -5, 0, 0, 0, 0, 0, 0, -5,
        -5, 0, 0, 0, 0, 0, 0, -5,
        0, 0, 0, 5, 5, 0, 0, 0};
    std::array<int, 64> early_black_queen_table = {
        -30, -20, -20, -20, -20, -20, -20, -30,
        -20, -20, -10, -10, -10, -10, -20, -20,
        -20, -10, -5, -5, -5, -5, -10, -20,
        -10, -10, -5, -5, -5, -5, -10, -10,
        -10, -10, -5, -5, -5, -5, -10, -10,
        -20, -10, -5, -5, -5, -5, -10, -20,
        -20, -20, 100, 100, 100, 100, -20, -20,
        -30, 50, 120, 150, 150, 120, 50, -30};
    std::array<int, 64> late_black_queen_table = {
        -20, -10, -10, -5, -5, -10, -10, -20,
        -10, 0, 0, 0, 0, 0, 0, -10,
        -10, 0, 5, 5, 5, 5, 0, -10,
        -5, 0, 5, 5, 5, 5, 0, -5,
        0, 0, 5, 5, 5, 5, 0, -5,
        -10, 5, 5, 5, 5, 5, 0, -10,
        -10, 0, 5, 0, 0, 0, 0, -10,
        -20, -10, -10, -5, -5, -10, -10, -20};
    std::array<int, 64> black_king_table = {
        -30, -40, -40, -50, -50, -40, -40, -30,
        -30, -40, -40, -50, -50, -40, -40, -30,
        -30, -40, -40, -50, -50, -40, -40, -30,
        -30, -40, -40, -50, -50, -40, -40, -30,
        -20, -30, -30, -40, -40, -30, -30, -20,
        -10, -20, -20, -20, -20, -20, -20, -10,
        20, 20, 0, 0, 0, 0, 20, 20,
        20, 30, 10, 0, 0, 10, 30, 20};

    std::array<int, 64> white_pawn_table;
    std::array<int, 64> white_knight_table;
    std::array<int, 64> white_bishop_table;
    std::array<int, 64> white_rook_table;
    std::array<int, 64> early_white_queen_table;
    std::array<int, 64> late_white_queen_table;
    std::array<int, 64> white_king_table;

    std::array<int, 64> mirror_table(const std::array<int, 64> &table) const
    {
        std::array<int, 64> mirrored_table;
        for (int rank = 0; rank < 8; ++rank)
        {
            for (int file = 0; file < 8; ++file)
            {
                int src_index = rank * 8 + file;
                int dest_index = (7 - rank) * 8 + file;
                mirrored_table[dest_index] = table[src_index];
            }
        }
        return mirrored_table;
    }

    // These are for tracking piece positions on board:
    int pawn_position_score = 0;
    int knight_position_score = 0;
    int bishop_position_score = 0;
    int rook_position_score = 0;
    int queen_position_score = 0;
    int king_position_score = 0;
    int pins_and_checks_score = 0;

    std::unordered_map<PieceType, int> material_value_map;

    // Constants. Will be optimized by machine learning model later. Currently has no getters/setters.
    // Pawn:
    static constexpr int doubled_pawn_penalty = 20;
    static constexpr int isolated_pawn_penalty = 20;
    static constexpr int passed_pawn_bonus = 50;
    static constexpr int pawn_center_control = 100;
    static constexpr int valuable_pawn_captures_bonus = 5;
    static constexpr int backwards_pawn_penalty = 20;
    static constexpr int pawn_chain_bonus = 30;

    // Bishop
    static constexpr int bishop_mobility_bonus = 5;
    static constexpr int bishop_center_bonus = 40;

    // Rook
    static constexpr int rook_open_file_bonus = 35;
    static constexpr int stacked_rooks_bonus = 25;
    static constexpr int rook_mobility_bonus = 5;

    // Knight
    static constexpr int knight_mobility_bonus = 25;

    // All:
    static constexpr int king_restriction_bonus = 8;
    static constexpr int checks_constant = 25;

public:
    Evaluation(chess::Board data, Color side) : data(data),
                                                side(side),
                                                white_pawns(data.pieces(PieceType::PAWN, Color::WHITE)),
                                                black_pawns(data.pieces(PieceType::PAWN, Color::BLACK)),
                                                white_bishops(data.pieces(PieceType::BISHOP, Color::WHITE)),
                                                black_bishops(data.pieces(PieceType::BISHOP, Color::BLACK)),
                                                white_knights(data.pieces(PieceType::KNIGHT, Color::WHITE)),
                                                black_knights(data.pieces(PieceType::KNIGHT, Color::BLACK)),
                                                white_rooks(data.pieces(PieceType::ROOK, Color::WHITE)),
                                                black_rooks(data.pieces(PieceType::ROOK, Color::BLACK)),
                                                white_queens(data.pieces(PieceType::QUEEN, Color::WHITE)),
                                                black_queens(data.pieces(PieceType::QUEEN, Color::BLACK)),
                                                white_king(data.pieces(PieceType::KING, Color::WHITE)),
                                                black_king(data.pieces(PieceType::KING, Color::BLACK)),
                                                white_pieces(data.us(Color::WHITE)),
                                                black_pieces(data.us(Color::BLACK)),
                                                all_pieces(data.occ())
    {
        white_pawn_table = mirror_table(black_pawn_table);
        white_knight_table = mirror_table(black_knight_table);
        white_bishop_table = mirror_table(black_bishop_table);
        white_rook_table = mirror_table(black_rook_table);
        early_white_queen_table = mirror_table(early_black_queen_table);
        late_white_queen_table = mirror_table(late_black_queen_table);
        white_king_table = mirror_table(black_king_table);
    }

    int naive_material_balance()
    {
        int balance = 0;
        for (Color::underlying color : {Color::WHITE, Color::BLACK})
        {
            for (PieceType::underlying piece_type : {PieceType::PAWN, PieceType::KNIGHT, PieceType::BISHOP,
                                                     PieceType::ROOK, PieceType::QUEEN})
            {
                Bitboard pieces = data.pieces(piece_type, color);
                int count = pieces.count();

                // Value of pieces
                int piece_value = 0;
                switch (piece_type)
                {
                case PieceType::PAWN:
                    material_value_map[PieceType::PAWN] = 200;
                    break;
                case PieceType::KNIGHT:
                    material_value_map[PieceType::KNIGHT] = 600;
                    break;
                case PieceType::BISHOP:
                    material_value_map[PieceType::BISHOP] = 700;
                    break;
                case PieceType::ROOK:
                    material_value_map[PieceType::ROOK] = 1000;
                    break;
                case PieceType::QUEEN:
                    material_value_map[PieceType::QUEEN] = 1800;
                    break;
                default:
                    break;
                }
                // (+) if white, (-) if black
                balance += (count * material_value_map[piece_type] * (color == Color::WHITE ? 1 : -1));
            }
        }
        return balance;
    }

    // Evaluating Pawns
    void pawn_structure(Bitboard allied_pawns, Bitboard enemy_pawns, int &doubled_pawns, int &isolated_pawns,
                        int &passed_pawns, int &center, int &valuable_pawn_captures, int &backwards_pawns, int &pawn_chain, Bitboard enemy_king,
                        Bitboard enemy_pieces, const Color &color)
    {
        // Doubled Pawns (-): Weak due to vulnerable position
        // Isolated Pawns (-): Weak due to no pawns on adjacent files
        // Passed Pawns (+): Strong due to no enemy pawn interference
        // Backwards Pawns (-): Easily targeted, and also gives opponent outpost squares
        // Pawn chain (+): Pawn chains are hard to attack. Counts each instance of a pawn supporting another.
        int allied_pawns_count = allied_pawns.count();
        if (allied_pawns_count == 0)
            return; // No pawns, no point evaluating.
        std::vector<chess::Square> pawn_positions;
        pawn_positions.reserve(allied_pawns_count);

        for (int index = 0; index < 8; ++index)
        {
            File file(static_cast<File::underlying>(index));
            Bitboard file_bb(file);
            Rank rank(static_cast<Rank::underlying>(index));
            Bitboard rank_bb(rank);
            Bitboard adj_files_left = BitOp::shift_left(file_bb);
            Bitboard adj_files_right = BitOp::shift_right(file_bb);
            Bitboard pawn_captures_bb = BitOp::shift_left(BitOp::shift_forward(allied_pawns, color) & file_bb) | BitOp::shift_right(BitOp::shift_forward(allied_pawns, color) & file_bb);

            int count = (allied_pawns & file_bb).count();

            // Check pawn is on file
            if (count > 0)
            {
                // Doubled Pawns: increments by 1 if number of pawns on same rank is more than 1.
                if (count > 1)
                    doubled_pawns += count - 1;

                // Isolated Pawns:
                if (Helper::is_empty(allied_pawns, adj_files_left) && Helper::is_empty(allied_pawns, adj_files_right))
                {
                    isolated_pawns += count;
                }

                // Passed Pawns:
                if (Helper::is_empty(enemy_pawns, (adj_files_left | file_bb | adj_files_right)))
                {
                    passed_pawns += count;
                }

                // Captures:
                // Shifts bitboard into capturable spots. First half returns bitboard of capturable squares for the each pawn, second half finds enemy_pieces that are not pawns.
                valuable_pawn_captures += (pawn_captures_bb & (~enemy_pawns & enemy_pieces)).count();

                // Checks and attacks on king:
                if (Helper::any(pawn_captures_bb, enemy_king))
                {
                    pins_and_checks_score += (color == Color::WHITE ? checks_constant : -checks_constant);
                }

                // Restricting King movement:
                if (Helper::any(pawn_captures_bb, BitOp::get_surrounding_bits(enemy_king)))
                {
                    pins_and_checks_score += (color == Color::WHITE ? king_restriction_bonus : -king_restriction_bonus);
                }

                // Backwards pawns & pawn chain:
                int pos_captures = (pawn_captures_bb & allied_pawns).count();
                if (pos_captures == 2)
                {
                    backwards_pawns++;          // Backwards Pawn
                    pawn_chain += pos_captures; // Increment pawn chain bonus by 2
                }
                else if (pos_captures == 1)
                {
                    pawn_chain++; // Increment pawn chain bonus
                    if (Helper::any(pawn_captures_bb, adj_files_left))
                    { // Allied pawn is on the left
                        if (Helper::is_empty(allied_pawns, adj_files_right))
                        { // File on the right has no pawn
                            backwards_pawns++;
                        }
                    }
                    else
                    { // Allied pawn is on the right
                        if (Helper::is_empty(allied_pawns, adj_files_left))
                        { // File on the left has no pawn
                            backwards_pawns++;
                        }
                    }
                }
            }

            // Center Control:
            if (index == 3)
            {
                center += (((file_bb | adj_files_right) & (rank_bb | BitOp::shift_up(rank_bb)) & (allied_pawns)).count());
            }

            // Pawn piece location table:
            Bitboard pawns_in_file = allied_pawns & file_bb;
            while (pawns_in_file)
            {
                int square_index = pawns_in_file.lsb(); // Get least significant bit index
                pawn_positions.push_back(chess::Square(square_index));
                pawn_position_score += (color == Color::WHITE) ? white_pawn_table[square_index] : -black_pawn_table[square_index];
                (void)pawns_in_file.pop(); // Remove the least significant bit
            }
        }

        // Encourages pawns to not be in a position where they can be captured
        for (chess::Square sq : pawn_positions)
        {
            std::unordered_map<PieceType, int> pawn_is_attacked = Helper::isAttackedCount(data, sq, color == Color::WHITE ? Color::BLACK : Color::WHITE);
            std::unordered_map<PieceType, int> pawn_support = Helper::isAttackedCount(data, sq, color);
            int allied_attackers_total = Helper::total_attackers(pawn_support), enemy_attackers_total = Helper::total_attackers(pawn_is_attacked);
            if (enemy_attackers_total && !allied_attackers_total)
            { // Can be attacked and no defenders.
                pawn_position_score -= (color == Color::WHITE ? 40 : -40);
                continue;
            }
            else if (enemy_attackers_total >= allied_attackers_total)
            {                                                                                 // In an exchange, opposing side will be up a piece.
                pawn_position_score -= (enemy_attackers_total - allied_attackers_total) * 10; // Will be adjusted later.
                continue;
            };
        }
    }

    int pawn_score()
    {
        int white_doubled_pawns = 0, black_doubled_pawns = 0;
        int white_isolated_pawns = 0, black_isolated_pawns = 0;
        int white_passed_pawns = 0, black_passed_pawns = 0;
        int white_center = 0, black_center = 0;
        int white_valuable_pawn_captures = 0, black_valuable_pawn_captures = 0;
        int white_backwards_pawns = 0, black_backwards_pawns = 0;
        int white_pawn_chain = 0, black_pawn_chain = 0;

        // White
        Evaluation::pawn_structure(white_pawns, black_pawns, white_doubled_pawns, white_isolated_pawns,
                                   white_passed_pawns, white_center, white_valuable_pawn_captures, white_backwards_pawns,
                                   white_pawn_chain,
                                   black_king, black_pieces, Color::WHITE);

        // Black
        Evaluation::pawn_structure(black_pawns, white_pawns, black_doubled_pawns, black_isolated_pawns,
                                   black_passed_pawns, black_center, black_valuable_pawn_captures, black_backwards_pawns,
                                   black_pawn_chain,
                                   white_king, white_pieces, Color::BLACK);

        /*
        // For debugging
        std::cout << "White doubled pawns: " << white_doubled_pawns << '\n';
        std::cout << "White isolated pawns: " << white_isolated_pawns << '\n';
        std::cout << "White passed pawns: " << white_passed_pawns << '\n';
        std::cout << "White center pawns: " << white_center << '\n';
        std::cout << "White valuable pawn captures: " << white_valuable_pawn_captures << '\n';
        std::cout << "White backwards pawns: " << white_backwards_pawns << '\n';
        std::cout << "White pawn chain: " << white_pawn_chain << '\n' << '\n';
        std::cout << "Black doubled pawns: " << black_doubled_pawns << '\n';
        std::cout << "Black isolated pawns: " << black_isolated_pawns << '\n';
        std::cout << "Black passed pawns: " << black_passed_pawns << '\n';
        std::cout << "Black center pawns: " << black_center << '\n';
        std::cout << "Black valuable pawn captures: " << black_valuable_pawn_captures << '\n';
        std::cout << "Black backwards pawns: " << black_backwards_pawns << '\n';
        std::cout << "Black pawn chain: " << black_pawn_chain << '\n' << '\n';
        */

        return sum_pawn_components(white_doubled_pawns, black_doubled_pawns,
                                   white_isolated_pawns, black_isolated_pawns,
                                   white_passed_pawns, black_passed_pawns,
                                   white_center, black_center,
                                   white_valuable_pawn_captures, black_valuable_pawn_captures,
                                   white_backwards_pawns, black_backwards_pawns,
                                   white_pawn_chain, black_pawn_chain);
    }

    // Function whose only job is to sum up pawn_components.
    int const sum_pawn_components(int &white_doubled_pawns, int &black_doubled_pawns,
                                  int &white_isolated_pawns, int &black_isolated_pawns,
                                  int &white_passed_pawns, int &black_passed_pawns,
                                  int &white_center, int &black_center,
                                  int &white_valuable_pawn_captures, int &black_valuable_pawn_captures,
                                  int &white_backwards_pawns, int &black_backwards_pawns,
                                  int &white_pawn_chain, int &black_pawn_chain)
    {
        return -(white_doubled_pawns - black_doubled_pawns) * doubled_pawn_penalty - (white_isolated_pawns - black_isolated_pawns) * isolated_pawn_penalty + (white_passed_pawns - black_passed_pawns) * passed_pawn_bonus + (white_center - black_center) * pawn_center_control + (white_valuable_pawn_captures - black_valuable_pawn_captures) * valuable_pawn_captures_bonus - (white_backwards_pawns - black_backwards_pawns) * backwards_pawn_penalty + (white_pawn_chain - black_pawn_chain);
    }

    int bishop_score()
    {
        int bishop_pair_bonus = 0;
        int white_bishop_mobility = 0, black_bishop_mobility = 0;
        int white_bishop_center = 0, black_bishop_center = 0;
        bishop_eval(white_bishops, white_bishop_mobility, bishop_pair_bonus, white_bishop_center, black_king, Color::WHITE);
        bishop_eval(black_bishops, black_bishop_mobility, bishop_pair_bonus, black_bishop_center, white_king, Color::BLACK);

        return (white_bishop_mobility - black_bishop_mobility) * bishop_mobility_bonus + (white_bishop_center - black_bishop_center) * bishop_center_bonus + bishop_pair_bonus;
    }

    void bishop_eval(Bitboard bishops, int &mobility, int &bishop_pair_bonus, int &bishop_center, Bitboard enemy_king, Color color)
    {
        int bishops_count = bishops.count();
        if (bishops.count() == 0)
            return; // No bishops, no point evaluating.
        std::vector<chess::Square> bishop_location;
        bishop_location.reserve(bishops_count);

        // Bishop Pair
        if (bishops_count > 1)
        {
            bishop_pair_bonus += (color == Color::WHITE ? 0.5 : -0.5);
        }

        // Locate Bishops by iterating through ranks
        for (int sq_index = 0; sq_index < 64; ++sq_index)
        {
            chess::Square sq(sq_index);
            if (bishops.check(sq_index))
            {
                bishop_location.push_back(sq);
            }
            if (color == Color::WHITE)
            {
                bishop_position_score += white_bishop_table[sq_index];
            }
            else
            {
                bishop_position_score -= black_bishop_table[sq_index];
            }
        }

        // Mobility bonus for taking a lot of squares.
        for (int i = 0; i < bishops_count; ++i)
        {
            Bitboard bishop_attacks = chess::attacks::bishop(bishop_location[i], all_pieces);
            mobility += bishop_attacks.count();

            // Checks:
            if (Helper::any(bishop_attacks, enemy_king))
            {
                pins_and_checks_score += (color == Color::WHITE ? checks_constant : -checks_constant);
            }

            // Restricting king movement
            if (Helper::any(bishop_attacks, BitOp::get_surrounding_bits(enemy_king)))
            {
                pins_and_checks_score += (color == Color::WHITE ? king_restriction_bonus : -king_restriction_bonus);
            }
        }

        // Bonus for Bishops being in the center.
        Bitboard file_bb(File::FILE_D);
        Bitboard rank_bb(Rank::RANK_4);
        bishop_center += (((file_bb | BitOp::shift_right(file_bb)) & (rank_bb | BitOp::shift_up(rank_bb)) & (bishops)).count());

        // Fianchetto Bonus (Not yet implemented)

        // Encourages Bishops to not be in a position where they can be captured
        for (chess::Square sq : bishop_location)
        {
            std::unordered_map<PieceType, int> bishop_is_attacked = Helper::isAttackedCount(data, sq, color == Color::WHITE ? Color::BLACK : Color::WHITE);
            std::unordered_map<PieceType, int> bishop_support = Helper::isAttackedCount(data, sq, color);
            int allied_attackers_total = Helper::total_attackers(bishop_support), enemy_attackers_total = Helper::total_attackers(bishop_is_attacked);
            if (bishop_is_attacked[PieceType::PAWN])
            {
                // Bishop can be taken by a pawn, so it is essentially lost
                bishop_position_score -= (color == Color::WHITE ? 75 : -75); // Will be adjusted
                continue;
            }
            if (enemy_attackers_total && !allied_attackers_total)
            {
                bishop_position_score -= (color == Color::WHITE ? 75 : -75);
                continue;
            }
            else if (enemy_attackers_total >= allied_attackers_total)
            {
                bishop_position_score -= (enemy_attackers_total - allied_attackers_total) * 15; // Will be adjusted later;
                continue;
            }
        }
    }

    int knight_score()
    {
        int white_knight_mobility = 0, black_knight_mobility = 0;
        knight_eval(white_knights, white_knight_mobility, black_king, white_pieces, Color::WHITE);
        knight_eval(black_knights, black_knight_mobility, white_king, black_pieces, Color::BLACK);
        return (white_knight_mobility - black_knight_mobility) * knight_mobility_bonus;
    }

    void knight_eval(Bitboard knights, int &knight_mobility, Bitboard enemy_king, Bitboard allied_pieces, Color color)
    {
        int knights_count = knights.count();
        if (knights_count == 0)
            return; // No knights, no point evaluating.
        std::vector<chess::Square> knight_positions;
        knight_positions.reserve(knights_count);

        for (int index = 0; index < 8; ++index)
        {
            File file(static_cast<File::underlying>(index));
            Bitboard file_bb(file);
            Rank rank(static_cast<Rank::underlying>(index));
            Bitboard rank_bb(rank);

            // Locating knights
            Bitboard knights_in_file = knights & file_bb;
            while (knights_in_file)
            {
                int square_index = knights_in_file.lsb(); // Get least significant bit index
                knight_positions.push_back(chess::Square(square_index));
                // std::cout << color << " Knight Square index: " << std::to_string(square_index) << '\n';
                knight_position_score += (color == Color::WHITE) ? white_knight_table[square_index] : -black_knight_table[square_index];
                (void)knights_in_file.pop(); // Remove least significant bit
            }
        }

        for (int i = 0; i < knights_count; i++)
        {
            Bitboard knight_attacks = chess::attacks::knight(knight_positions[i]);
            // Checks
            if (Helper::any(knight_attacks, enemy_king))
            {
                pins_and_checks_score += (color == Color::WHITE ? checks_constant : -checks_constant);
            }

            // Restricting king movement
            if (Helper::any(knight_attacks, BitOp::get_surrounding_bits(enemy_king)))
            {
                pins_and_checks_score += (color == Color::WHITE ? king_restriction_bonus : -king_restriction_bonus);
            }

            // Knight Movement bonus:
            knight_mobility += (knight_attacks & ~allied_pieces).count();
        }

        // Encourages Knights to not be in a position where they can be captured
        for (chess::Square sq : knight_positions)
        {
            std::unordered_map<PieceType, int> knight_is_attacked = Helper::isAttackedCount(data, sq, color == Color::WHITE ? Color::BLACK : Color::WHITE);
            std::unordered_map<PieceType, int> knight_support = Helper::isAttackedCount(data, sq, color);
            int allied_attackers_total = Helper::total_attackers(knight_support), enemy_attackers_total = Helper::total_attackers(knight_is_attacked);
            if (enemy_attackers_total && !allied_attackers_total)
            {
                knight_position_score -= (color == Color::WHITE ? 60 : -60);
                continue;
            }
            else if (knight_is_attacked[PieceType::PAWN])
            {
                // Knight can be taken by a pawn, so it is essentially lost
                knight_position_score -= (color == Color::WHITE ? 50 : -50); // Will be adjusted
                continue;
            }
            else if (enemy_attackers_total >= allied_attackers_total)
            {
                knight_position_score -= (enemy_attackers_total - allied_attackers_total) * 15; // Will be adjusted later.
                continue;
            }
        }
    }

    int rook_score()
    {
        int white_rook_open_file = 0, black_rook_open_file = 0;
        int white_stacked_rook = 0, black_stacked_rook = 0;
        int white_rook_mobility = 0, black_rook_mobility = 0;

        // White
        rook_eval(white_rooks, white_rook_open_file, white_stacked_rook, white_rook_mobility, black_king, Color::WHITE);

        // Black
        rook_eval(black_rooks, black_rook_open_file, black_stacked_rook, black_rook_mobility, white_king, Color::BLACK);
        return (white_rook_open_file - black_rook_open_file) * rook_open_file_bonus + (white_stacked_rook - black_stacked_rook) * stacked_rooks_bonus + (white_rook_mobility - black_rook_mobility) * rook_mobility_bonus;
    }

    void rook_eval(Bitboard rooks, int &rook_open_file, int &stacked_rook, int &rook_mobility, Bitboard enemy_king, Color color)
    {
        int rooks_count = rooks.count();
        if (rooks_count == 0)
            return; // No rooks, no point evaluating.
        std::vector<chess::Square> rook_location;
        rook_location.reserve(rooks_count);

        for (int index = 0; index < 8; ++index)
        {
            File file(static_cast<File::underlying>(index));
            Bitboard file_bb(file);
            Rank rank(static_cast<Rank::underlying>(index));
            Bitboard rank_bb(rank);

            // Check if rooks are doubled
            Bitboard rooks_in_file = rooks & file_bb;
            if (rooks_in_file.count() >= 2)
                stacked_rook++;
            Bitboard rooks_in_rank = rooks & rank_bb;
            if (rooks_in_rank.count() >= 2)
                stacked_rook++;

            // Locate the rook:
            while (rooks_in_file)
            {
                int square_index = rooks_in_file.lsb(); // Get lsb
                rook_location.push_back(chess::Square(square_index));
                // std::cout << color << " Rook Square index: " << std::to_string(square_index) << '\n';
                rook_position_score += (color == Color::WHITE ? white_rook_table[square_index] : -black_rook_table[square_index]);
                (void)rooks_in_file.pop(); // Remove lsb
            }

            // Determining if file is open:
            bool file_is_open = !(file_bb & black_pawns);
            if (file_is_open)
                ++rook_open_file;

            // Determining if rank is open:
            bool rank_is_open = !(rank_bb & black_pawns);
            if (rank_is_open)
                ++rook_open_file;
        }

        // Determine how mobile rooks are:
        for (int i = 0; i < rooks_count; ++i)
        {
            rook_mobility += (chess::attacks::rook(rook_location[i], all_pieces)).count();
        }

        for (int i = 0; i < rook_location.size(); i++)
        {
            Bitboard rook_attacks = chess::attacks::rook(rook_location[i], all_pieces);
            // Checks
            if (Helper::any(rook_attacks, enemy_king))
            {
                pins_and_checks_score += (color == Color::WHITE ? checks_constant : -checks_constant);
            }

            // Restricting king movement
            if (Helper::any(rook_attacks, BitOp::get_surrounding_bits(enemy_king)))
            {
                pins_and_checks_score += (color == Color::WHITE ? king_restriction_bonus : -king_restriction_bonus);
            }
        }

        // Encourages rooks to not be in a position where they can be captured
        for (chess::Square sq : rook_location)
        {
            std::unordered_map<PieceType, int> rook_is_attacked = Helper::isAttackedCount(data, sq, color == Color::WHITE ? Color::BLACK : Color::WHITE);
            std::unordered_map<PieceType, int> rook_support = Helper::isAttackedCount(data, sq, color);
            int allied_attackers_total = Helper::total_attackers(rook_support), enemy_attackers_total = Helper::total_attackers(rook_is_attacked);
            if (enemy_attackers_total && !allied_attackers_total)
            {
                rook_position_score -= (color == Color::WHITE ? 125 : -125); // Will be adjusted
                continue;
            }
            else if (rook_is_attacked[PieceType::PAWN])
            {                                                                // Pawn can take rook. Almost always a bad exchange.
                rook_position_score -= (color == Color::WHITE ? 125 : -125); // Will be adjusted
                continue;
            }
            else if (rook_is_attacked[PieceType::KNIGHT] || rook_is_attacked[PieceType::BISHOP])
            {
                if (allied_attackers_total < enemy_attackers_total)
                { // Opposing side will win exchange, leading to loss of material
                    rook_position_score -= (color == Color::WHITE ? 50 : -50);
                    continue;
                }
            }
            else if (enemy_attackers_total >= allied_attackers_total)
            {
                rook_position_score -= (allied_attackers_total - enemy_attackers_total) * 15; // Will be adjusted later.
                continue;
            }
        }
    }

    int queen_score()
    {
        // White
        queen_eval(white_queens, black_king, black_pieces, Color::WHITE);

        // Black
        queen_eval(black_queens, white_king, white_pieces, Color::BLACK);
        return 0;
    }

    void queen_eval(Bitboard queens, Bitboard enemy_king, Bitboard enemy_pieces, Color color)
    {
        int queens_count = queens.count();
        int enemy_pieces_count = enemy_pieces.count();
        if (queens_count == 0)
            return; // No queens, no point evaluating.
        std::vector<chess::Square> queen_location;
        queen_location.reserve(queens_count);

        for (int index = 0; index < 8; ++index)
        {
            File file(static_cast<File::underlying>(index));
            Bitboard file_bb(file);
            Rank rank(static_cast<Rank::underlying>(index));
            Bitboard rank_bb(rank);

            Bitboard queens_in_file = queens & file_bb;
            while (queens_in_file)
            {
                int square_index = queens_in_file.lsb(); // Get least significant bit index
                queen_location.push_back(chess::Square(square_index));
                if (enemy_pieces_count > 10)
                {
                    queen_position_score += (color == Color::WHITE ? early_white_queen_table[square_index] : -early_black_queen_table[square_index]);
                }
                else
                {
                    queen_position_score += (color == Color::WHITE ? late_white_queen_table[square_index] : -late_black_queen_table[square_index]);
                }
                (void)queens_in_file.pop(); // Remove the least significant bit
            }
        }
        // std::cout << color << " Queen Square index: " << std::to_string(square_index) << '\n';

        for (int i = 0; i < queen_location.size(); i++)
        {
            Bitboard queen_attacks = chess::attacks::queen(queen_location[i], all_pieces);
            if (enemy_pieces_count < 11)
            {
                // Checks
                if (Helper::any(queen_attacks, enemy_king))
                {
                    pins_and_checks_score += (color == Color::WHITE ? checks_constant : -checks_constant);
                }
            }

            // Restricting king movement
            if (Helper::any(queen_attacks, BitOp::get_surrounding_bits(enemy_king)))
            {
                pins_and_checks_score += (color == Color::WHITE ? king_restriction_bonus : -king_restriction_bonus);
            }
        }
    }

    int king_score()
    {
        // White
        king_eval(Color::WHITE);

        // Black
        king_eval(Color::BLACK);
        return 0;
    }

    void king_eval(Color color)
    {
        chess::Square king_square = data.kingSq(color);
        int king_attackers = Helper::total_attackers(Helper::isAttackedCount(data, king_square, color == Color::WHITE ? Color::BLACK : Color::WHITE));
        if (king_attackers >= 2)
        {
            king_position_score -= (color == Color::WHITE ? 300 : -300); // Will be adjusted. Tries to prevent double checks.
        }
    }

    int const sum_pos()
    {
        return pawn_position_score + bishop_position_score + knight_position_score + rook_position_score + queen_position_score + king_position_score;
    }

    int static_eval()
    {
        int temp;
        switch (data.isGameOver().second)
        {
        case chess::GameResult::NONE:
            temp = naive_material_balance() + pawn_score() + bishop_score() + knight_score() + rook_score() + queen_score() + king_score();
            return temp + sum_pos() + pins_and_checks_score;
        case chess::GameResult::WIN:
            return side == Color::BLACK ? -99999 : 99999;
        case chess::GameResult::LOSE:
            return side == Color::BLACK ? 99999 : -99999;
        case chess::GameResult::DRAW:
            return 0;
        default:
            return 0;
        }
    }
};

/*
int main() {
    // chess::Board board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    chess::Board Doubled_pawns_board3("rn2k2r/p1pp1p1p/1p1bp3/5b1n/2B2q2/1PN1BP1N/P1PPQ1PP/R2K3R b kq - 0 1");
    Evaluation evo(Doubled_pawns_board3);
    std::cout << evo.static_eval() << '\n';
    return 0;
}
*/
#endif

/*
std::cout << "Pawn Score: " << std::to_string(pawn_score()) << "\n";
                std::cout << "Bishop Score: " << std::to_string(bishop_score()) << "\n";
                std::cout << "Knight Score: " << std::to_string(knight_score()) << "\n";
                std::cout << "Rook Score: " << std::to_string(rook_score()) << "\n";
                std::cout << "Queen Score: " << std::to_string(queen_score()) << "\n";
*/
