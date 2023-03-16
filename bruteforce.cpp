#include <iostream>
#include <fstream>
#include <string>
#include <chrono>

void init_board(int board[9][9], std::string filename);
void copy_board(int source[9][9], int dest[9][9]);
bool recursive_bruteforce(int board[9][9]);
bool valid_placement(int board[9][9], int row, int col, int num);
void print_board(int board[9][9]);

int main(int argc, char **argv) {
    int board[9][9];
    init_board(board, argv[1]);

    auto start = std::chrono::high_resolution_clock::now();
    if(recursive_bruteforce(board)) {
        std::cout << "Solution found:\n";
    } else {
        std::cout << "Failed to find solution\n";
    }
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    std::cout << "(took " << duration.count() << " us)\n";

    print_board(board);
}

void init_board(int board[9][9], std::string filename) {
    for(int row=0; row<9; row++)
        for(int col=0; col<9; col++)
            board[row][col] = 0;
    
    std::fstream file;
    file.open(filename);

    std::string line;
    int row=0;
    int col=0;
    while(file >> line) {
        col = 0;
        for(char c : line) {
            if(isdigit(c)) {
                board[row][col] = c-'0';
            }
            col++;
            if(col > 8) break;
        }
        row++;
        if(row > 8) break;
    }

    file.close();
}

void copy_board(int source[9][9], int dest[9][9]) {
    for(int row=0; row<9; row++)
        for(int col=0; col<9; col++)
            dest[row][col] = source[row][col];
}

bool recursive_bruteforce(int board[9][9]) {
    for(int row=0; row<9; row++)
        for(int col=0; col<9; col++) {
            if(board[row][col] != 0)
                continue;
            for(int num=1; num<=9; num++) {
                if(!valid_placement(board, row, col, num))
                    continue;
                int new_board[9][9];
                copy_board(board, new_board);
                new_board[row][col] = num;
                if(recursive_bruteforce(new_board)) {
                    copy_board(new_board, board);
                    return true;
                }
            }
            return false;
        }
    return true;
}

bool valid_placement(int board[9][9], int row, int col, int num) {
    for(int r=0; r<9; r++)
        if(board[r][col] == num)
            return false;
    
    for(int c=0; c<9; c++)
        if(board[row][c] == num)
            return false;
    
    for(int r=0; r<3; r++)
        for(int c=0; c<3; c++)
            if(board[(row/3)*3+r][(col/3)*3+c] == num)
                return false;

    return true;
}

void print_board(int board[9][9]) {
    for(int row=0; row<9; row++) {
        for(int col=0; col<9; col++) {
            std::cout << (char)(board[row][col]+'0');
        }
        std::cout << "\n";
    }
}