
#include "game.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

#include "object.hpp"
#include "utils.hpp"


static int life_per_turn(int turn) {
    if (turn < 3)
        return 1;
    else if (turn < 5)
        return 2;
    return 3;
}


Game::Game() : life(10), victories(0), turn(0) {
    team = new Team(this);
    shop = new Shop(team);
    begin_turn();
    fight_status = FIGHT_STATUS::None;
};

Game::~Game() {
    Team::clear_team_list();
    delete team;
    delete shop;
}

void Game::reset() {
    life = 10;
    victories = 0;
    turn = 0;

    delete team;
    delete shop;
    team = new Team(this);
    shop = new Shop(team);

    begin_turn();
}


void Game::begin_turn() {
    money = 10;
    shop->begin_turn();
    team->begin_turn();
    turn++;
}

void Game::move(Pos src_pos, Pos dst_pos) {
    team->move(src_pos, dst_pos);
}

void Game::end_turn() {
    team->end_turn();
}

bool Game::is_over() const {
    return !(life > 0 && victories < 10);
}

void Game::start_fight() {
    fighting_team = Team::copy_team(team);
    adv_team = Team::get_random_team(turn);
    fight_status = FIGHT_STATUS::Fighting;

    fighting_team->bind(adv_team);
    adv_team->bind(fighting_team);
}

bool Game::fight_step() {
    fight_status = Team::fight_step(fighting_team, adv_team);
    return in_fight();
}

int Game::end_fight() {
    int output = 0;
    if (fight_status == FIGHT_STATUS::Win) {
        victories++;
        output = 1;
    } else if (fight_status == FIGHT_STATUS::Loss) {
        life -= life_per_turn(turn);
        output = -1;
    }

    delete fighting_team;
    delete adv_team;
    fighting_team = nullptr;
    adv_team = nullptr;

    if (life > 0)
        begin_turn();

    team->reset();
    fight_status = FIGHT_STATUS::None;

    return output;
}

bool Game::in_fight() const {
    return fight_status == FIGHT_STATUS::Fighting;
}

void Game::buy_pet(Pos src_pos, Pos dst_pos) {
    check_money("BUY_PET", 3);
    if (team->has_pet(dst_pos)) {
        combine_from_shop(src_pos, dst_pos);
        return;
    }

    if (team->get_nb_pets() == 5)
        throw InvalidAction("[BUY_PET]: already have 5 pets in the team !");
    Pet* pet = shop->buy_pet(src_pos);
    team->add(pet, dst_pos);
    money -= 3;
}

void Game::buy_object(size_t obj_index, Pos pet_pos) {
    int cost = shop->get_cost_object(obj_index);
    check_money("BUY_OBJECT", cost);

    size_t pet_index = team->pos_to_index(pet_pos);
    shop->buy_object(obj_index, pet_index);
    money -= cost;
}

void Game::combine(Pos src_pos, Pos dst_pos) {
    team->combine(src_pos, dst_pos);
}

void Game::sell(Pos pos) {
    money += team->sell(pos);
}

void Game::roll() {
    check_money("ROLL", 1);

    shop->roll();
    money--;
}

void Game::freeze_pet(size_t index) {
    shop->freeze_pet(index);
}

void Game::freeze_object(size_t index) {
    shop->freeze_object(index);
}

void Game::earn_money(int amount) {
    money += amount;
}

std::string Game::serialize() const {
    std::string state = std::to_string(turn) + " ";
    state += std::to_string(life) + " ";
    state += std::to_string(victories) + " ";
    state += std::to_string(money) + "\n";
    state += team->serialize() + "\n";
    state += shop->serialize();
    return state;
}

void Game::unserialize(std::string game_str) {
    std::stringstream stream(game_str);
    std::string line;

    std::getline(stream, line, '\n');
    size_t index = line.find(' ');
    turn = std::stoi(line.substr(0, index));
    line = line.substr(index+1);

    index = line.find(' ');
    life = std::stoi(line.substr(0, index));
    line = line.substr(index+1);

    index = line.find(' ');
    victories = std::stoi(line.substr(0, index));
    line = line.substr(index+1);

    index = line.find(' ');
    money = std::stoi(line.substr(0, index));

    std::string turn_str = std::to_string(turn) + " ";
    delete team;
    std::getline(stream, line, '\n');
    team = Team::unserialize(this, turn_str + line);

    delete shop;
    std::getline(stream, line, '\n');
    shop = Shop::unserialize(team, turn_str + line);

    team->bind(shop);
}

void Game::save_state() const {
    std::string state = serialize();
    std::ofstream file("data/game_save");
    if (!file) {
        std::cout << "Couldn't save file" << std::endl;
        return;
    }

    file << state;
    file.close();
}

void Game::load_state() {
    std::ifstream file("data/game_save");
    if (!file) {
        std::cout << "Couldn't load file" << std::endl;
        return;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string state = buffer.str();
    file.close();

    unserialize(state);
}

void Game::cheat() {
    money += 1000;
}


void Game::combine_from_shop(size_t shop_index, Pos pet_pos) {
    std::string shop_pet_name = shop->get_pet_name(shop_index);
    team->can_combine(pet_pos, shop_pet_name);

    Pet* pet = shop->buy_pet(shop_index);
    team->combine(pet_pos, pet);
    money -= 3;
}

void Game::check_money(std::string action, int amount) const {
    if (money >= amount)
        return;

    std::string msg = "[" + action + "]: not enough money to do this action";
    msg += " (" + std::to_string(money) + " gold left)";
    throw InvalidAction(msg);
}
