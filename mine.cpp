#include <iostream>
#include <string>
#include <random>
#include <time.h>

#include <Windows.h>


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
        if (within_board_bounds(pnt.y+SURROUNDING[check].y, pnt.x+SURROUNDING[check].x))
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


void start_game(int difficulty) {

    // application variables
    bool running = true;
    bool needs_redraw = true;
    bool first_reveal = true;
    bool win = false;

    bool escaped = false;

    int NUM_MINES;
    switch (difficulty) {
        case 0: // easy
            NUM_MINES = (int) BOARD_W*BOARD_H/10;
            break;

        case 1: // medium
            NUM_MINES = (int) BOARD_W*BOARD_H/7;
            break;

        case 2: // hard
            NUM_MINES = (int) BOARD_W*BOARD_H/5;
            break;
    }

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

        if (GetKeyState(VK_ESCAPE) & 0x8000 /*check higher order bit apparently*/) {
            escaped = true;
            running = false;
        }

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

    if (!escaped) {
        std::cout << '\n';
        std::cout << (win ? "You Won!\n" : "KABOOM!\n");
        std::cout << '\n';

        system("pause");
    }

    system("mode con: cols=100 lines=40"); // reset screen size

}


#define INCREMENT(num, limit) (num<limit) ? num++ : num=0
#define DECREMENT(num, max) (num>0) ? num-- : num=max


int select_difficulty(int current) {
    int selected = current;

    bool refresh;
    bool init = true;

    while (1) {
        refresh = true;

        if ((GetKeyState(VK_SPACE) & 0x8000)) {
            return selected;
        }
        else if ((GetKeyState(VK_LEFT) & 0x8000)||GetKeyState('A') & 0x8000) {
            DECREMENT(selected, 2);
        }
        else if ((GetKeyState(VK_RIGHT) & 0x8000)||GetKeyState('D') & 0x8000) {
            INCREMENT(selected, 2);
        }
        else {
            refresh = false;
        }

        if (refresh || init){
            system("cls");
            init = false;

            std::cout << "< Easy : Normal : Hard >\n";
            switch (selected) {
                case (0):
                    std::cout << "   /\\ \n";
                    break;

                case (1):
                    std::cout << "           /\\ \n";
                    break;

                case (2):
                    std::cout << "                   /\\ \n";
                    break;
            }

            std::cout << "A/D or LEFT/RIGHT to adjust, SPACE to confirm";
            
            Sleep(80);
        }
        
    }
}


int main() {

    system("mode con: cols=100 lines=40");

    bool running = true;

    short selected_diff = 1; // difficulty scales from 0 to 2 (easy -> hard)
    short selected_option = 0;

    bool refresh;
    bool init = true; // used to draw the first frame

    while (running) {

        refresh = true;

        if ((GetKeyState(VK_RETURN) & 0x8000)) {

            switch (selected_option) {
                case (0): // play
                    start_game(selected_diff);
                    break;

                case(1): // options
                    selected_diff = select_difficulty(selected_diff);
                    break;

                case(2): // about
                    system("cls");
                    std::cout << "MineSweeper, implemented in C++ by Alan Ji\n\n";
                    system("pause");
                    break;

                case(3): // quit
                    running = false;
                    break;
            }
        }

        else if ((GetKeyState(VK_UP) & 0x8000)||GetKeyState('W') & 0x8000) {
            DECREMENT(selected_option, 3);
        } 
        
        else if ((GetKeyState(VK_DOWN) & 0x8000)||GetKeyState('S') & 0x8000) {
            INCREMENT(selected_option, 3);
        }

        else {
            refresh = false;
        }

        //draw menu
        if (refresh || init) {
            init = false;
            system("cls");

            std::cout << "\n\n";
            std::cout << 
                "   __   __  ___   __    _  _______  _______  _     _  _______  _______  _______  _______  ______   \n" <<
                "  |  |_|  ||   | |  |  | ||       ||       || | _ | ||       ||       ||       ||       ||    _ |  \n" <<
                "  |       ||   | |   |_| ||    ___||  _____|| || || ||    ___||    ___||    _  ||    ___||   | ||  \n" <<
                "  |       ||   | |       ||   |___ | |_____ |       ||   |___ |   |___ |   |_| ||   |___ |   |_||_ \n" <<
                "  |       ||   | |  _    ||    ___||_____  ||       ||    ___||    ___||    ___||    ___||    __  |\n" <<
                "  | ||_|| ||   | | | |   ||   |___  _____| ||   _   ||   |___ |   |___ |   |    |   |___ |   |  | |\n" <<
                "  |_|   |_||___| |_|  |__||_______||_______||__| |__||_______||_______||___|    |_______||___|  |_|\n\n";

            std::cout << ((selected_option == 0) ? ">  " : " ") << "Play\n";

            std::cout << ((selected_option == 1) ? ">  " : " ") << "Options\n";

            std::cout << ((selected_option == 2) ? ">  " : " ") << "About\n";

            std::cout << ((selected_option == 3) ? ">  " : " ") << "Quit\n\n";

            std::cout << "W/S or UP/DOWN to navigate, ENTER to select";

        }
        Sleep(80);
    }

    return 0;
}