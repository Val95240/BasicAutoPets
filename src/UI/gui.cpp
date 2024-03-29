
#include "UI/gui.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#include <locale.h>
#include <ncurses.h>

#include "utils.hpp"


const std::unordered_map<UIState, std::string> MSGS = {
    {UIState::none, "What do you want to do ?     (B)uy     (S)ell     (F)reeze     (C)ombine team     (R)oll     (O)rder     (E)nd turn     (Q)uit"},
    {UIState::buy, "Buying !"},
    {UIState::sell, "Selling !"},
    {UIState::freeze, "Freezing !"},
    {UIState::combine, "Combining pets !"},
    {UIState::order, "Ordering !"},
    {UIState::fighting, "Fight !     (A)uto-play     (P)lay     (S)kip"},
};


GUI::GUI(Game* game) : UserInterface(game), m_state(UIState::none) {
    setlocale(LC_ALL, "");
    initscr();
    curs_set(0);
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    m_game->cheat();
}

GUI::~GUI() {
    endwin();
    std::cout << "Deleting GUI" << std::endl;
}

bool GUI::run() {
    do {
        clear();
        display_game();
    } while (act());

    return play_again();
}

void GUI::display_game() const {
    disp_frame();
    disp_game_state();
    disp_team();
    disp_shop();
    disp_action();
    disp_status();
    disp_logs();
}


bool GUI::play_again() const {
    disp_play_again();
    while (true) {
        int c = std::tolower(getch());
        if (c == 'y')
            return true;
        else if (c == 'n' || c == 10)
            return false;

        disp_play_again(true);
    }
}

bool GUI::act() {
    bool continue_game = true;
    try {
        switch (m_state) {
            case UIState::none:
                return take_action();
            case UIState::buy:
                buy();
                break;
            case UIState::sell:
                sell();
                break;
            case UIState::freeze:
                freeze();
                break;
            case UIState::combine:
                combine();
                break;
            case UIState::order:
                move();
                break;
            case UIState::fighting:
                fight();
                continue_game = !m_game->is_over();
                break;
        }
    } catch (InvalidAction& e) {
        m_status = e.what_str();
    }
    return continue_game;
}

bool GUI::take_action() {
    int c = std::tolower(getch());
    m_status = "";

    switch (c) {
        case KEY_RESIZE:
            clear();
            display_game();
            break;

        case 'b':
            m_state = UIState::buy;
            break;

        case 's':
            m_state = UIState::sell;
            break;

        case 'f':
            m_state = UIState::freeze;
            break;

        case 'c':
            m_state = UIState::combine;
            break;

        case 'r':
            m_status = "Rolling...";
            m_game->roll();
            break;

        case 'o':
            m_state = UIState::order;
            break;

        case 'e':
            m_state = UIState::fighting;
            m_game->end_turn();
            disp_team();
            utils::vector_logs.push_back("Press a key to continue...");
            disp_logs();
            getch();
            break;

        case 'd':
            m_game->save_state();
            m_status = "Current game state saved !";
            disp_status();
            break;

        case 'l':
            m_game->load_state();
            m_status = "Loading saved game state";
            disp_status();
            disp_logs(false);
            refresh();
            break;

        case 'q':
            return false;
    }

    return true;
}

void GUI::buy() {
    m_state = UIState::none;
    m_status = "[BUY]: Buying ...";
    disp_status();

    int c = std::tolower(getch());
    if ('1' <= c && c <= '7')
        buy_pet(c - '1');

    else if (c == '9' || c == '0')
        buy_object((c == '9' ? 0 : 1));
}

void GUI::sell() {
    m_state = UIState::none;
    m_status = "[SELL]: Selling pet ...";
    size_t c = std::tolower(getch());
    if (!('1' <= c && c <= '5')) {
        m_status = "[SELL]: Invalid pet index";
        return;
    }
    c = (c - '1');
    std::string pet_name = get_team_pet(c)->m_name;
    m_game->sell(c);

    if (!pet_name.empty())
        m_status = "[SELL]: Sold " + pet_name + " (index " + std::to_string(c+1) + ")";
}

void GUI::freeze() {
    m_state = UIState::none;
    int c = std::tolower(getch());

    if ('1' <= c && c <= '5') {
        m_game->freeze_pet(c - '1');
        m_status = "[FREEZE]: Frozen pet " + std::to_string(c - '0');
    } else if (c == '9' || c == '0') {
        m_game->freeze_object((c == '9' ? 0 : 1));
        m_status = "[FREEZE]: Frozen object " + std::to_string(c == '9' ? 1 : 2);
    }
}

void GUI::combine() {
    m_state = UIState::none;
    m_status = "[COMBINE]: Combining pets ... and ...";
    disp_status();

    int c1 = std::tolower(getch());
    if (!('1' <= c1 && c1 <= '5')) {
        m_status = "[COMBINE]: Invalid pet index";
        return;
    }
    m_status = "[COMBINE]: Combining pets " + std::to_string(c1 - '0') + " and ...";
    disp_status();

    int c2 = std::tolower(getch());
    if (!('1' <= c2 && c2 <= '5')) {
        m_status = "[COMBINE]: Invalid pet index";
        return;
    }
    m_status = "[COMBINE]: Combining pets " + std::to_string(c1 - '0') + " and " + std::to_string(c2 - '0');

    m_game->combine(c1 - '1', c2 - '1');
}

void GUI::move() {
    m_state = UIState::none;
    m_status = "[ORDER]: Switching ... and ...";
    disp_status();

    int c1 = std::tolower(getch());
    if (!('1' <= c1 && c1 <= '5')) {
        m_status = "[ORDER]: Invalid order index";
        return;
    }
    m_status = "[ORDER]: Switching " + std::to_string(c1 - '0') + " and ...";
    disp_status();

    int c2 = std::tolower(getch());
    if (!('1' <= c2 && c2 <= '5')) {
        m_status = "[ORDER]: Invalid order index";
        return;
    }
    m_status = "[ORDER]: Switching " + std::to_string(c1 - '0') + " and " + std::to_string(c2 - '0');

    m_game->move(c1 - '1', c2 - '1');
}

void GUI::fight() {
    clear();
    disp_frame();
    disp_action();

    m_game->start_fight();
    disp_fight();
    int c = get_fighting_action();
    while (m_game->fight_step()) {
        if (c == 's' || c == 'q')
            continue;

        disp_fight();
        disp_logs();
        if (c == 'a') {
            std::this_thread::sleep_for(std::chrono::milliseconds(750));
            refresh();
        } else if (c == 'p' || c == 'n') {
            c = get_fighting_action();
        }
    }

    disp_fight();
    if (c == 'a')
        std::this_thread::sleep_for(std::chrono::milliseconds(750));
    refresh();

    utils::vector_logs.push_back("Press a key to continue...");
    disp_logs();
    m_status = "";
    getch();

    m_game->end_fight();
    m_state = UIState::none;
}


void GUI::buy_pet(size_t index) {
    Pet const* pet = get_shop_pet(index);
    if (!pet) {
        m_status = "[BUY] No pet in shop at index " + std::to_string(index + 1);
        return;
    }

    std::string pet_name = pet->m_name;
    m_status = "[BUY]: Buying " + pet_name + " in position ...";
    disp_status();

    size_t target = std::tolower(getch());
    if (!('1' <= target && target <= '5')) {
        m_status = "[BUY]: Invalid dst index";
        return;
    }
    size_t target_index = target - '1';
    m_game->buy_pet(index, target_index);
    m_status = "[BUY]: Bought " + pet_name + " in position " + std::to_string(target_index + 1);
}

void GUI::buy_object(size_t index) {
    Object const* obj = get_shop_object(index, true);
    if (!obj) {
        m_status = "[BUY]: No object in shop at index " + std::to_string(index + 1);
        return;
    }

    std::string obj_name = obj->m_name;
    if (obj->m_target_all) {
        m_game->buy_object(index, 0);
        m_status = "[BUY]: Bought " + obj_name;
        return;
    }

    m_status = "[BUY]: Buying " + obj_name + " for pet ...";
    disp_status();

    size_t target = std::tolower(getch());
    if (!('1' <= target && target <= '5')) {
        m_status = "[BUY]: Invalid target for " + obj_name;
        return;
    }
    size_t target_index = target - '1';
    std::string target_name = get_team_pet(target_index)->m_name;

    m_game->buy_object(index, target_index);
    if (!target_name.empty())
        m_status = "[BUY]: Giving " + obj_name + " to " + target_name;
}

int GUI::get_fighting_action() {
    int c;
    bool invalid;
    std::unordered_set<int> valid_actions = {'a', 'p', 'n', 's', 'q'};
    do {
        c = std::tolower(getch());
        if (c == KEY_RESIZE) {
            clear();
            disp_frame();
            disp_action();
            disp_fight();
            disp_status();
            disp_logs(false);
            continue;
        }

        invalid = (valid_actions.count(c) == 0);
        if (invalid) {
            m_status = "Invalid action !";
            disp_status();
        }
    } while (invalid);
    m_status = "";
    disp_status();

    return c;
}


void GUI::disp_frame() const {
    std::string hborder(COLS-2, '-');
    hborder = "+" + hborder + "+";
    mvaddstr(0, 0, hborder.c_str());
    mvaddstr(LINES-1, 0, hborder.c_str());
    for (int j=1; j<LINES-1; j++) {
        mvaddch(j, 0, '|');
        mvaddch(j, COLS-1, '|');
    }
}

void GUI::disp_game_state() const {
    int padding = (COLS-1 - 4*9) / 4;
    int inner_padding = (COLS-1 - 4*9) / 6;

    mvprintw(3, padding, "Money : % 2d", get_money());
    mvprintw(3, padding+9+inner_padding, " Life: % 2d", get_life());
    mvprintw(3, padding+2*(9+inner_padding), " Wins: % 2d/10", get_victories());
    mvprintw(3, padding+3*(9+inner_padding), " Turn % 2d", get_turn());
}

void GUI::disp_pet(Pet const* pet, int x, int y, bool disp_xp, bool in_shop, bool frozen) const {
    std::string object_name = get_object_repr(pet);
    if (!object_name.empty())
        mvaddstr(y, x+3, object_name.c_str());
    mvaddstr(y+1, x+3, get_repr(pet).c_str());
    if (in_shop && has_attack_buff(pet))
        attron(A_UNDERLINE);
    mvprintw(y+2, x+1, "%02d", get_attack(pet));
    attroff(A_UNDERLINE);

    mvaddch(y+2, x+5, '/');

    if (in_shop && has_life_buff(pet))
        attron(A_UNDERLINE);
    mvprintw(y+2, x+7, "%02d", get_life(pet));
    attroff(A_UNDERLINE);

    int lvl = pet->get_level();
    if (disp_xp) {
        int xp = pet->get_xp();
        if (lvl == 1)
            mvprintw(y+3, x, "Lvl 1 %d/2", xp);
        else if (lvl == 2)
            mvprintw(y+3, x, "Lvl 2 %d/3", xp - 2);
        else
            mvaddstr(y+3, x, "Lvl 3 0/0");
    }
    if (frozen)
        mvaddstr(y+4, x+4, "🧊");
}

void GUI::disp_object(Object const* obj, int x, int y, bool frozen) const {
    mvaddstr(y, x+3, get_repr(obj).c_str());
    mvprintw(y+1, x, "Cost: %d", get_cost(obj));
    if (frozen)
        mvaddstr(y+2, x+3, "🧊");
}

void GUI::disp_team() const {
    int padding = (COLS-1 - 4*9) / 3.5;
    int inner_padding = (COLS-1 - 4*9) / 10;

    for (int i=4; i>=0; i--) {
        Pet const* pet = get_team_pet(i);
        if (pet)
            disp_pet(pet, padding, 7, true, true);
        else
            mvaddstr(8, padding, "  Empty  ");
        padding += 9 + inner_padding;
    }
}

void GUI::disp_shop() const {
    int padding = (COLS-1 - 4*9) / 6;
    int inner_padding = (COLS-1 - 4*9) / 9;

    size_t nb_pets = nb_pets_in_shop();
    for (size_t i=0; i<nb_pets; i++) {
        Pet const* pet = get_shop_pet(i);
        if (pet)
            disp_pet(pet, padding, 13, false, true, is_pet_frozen(i));
        else
            mvaddstr(15, padding, "  ___  ");
        padding += 9 + inner_padding;
    }
    padding += (9 + inner_padding) * (5 - nb_pets);

    size_t nb_objs = nb_objs_in_shop();
    for (size_t i=0; i<nb_objs; i++) {
        Object const* obj = get_shop_object(i, false);
        if (obj)
            disp_object(obj, padding, 14, is_obj_frozen(i));
        else
            mvaddstr(15, padding, "  ___  ");
        padding += 9 + inner_padding;
    }
}

void GUI::disp_action() const {
    std::string empty_msg(COLS-3, ' ');
    mvaddstr(20, 1, empty_msg.c_str());
    mvaddstr(20, 1, MSGS.at(m_state).c_str());
}

void GUI::disp_fight() const {
    // Clear previous pets
    std::string empty_line(COLS-3, ' ');
    for (int line=6; line<11; line++)
        mvaddstr(line, 1, empty_line.c_str());

    int middle = COLS / 2;
    int padding = (middle - 5*9) / 6;

    for (int i=0; i<4; i++)
        mvaddch(6+i, middle, '|');

    for (size_t i=0; i<5; i++) {
        int x = middle - padding*2*(i+1);
        Pet const* pet = get_team_pet(i);
        if (pet)
            disp_pet(pet, x, 6, true, false, false);
    }

    for (size_t i=0; i<5; i++) {
        int x = middle + padding*(2*i+1);
        Pet const* pet = get_adv_pet(i);
        if (pet)
            disp_pet(pet, x, 6, true, false, false);
    }
}

void GUI::disp_status() const {
    std::string empty_msg(COLS-3, ' ');
    mvaddstr(22, 1, empty_msg.c_str());
    mvaddstr(22, 1, m_status.c_str());
}

void GUI::disp_logs(bool clear) const {
    // Clean logs
    if (clear) {
        std::string empty_line(COLS-3, ' ');
        for (int line=23; line<LINES-1; line++)
            mvaddstr(line, 1, empty_line.c_str());
    }

    for (size_t i=0; i<utils::vector_logs.size(); i++) {
        std::string msg = "  | " + utils::vector_logs[i];
        mvaddstr(23+i, 3, msg.c_str());
    }

    if (clear)
        utils::vector_logs.clear();
}

void GUI::disp_play_again(bool invalid) const {
    std::string msg = "Game over ! Do you want to play again ? [y/N]";

    std::string hborder(msg.size() + 16, '-');
    hborder = "+" + hborder + "+";
    int first_col = COLS / 2 - msg.size() / 2 - 8;
    mvaddstr(LINES/5, first_col, hborder.c_str());
    mvaddstr(LINES/5+6, first_col, hborder.c_str());
    for (int j=LINES/5+1; j<LINES/5+6; j++) {
        mvaddch(j, first_col, '|');
        mvaddch(j, first_col + msg.size() + 17, '|');
    }

    std::string empty_line(msg.size() + 16, ' ');
    for (int i=LINES/5+1; i<LINES/5+6; i++)
        mvaddstr(i, first_col+1, empty_line.c_str());

    mvaddstr(LINES/5 + 3, COLS/2 - msg.size()/2, msg.c_str());
    if (invalid) {
        std::string invalid_msg = "Invalid choice, please type Y or N";
        mvaddstr(LINES/5 + 4, COLS/2 - invalid_msg.size()/2, invalid_msg.c_str());
    }
    refresh();
}
