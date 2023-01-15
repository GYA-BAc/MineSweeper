#include <iostream>
#include <string>

#include <Windows.h>

#include <random>
#include <time.h>

#define BOARD_W 30
#define BOARD_H 30

#define BLANK  ' '
#define MINE   '@'
#define HIDDEN '.'
#define MARKED (unsigned char) 254 //this ■


struct Tile {
    char display;
    bool visible;
    bool marked;
};


struct Point {
    int y;
    int x;
};

const Point SURROUNDING[8] = {
    {-1, -1}, {-1, 0}, {-1, 1},
    { 0, -1},          { 0, 1},
    { 1, -1}, { 1, 0}, { 1, 1},
};




void init_board(Tile (*board)[BOARD_H][BOARD_W], Tile filler) {
    for (int i=0; i<BOARD_H; i++) {
        for (int j=0; j<BOARD_W; j++) { 
            (*board)[i][j] = filler;
        }
    }
}


bool within_board_bounds(int y, int x) {
    return (
        y >= 0      &&
        y < BOARD_H &&
        x >= 0      &&
        x < BOARD_W
    );
}


int num_marked(Tile (*board)[BOARD_H][BOARD_H]) {

    int count = 0;
    for (int i=0; i<BOARD_H; i++) {
        for (int j=0; j<BOARD_W; j++) { 
            if ((*board)[i][j].marked) count++;
        }
    }
    return count;
}


bool is_win(Tile (*board)[BOARD_H][BOARD_H], int n_mines) {

    int revealed = 0;
    for (int i=0; i<BOARD_H; i++) {
        for (int j=0; j<BOARD_W; j++) {

            if ((*board)[i][j].visible) 
                revealed++;

            if ((*board)[i][j].display == MINE && !(*board)[i][j].marked) 
                return false;
            
        }
    }

    return revealed == BOARD_W*BOARD_H-n_mines;
    
}


void place_mines(Tile (*board)[BOARD_H][BOARD_W], Point current_cursor, int num_mines) {

    std::srand(time(0)); //seed random

    int mines_placed = 0;
    Point rand_coord;

    //place mines
    while (mines_placed < num_mines) {
        //generate random coordinate
        rand_coord.y = rand() % BOARD_H;
        rand_coord.x = rand() % BOARD_W;

        //check if there is already a mine there
        if ((*board)[rand_coord.y][rand_coord.x].display == MINE) {
            continue;
        }

        //ensure player's first reveal is not a mine
        if (
            abs(current_cursor.x-rand_coord.x) <= 2 &&
            abs(current_cursor.y-rand_coord.y) <= 2
        ) {
            continue;
        }

        (*board)[rand_coord.y][rand_coord.x].display = MINE;

        mines_placed++;
    }

    //set numbers around mines
    int bordering_mines;
    for (int i=0; i<BOARD_H; i++) {
        for (int j=0; j<BOARD_W; j++) {
            
            //count bordering mines
            bordering_mines = 0;
            for (int check=0; check<8; check++) {
                //ensure that no out-of-bounds checking occurs
                if (!within_board_bounds(i+SURROUNDING[check].y, j+SURROUNDING[check].x))
                    continue;

                if ((*board)[i+SURROUNDING[check].y][j+SURROUNDING[check].x].display == MINE)
                    bordering_mines++;
            }
            
            if (bordering_mines > 0 && (*board)[i][j].display != MINE) {
                (*board)[i][j].display = (char) (bordering_mines + 48); //add 48 to get to ascii numbers
            }
        }
    }
}


bool reveal_point(Tile (*board)[BOARD_H][BOARD_W], Point pnt) {
    //returns if a mine was hit

    if ((*board)[pnt.y][pnt.x].visible) {
        return false;
    } else {
        (*board)[pnt.y][pnt.x].visible = true;
    }

    //unmark revealed tiles
    if ((*board)[pnt.y][pnt.x].marked) 
        (*board)[pnt.y][pnt.x].marked = false;

    //check for mine
    if ((*board)[pnt.y][pnt.x].display == MINE) 
        return true;

    if (!((*board)[pnt.y][pnt.x].display == BLANK))
        return false;

    // if revealed square was Blank, reveal the surrounding squares recursively 
    for (int check=0; check<8; check++) {
        if (!within_board_bounds(pnt.y+SURROUNDING[check].y, pnt.x+SURROUNDING[check].x))
            continue;
        reveal_point(board, Point {pnt.y+SURROUNDING[check].y, pnt.x+SURROUNDING[check].x});
    }

    return false;
}


void print_board(Tile (*board)[BOARD_H][BOARD_W], Point cursor, bool reveal_all) {

    std::string strboard = "";
    bool is_cursor;
    bool visible;
    bool marked;
    bool is_blank;

    for (int i=0; i<BOARD_H; i++) {
        for (int j=0; j<BOARD_W; j++) {
            is_cursor = (i == cursor.y && j == cursor.x);
            visible = (*board)[i][j].visible;
            marked = (*board)[i][j].marked;

            if (!reveal_all) {
                strboard += is_cursor ? '>' : ' ';
                strboard += visible ? (*board)[i][j].display : (marked ? MARKED : HIDDEN);
                strboard += is_cursor ? '<' : ' ';
            } else {
                is_blank = (*board)[i][j].display == BLANK;
                strboard += is_cursor ? '>' : (visible||is_blank ? ' ' : (marked ? '[' : (char) 219));
                strboard += (*board)[i][j].display;
                strboard += is_cursor ? '<' : (visible||is_blank ? ' ' : (marked ? ']' : (char) 219));
            }
        }
        strboard += '\n';
    }
    std::cout << strboard;
}


void print_board_info(Tile (*board)[BOARD_H][BOARD_W], int total_mines) {
    std::string strinfo = "";
    strinfo += std::string((int) BOARD_W*3/2-5, '=') + "Info:" + std::string((int) BOARD_W*3/2, '=');
    strinfo += "\nWASD/Arrows to move, Space to reveal, X to mark, Esc to quit: ";
    strinfo += "\nMark every mine and reveal every non-mine tile to win!";
    strinfo += "\n    Board Size: " + std::to_string(BOARD_W) + 'x' + std::to_string(BOARD_H);
    strinfo += "\n    Mines: " + std::to_string(total_mines);
    strinfo += "\n    Marked: " + std::to_string(num_marked(board));
    std::cout << strinfo;
}


int main() {

    // application variables
    bool running = true;
    bool needs_redraw = true;
    bool first_reveal = true;
    bool win = false;

    const int NUM_MINES = (int) BOARD_W*BOARD_H/5;

    // create board
    Tile board[BOARD_H][BOARD_W];

    init_board(&board, Tile {BLANK, false, false});

    Point cursor = {0, 0};
    
    //resize console window
    char cmd_buf[128];
    std::snprintf(cmd_buf, 128, "mode con: cols=%d lines=%d", BOARD_W*3, BOARD_H+8);
    system(cmd_buf);

    //mainloop
    while (running) {

        if (GetKeyState(VK_ESCAPE) & 0x8000 /*check higher order bit apparently*/) 
            running = false;

        if ((GetKeyState(VK_UP) & 0x8000)||GetKeyState('W') & 0x8000) {
            if (within_board_bounds(cursor.y-1, cursor.x))
                cursor.y--;
                needs_redraw = true;
        }
        if ((GetKeyState(VK_LEFT) & 0x8000)||GetKeyState('A') & 0x8000) {
            if (within_board_bounds(cursor.y, cursor.x-1))
                cursor.x--;
                needs_redraw = true;
        }
        if ((GetKeyState(VK_DOWN) & 0x8000)||GetKeyState('S') & 0x8000) {
            if (within_board_bounds(cursor.y+1, cursor.x))
                cursor.y++;
                needs_redraw = true;
        }
        if ((GetKeyState(VK_RIGHT) & 0x8000)||GetKeyState('D') & 0x8000) {
            if (within_board_bounds(cursor.y, cursor.x+1))
                cursor.x++;
                needs_redraw = true;
        }
        if ((GetKeyState(VK_SPACE) & 0x8000)) {
            if (first_reveal) {
                first_reveal = false;
                place_mines(&board, cursor, NUM_MINES);
            }
            if (!board[cursor.y][cursor.x].marked) {
                running = !reveal_point(&board, cursor);
                needs_redraw = true;
            }
        }
        if (GetKeyState('X') & 0x8000) {
            if (!board[cursor.y][cursor.x].visible) {
                board[cursor.y][cursor.x].marked = !board[cursor.y][cursor.x].marked;
                needs_redraw = true;
            }
        }

        if (is_win(&board, NUM_MINES)) {
            win = true;
            running = false;
        }

        if (needs_redraw) {
            system("cls"); // clear screen
            print_board(&board, cursor, !running); //reveal entire board if game ending
            if (running)
                print_board_info(&board, NUM_MINES);
            needs_redraw = false;
        }
        Sleep(80);
    }

    std::cout << '\n';
    std::cout << (win ? "You Won!" : "KABOOM!");
    std::cout << "\nPress Any Key to Quit";
    std::cin.get();

    return 0; 
}