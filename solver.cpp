#include <iostream>
#include <fstream>
#include <string>
#include <chrono>

typedef struct {
    int row;
    int col;
} Coord;

enum SetNextResult {
    success_set,
    success_no_set,
    fail_solution_found,
    fail_all_0,
    fail_stuck,
};

int set_bit_index(uint16_t bits);

class Sudoku {
public:
    uint16_t possible[9][9];
    uint16_t filled[9][9];
    int row_counts[9][9]; //[row, num]
    int col_counts[9][9]; //[col, num]
    int box_counts[9][9]; //[box, num]
    int filled_count;

    Sudoku(std::string filename) {
        filled_count = 0;
        //init possible and filled arrays
        for(size_t row=0; row<9; row++) {
            for(size_t col=0; col<9; col++) {
                possible[row][col] = 0b111111111;
                filled[row][col] = 0;
                row_counts[row][col] = 9;
                col_counts[row][col] = 9;
                box_counts[row][col] = 9;
            }
        }
        
        std::fstream file;
        file.open(filename);

        std::string line;
        Coord index = {0,0};
        while(file >> line) {
            index.col = 0;
            for(char c : line) {
                if(isdigit(c)) {
                    add_number(c-'0', index);
                }
                index.col++;
                if(index.col > 8) break;
            }
            index.row++;
            if(index.row > 8) break;
        }

        file.close();
    }

    Sudoku(const Sudoku& sin) {
        filled_count = sin.filled_count;
        //init possible and filled arrays
        for(size_t row=0; row<9; row++) {
            for(size_t col=0; col<9; col++) {
                possible[row][col] = sin.possible[row][col];
                filled[row][col] = sin.filled[row][col];
                row_counts[row][col] = sin.row_counts[row][col];
                col_counts[row][col] = sin.col_counts[row][col];
                box_counts[row][col] = sin.box_counts[row][col];
            }
        }
    }

    void add_number(int num, Coord c) {
        filled_count++;
        //bit mask
        uint16_t mask = 1<<(num-1);

        //set filled and possible
        filled[c.row][c.col] = mask;

        //run through possibilities and remove them one by one for filled cell
        for(int numm1=0; numm1<9; numm1++) {
            uint16_t mask = 1<<numm1;
            if(possible[c.row][c.col] & mask)
                sub_counts(numm1+1, c);
            possible[c.row][c.col] &= ~mask;
        }

        //set bits to 0 in row
        //if going to invert, sub from counts
        for(int row=0; row<9; row++) {
            if(possible[row][c.col] & mask)
                sub_counts(num, (Coord){row,c.col});
            possible[row][c.col] &= ~mask;
        }

        //set bits to 0 in col
        //if going to invert, sub from counts
        for(int col=0; col<9; col++) {
            if(possible[c.row][col] & mask)
                sub_counts(num, (Coord){c.row,col});
            possible[c.row][col] &= ~mask;
        }

        //set bits to 0 in box
        //if going to invert, sub from counts
        Coord box = {(c.row/3)*3, (c.col/3)*3}; //box corner
        for(int row=0; row<3; row++)
            for(int col=0; col<3; col++) {
                if(possible[box.row+row][box.col+col] & mask)
                    sub_counts(num, (Coord){box.row+row,box.col+col});
                possible[box.row+row][box.col+col] &= ~mask;
            }
    }

    void sub_counts(int num, Coord c) {
        //sub one from counts
        row_counts[c.row][num-1]--;
        col_counts[c.col][num-1]--;
        box_counts[(c.row/3)*3 + c.col/3][num-1]--;
        // if(num == 2) std::cout << "box " << (c.row/3)*3 + c.col/3 << " --\n";
    }

    SetNextResult set_next() {
        //returns coord of next single possibility
        //single possibility in block checker
        for(int num=1; num<=9; num++) {
            //calculate mask
            uint16_t mask = 1<<(num-1);

            //check for any rows with one instance of number possibility
            for(int row=0; row<9; row++)
                if(row_counts[row][num-1] == 1)
                    for(int col=0; col<9; col++)
                        if(possible[row][col] & mask) {
                            add_number(num, (Coord){row,col});
                            return success_set;
                        }
            
            //check for any cols with one instance of number possibility
            for(int col=0; col<9; col++)
                if(col_counts[col][num-1] == 1)
                    for(int row=0; row<9; row++)
                        if(possible[row][col] & mask) {
                            add_number(num, (Coord){row,col});
                            return success_set;
                        }

            //check for any boxes with one instance of number possibility
            for(int box=0; box<9; box++)
                if(box_counts[box][num-1] == 1) {
                    int box_corner_r = (box/3)*3;
                    int box_corner_c = (box%3)*3;
                    for(int row=0; row<3; row++)
                        for(int col=0; col<3; col++)
                            if(possible[box_corner_r+row][box_corner_c+col] & mask) {
                                add_number(num, (Coord){box_corner_r+row,box_corner_c+col});
                                return success_set;
                            }
                }
        }

        bool all_zero = true;

        //single possibility in cell checker
        for(int row=0; row<9; row++)
            for(int col=0; col<9; col++) {
                //check for any cells with one possibility
                uint16_t pos = possible[row][col];
                if(pos != 0) {
                    all_zero = false;
                    if((pos & -pos) == pos) {
                        set_from_possible((Coord){row,col});
                        return success_set;
                    }
                }
            }
        
        if(all_zero) {
            if(filled_count == 9*9)
                return fail_solution_found;
            return fail_all_0;
        }
        

        //return fail_stuck;

        //no simple next move, check for more complex blocking, much more expensive
        /*
        if block contains (all instances of num in another block), remove from rest of block
        only works for (row/col) and box, but works both ways
        only works if containing block has more instances than other block, and 1 < other block instances <= 3
        */

        bool changed = false;
        for(int numm1=0; numm1<9; numm1++) {
            uint16_t mask = 1<<numm1;
            for(int box=0; box<9; box++) {
                //check rows
                for(int row_offset=0; row_offset<3; row_offset++) {
                    int row = (box/3)*3 + row_offset;

                    //row is containing:
                    if(row_counts[row][numm1] > box_counts[box][numm1] && box_counts[box][numm1] > 1 && box_counts[box][numm1] <= 3) {
                        //check if all counts occur in overlap:
                        //count counts in overlap
                        int counts = 0;
                        for(int col_offset=0; col_offset<3; col_offset++) {
                            int col = (box%3)*3 + col_offset;
                            counts += !!(possible[row][col] & mask);
                        }
                        //if counts in overlap == other counts
                        if(counts == box_counts[box][numm1]) {
                            for(int col=0; col<9; col++) {
                                if(col/3 != box%3) {
                                    if(possible[row][col] & mask) {
                                        possible[row][col] &= ~mask;
                                        sub_counts(numm1+1, (Coord){row,col});
                                        changed = true;
                                    }
                                }
                            }
                        }
                    }

                    //box is containing:
                    if(box_counts[box][numm1] > row_counts[row][numm1] && row_counts[row][numm1] > 1 && row_counts[row][numm1] <= 3) {
                        //check if all counts occur in overlap:
                        //count counts in overlap
                        int counts = 0;
                        for(int col_offset=0; col_offset<3; col_offset++) {
                            int col = (box%3)*3 + col_offset;
                            counts += !!(possible[row][col] & mask);
                        }
                        //if counts in overlap == other counts
                        if(counts == row_counts[row][numm1]) {
                            for(int col_offset=0; col_offset<3; col_offset++)
                                for(int r_offset=0; r_offset<3; r_offset++) {
                                    int c = (box%3)*3 + col_offset;
                                    int r = (box/3)*3 + r_offset;
                                    if(r != row) {
                                        if(possible[r][c] & mask) {
                                            possible[r][c] &= ~mask;
                                            sub_counts(numm1+1, (Coord){r,c});
                                            changed = true;
                                        }
                                    }
                                }
                        }
                    }
                } 

                //check cols
                for(int col_offset=0; col_offset<3; col_offset++) {
                    int col = (box%3)*3 + col_offset;
                    //col is containing:
                    if(col_counts[col][numm1] > box_counts[box][numm1] && box_counts[box][numm1] > 1 && box_counts[box][numm1] <= 3) {
                        //check if all counts occur in overlap:
                        //count counts in overlap
                        int counts = 0;
                        for(int row_offset=0; row_offset<3; row_offset++) {
                            int row = (box/3)*3 + row_offset;
                            counts += !!(possible[row][col] & mask);
                        }
                        //if counts in overlap == other counts
                        if(counts == box_counts[box][numm1]) {
                            for(int row=0; row<9; row++) {
                                if(row/3 != box/3) {
                                    if(possible[row][col] & mask) {
                                        possible[row][col] &= ~mask;
                                        sub_counts(numm1+1, (Coord){row,col});
                                        changed = true;
                                    }
                                }
                            }
                        }
                    }
                    //box is containing:
                    if(box_counts[box][numm1] > col_counts[col][numm1] && col_counts[col][numm1] > 1 && col_counts[col][numm1] <= 3) {
                        //check if all counts occur in overlap:
                        //count counts in overlap
                        int counts = 0;
                        for(int row_offset=0; row_offset<3; row_offset++) {
                            int row = (box/3)*3 + row_offset;
                            counts += !!(possible[row][col] & mask);
                        }
                        //if counts in overlap == other counts
                        if(counts == col_counts[col][numm1]) {
                            for(int row_offset=0; row_offset<3; row_offset++)
                                for(int c_offset=0; c_offset<3; c_offset++) {
                                    int c = (box%3)*3 + c_offset;
                                    int r = (box/3)*3 + row_offset;
                                    if(c != col) {
                                        if(possible[r][c] & mask) {
                                            possible[r][c] &= ~mask;
                                            sub_counts(numm1+1, (Coord){r,c});
                                            changed = true;
                                        }
                                    }
                                }
                        }
                    }
                } 
            }
        }
        //if found complex blocking
        if(changed)
            return success_no_set;

        return fail_stuck;
    }

    void cout_filled() {
        char buf[91];
        int bufi = 0;
        for(size_t row=0; row<9; row++) {
            for(size_t col=0; col<9; col++) {
                int index = set_bit_index(filled[row][col]);
                buf[bufi] = (char)(index!=-1 ? index+'1' : '.'); bufi++;
            }
            buf[bufi] = '\n'; bufi++;
        }
        buf[bufi] = '\0';
        std::cout << buf;
    }

    void set_from_possible(Coord c) {
        add_number(set_bit_index(possible[c.row][c.col])+1, c);
    }
};

bool solve(Sudoku sudoku);
bool solve_no_output(Sudoku sudoku);

int main(int argc, char** argv) {
    if(argc < 2) {
        std::cout << "File name expected!\n";
        return 1;
    }
    std::string filename(argv[1]);
    
    Sudoku sudoku(filename);

    auto start = std::chrono::high_resolution_clock::now();
    if(!solve(sudoku)) {
        std::cout << "Got stuck! Cannot solve:\n";
        sudoku.cout_filled();
    }    
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    std::cout << "(took " << duration.count() << " us)\n";

    // for(int i=0; i<10; i++) {
    //     Sudoku sudoku(filename);

    //     // sudoku.cout_filled();
        
    //     auto start = std::chrono::high_resolution_clock::now();
    //     bool result = solve_no_output(sudoku);
    //     auto stop = std::chrono::high_resolution_clock::now();
    //     auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    //     std::cout << "run " << i << ": " << duration.count() << " us\n";

    //     if(!result) {
    //         std::cout << "Got stuck! Cannot solve:\n";
    //         sudoku.cout_filled();
    //     }
    // }
        
        return 0;
}

bool solve(Sudoku sudoku) {
    bool run = true;
    while(run) {
        SetNextResult next = sudoku.set_next();
        //implement guess stack
        //or recursive algorithm that 
        switch(next) {
        case fail_solution_found:
            std::cout << "Solution found!\n";
            sudoku.cout_filled();
            return true;
        case fail_all_0:
            //std::cout << "Got stuck! no more possible numbers:\n";
            //sudoku.cout_filled();
            return false;
        case fail_stuck:
            // std::cout << "Got stuck! Taking a guess\n";
            // sudoku.cout_filled();
            //begin guessing
            run = 0;
        }
    }

    //guess
    for(int row=0; row<9; row++) {
        for(int col=0; col<9; col++) {
            for(int numm1=0; numm1<9; numm1++) {
                if(!(sudoku.possible[row][col] & 1<<numm1))
                    continue;
                Sudoku sudoku_guess(sudoku);
                sudoku_guess.add_number(numm1+1, (Coord){row,col});
                if(solve(sudoku_guess))
                    return true;
            }
        }
    }

    return false;
}

bool solve_no_output(Sudoku sudoku) {
    bool run = true;
    while(run) {
        SetNextResult next = sudoku.set_next();
        //implement guess stack
        //or recursive algorithm that 
        switch(next) {
        case fail_solution_found:
            return true;
        case fail_all_0:
            //std::cout << "Got stuck! no more possible numbers:\n";
            //sudoku.cout_filled();
            return false;
        case fail_stuck:
            //sudoku.cout_filled();
            //begin guessing
            run = 0;
        }
    }

    //guess
    for(int row=0; row<9; row++) {
        for(int col=0; col<9; col++) {
            for(int numm1=0; numm1<9; numm1++) {
                if(!(sudoku.possible[row][col] & 1<<numm1))
                    continue;
                Sudoku sudoku_guess(sudoku);
                sudoku_guess.add_number(numm1+1, (Coord){row,col});
                if(solve_no_output(sudoku_guess))
                    return true;
            }
        }
    }

    return false;
}

int set_bit_index(uint16_t bits) {
    for(size_t i=0; i<16; i++) {
        if(bits & (1<<i)) return i;
    }
    return -1;
}