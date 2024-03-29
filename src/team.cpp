
#include "team.hpp"

#include <algorithm>
#include <fstream>

#include "game.hpp"
#include "utils.hpp"


TeamList Team::team_list;

Team* Team::unserialize(Game* game, std::string team_str) {
    Team* new_team = new Team(game);
    size_t index = team_str.find(' ');

    if (index == std::string::npos || index == team_str.size() - 1) {
        new_team->m_turn = std::stoi(team_str);
        return new_team;
    }

    new_team->m_turn = std::stoi(team_str.substr(0, index));
    team_str = team_str.substr(index);

    size_t order_index = team_str.find('/');
    std::string order_str;
    if (order_index != std::string::npos) {
        order_str = team_str.substr(order_index+2);
        team_str = team_str.substr(0, order_index-1);
    }

    Pos pos = 0;
    while (!team_str.empty()) {
        index = team_str.find(')');
        std::string pet_str = team_str.substr(2, index-2);
        team_str = team_str.substr(index+1);

        Pos saved_pos = pos;
        if (!order_str.empty())
            saved_pos = order_str[pos] - '0';
        new_team->append_pet(Pet::unserialize(new_team, nullptr, pet_str), saved_pos);
        pos++;
    }

    return new_team;
}

Team* Team::get_random_team(int turn) {
    if (Team::team_list.empty())
        Team::load_teams();

    int opp_turn = turn;
    while (team_list[opp_turn].empty())
        opp_turn--;

    Team* opp_team = utils::choice(team_list[opp_turn])[0];
    return Team::copy_team(opp_team);
}

Team* Team::copy_team(Team const* team) {
    Team* new_team = new Team(team->m_game);
    new_team->m_turn = team->m_turn;

    for (size_t i=0; i<team->m_pets.size(); i++) {
        Pet* new_pet = Pet::copy_pet(team->m_pets[i], new_team, nullptr);
        new_team->append_pet(new_pet, team->m_order[i]);
    }

    return new_team;
}

void Team::clear_team_list() {
    for (auto [turn, teams] : Team::team_list) {
        for (Team* team : teams)
            delete team;
    }
    Team::team_list.clear();
}

FIGHT_STATUS Team::fight_step(Team* team, Team* adv_team) {
    // Start of battle
    if (!team->m_in_fight) {
        team->m_in_fight = true;
        utils::vector_logs.push_back("Starting fight !");
        return start_of_battle(team, adv_team);
    }

    utils::vector_logs.push_back("Another step");

    Pet* pet = team->m_pets.front();
    Pet* adv_pet = adv_team->m_pets.front();

    Pet *first_pet, *second_pet;
    if (pet->get_attack() > adv_pet->get_attack()) {
        first_pet = pet;
        second_pet = adv_pet;
    } else {
        first_pet = adv_pet;
        second_pet = pet;
    }

    first_pet->on_before_attack();
    second_pet->on_before_attack();

    if (first_pet->is_alive() && second_pet->is_alive()) {
        first_pet->attacks(second_pet);
        second_pet->attacks(first_pet);
    }

    if (first_pet->is_alive())
        first_pet->on_hurt();
    if (second_pet->is_alive())
        second_pet->on_hurt();

    Team::remove_dead_pets(team, adv_team);
    team->add_summons();
    adv_team->add_summons();

    return Team::check_end_of_battle(team, adv_team);
}


Team::Team(Game* game) : m_game(game), m_turn(0) { }

Team::~Team() {
    for (Pet* pet : m_pets)
        delete pet;
}

void Team::bind(Team* adv_team) const {
    for (Pet* pet : m_pets)
        pet->bind(adv_team);
}

void Team::bind(Shop* shop) const {
    for (Pet* pet : m_pets)
        pet->bind(shop);
}

size_t Team::get_nb_pets() const {
    return m_pets.size();
}

std::vector<Pet*> Team::get_pets() {
    return m_pets;
}

void Team::can_combine(Pos pos, std::string other_pet) const {
    size_t index = pos_to_index(pos);
    if (m_pets[index]->m_name != other_pet)
        throw InvalidAction("[COMBINE]: trying to combine different pets");
}

void Team::can_combine(Pos src_pos, Pos dst_pos) const {
    if (src_pos == dst_pos)
        throw InvalidAction("[COMBINE]: same source (" + std::to_string(src_pos+1) + ")" + \
                            " and destination (" + std::to_string(dst_pos+1) + ")");
    size_t src_index = pos_to_index(src_pos);
    can_combine(dst_pos, m_pets[src_index]->m_name);
}

void Team::begin_turn() {
    m_turn++;
    for (Pet* pet : m_pets)
        pet->on_start_turn();
}

void Team::move(Pos src_pos, Pos dst_pos) {
    for (size_t i=0; i<m_order.size(); i++) {
        if (m_order[i] == src_pos)
            m_order[i] = dst_pos;
        else if (m_order[i] == dst_pos)
            m_order[i] = src_pos;
    }
    sort_team();
}

void Team::end_turn() {
    for (Pet* pet : m_pets)
        pet->on_end_turn();
}

void Team::reset() {
    for (Pet* pet : m_pets)
        pet->reset_stats();
}

size_t Team::pos_to_index(Pos pos) const {
    for (size_t i=0; i<m_pets.size(); i++) {
        if (m_order[i] == pos)
            return i;
    }
    throw InvalidAction("No pet in team at position " + std::to_string(pos+1));
}

bool Team::has_pet(Pos pos) const {
    for (size_t i=0; i<m_pets.size(); i++) {
        if (m_order[i] == pos)
            return true;
    }
    return false;
}

void Team::add(Pet* new_pet, Pos pos) {
    new_pet->on_buy();
    for (Pet* pet : m_pets)
        pet->on_friend_bought(new_pet);
    append_pet(new_pet, pos);
}

void Team::combine(Pos pos, Pet* other_pet, bool activate_on_buy) {
    size_t index = pos_to_index(pos);
    Pet* dst = m_pets[index];
    dst->combine(other_pet);
    if (activate_on_buy)
        dst->on_buy();
    delete other_pet;
}

void Team::combine(Pos src_pos, Pos dst_pos) {
    can_combine(src_pos, dst_pos);

    size_t src_index = pos_to_index(src_pos);
    combine(dst_pos, m_pets[src_index], false);
    remove_pet(src_index);
}

int Team::sell(Pos pos) {
    size_t index = pos_to_index(pos);
    Pet* pet = m_pets[index];
    int lvl = pet->get_level();

    remove_pet(index);
    pet->on_sell();
    delete pet;

    for (Pet* other_pet : m_pets)
        other_pet->on_friend_sold();

    return lvl;
}

void Team::summon(Pos pos, Pet* new_pet) {
    m_summoned_pets.push_back({new_pet, pos});
}

void Team::faint(size_t index) {
    Pet* pet = m_pets[index];
    Pos pos = m_order[index];
    remove_pet(index);
    pet->on_faint(pos);
    delete pet;
}

void Team::give_object(size_t index, Object* obj) {
    if (obj->m_type == ObjType::ITEM)
        m_pets[index]->equip_object(obj);

    if (!obj->m_target_all)
        m_pets[index]->on_object(obj);

    for (size_t i=0; i<m_pets.size(); i++)
        m_pets[i]->on_object_bought(index, obj);

    if (obj->m_name == "Pill") {
        remove_dead_pets();
        add_summons();
    }
}

void Team::earn_money(int amount) const {
    m_game->earn_money(amount);
}

std::tuple<int, std::string, std::string> Team::get_fight_str(Team* other_team) {
    m_in_fight = true;

    while (!m_pets.empty() && !other_team->m_pets.empty()) {
        Pet* pet = m_pets.front();
        Pet* other_pet = other_team->m_pets.front();

        while (pet->is_alive() && other_pet->is_alive()) {
            if (pet->get_attack() > other_pet->get_attack()) {
                pet->attacks(other_pet);
                other_pet->attacks(pet);
            } else {
                other_pet->attacks(pet);
                pet->attacks(other_pet);
            }
        }

        if (!pet->is_alive())
            pet->on_faint(0);
        if (!other_pet->is_alive())
            other_pet->on_faint(0);
    }

    int output;
    if (m_pets.empty() && other_team->m_pets.empty())
        output = 0;
    else if (m_pets.empty())
        output = -1;
    else
        output = 1;

    std::string team_str1 = serialize();
    std::string team_str2 = other_team->serialize();

    m_in_fight = false;
    reset();
    other_team->reset();
    return {output, team_str1, team_str2};
}

std::string Team::serialize() const {
    if (m_pets.empty())
        return "";

    std::string team_str;
    for (Pet* pet : m_pets) {
        if (pet->is_alive())
            team_str += pet->serialize() + " ";
    }
    team_str.pop_back();

    team_str += " / ";
    for (Pos pos : m_order)
        team_str += std::to_string(pos);

    return team_str;
}


void Team::load_teams() {
    std::ifstream team_file("data/saved_teams.txt");
    std::string team_str;
    while (getline(team_file, team_str)) {
        Team* new_team = Team::unserialize(nullptr, team_str);
        Team::team_list[new_team->m_turn].push_back(new_team);
    }
}

std::vector<Pet*> Team::order_pets(Team const* team, Team const* adv_team) {
    std::vector<Pet*> ordered_pets;
    while (ordered_pets.size() < team->m_pets.size() + adv_team->m_pets.size()) {
        Pet* curr_pet;
        int max_attack = 0;
        for (Pet* pet : team->m_pets) {
            bool done = (std::find(ordered_pets.begin(), ordered_pets.end(), pet) != ordered_pets.end());
            if (!done && pet->get_attack() > max_attack) {
                max_attack = pet->get_attack();
                curr_pet = pet;
            }
        }

        for (Pet* pet : adv_team->m_pets) {
            bool done = (std::find(ordered_pets.begin(), ordered_pets.end(), pet) != ordered_pets.end());
            if (!done && pet->get_attack() > max_attack) {
                max_attack = pet->get_attack();
                curr_pet = pet;
            }
        }

        ordered_pets.push_back(curr_pet);
    }

    return ordered_pets;
}

void Team::remove_dead_pets(Team* team, Team* adv_team) {
    while (team->remove_dead_pets() || adv_team->remove_dead_pets()) { }
}

FIGHT_STATUS Team::start_of_battle(Team* team, Team* adv_team) {
    std::vector<Pet*> ordered_pets = order_pets(team, adv_team);

    for (Pet* pet : ordered_pets)
        pet->on_start_battle();

    Team::remove_dead_pets(team, adv_team);
    return check_end_of_battle(team, adv_team);
}

FIGHT_STATUS Team::check_end_of_battle(Team const* team, Team const* adv_team) {
    if (team->m_pets.empty() && adv_team->m_pets.empty()) {
        utils::vector_logs.push_back("Draw !");
        return FIGHT_STATUS::Draw;
    } else if (adv_team->m_pets.empty()) {
        utils::vector_logs.push_back("Win !");
        return FIGHT_STATUS::Win;
    } else if (team->m_pets.empty()) {
        utils::vector_logs.push_back("Loss...");
        return FIGHT_STATUS::Loss;
    }

    return FIGHT_STATUS::Fighting;
}

void Team::sort_team() {
    if (m_pets.size() <= 1)
        return;

    std::vector<Pet*> ordered_pets;
    std::vector<Pos> new_order;
    for (size_t i=0; i<5; i++) {
        for (size_t j=0; j<m_pets.size(); j++) {
            if (m_order[j] == i) {
                ordered_pets.push_back(m_pets[j]);
                new_order.push_back(i);
            }
        }
    }
    m_pets = ordered_pets;
    m_order = new_order;
}

void Team::append_pet(Pet* new_pet, Pos pos) {
    if (has_pet(pos))
        throw InvalidAction("[APPEND_PET] Already got pet at position " + std::to_string(pos+1));

    m_pets.push_back(new_pet);
    m_order.push_back(pos);
    sort_team();
}

void Team::add_summons() {
    if (m_summoned_pets.empty())
        return;

    std::sort(m_summoned_pets.begin(), m_summoned_pets.end(),
              [](std::pair<Pet*, Pos> p1, std::pair<Pet*, Pos> p2) {
              return p1.second < p2.second;
              });

    for (size_t index=0; index<m_summoned_pets.size(); index++) {
        auto [new_pet, pos] = m_summoned_pets[index];

        utils::vector_logs.push_back("Summoning new " + new_pet->m_name);
        if (m_pets.size() >= 5) {
            utils::vector_logs.push_back("Already 5 pets in team, aborting");
            delete new_pet;
            continue;
        }

        bool first = false;
        if (pos == (size_t) -1) {
            first = true;
            pos = 0;
        }

        if (has_pet(pos)) {
            size_t ind = pos_to_index(pos);
            size_t min_pos = (first ? 0 : 1);
            size_t max_pos = pos + 1;
            for (size_t i=min_pos; ind+i < m_pets.size() && m_order[ind+i] == pos+i; i++) {
                m_order[ind+i]++;
                max_pos = i + 1;
            }

            for (size_t i=index+1; i < m_summoned_pets.size() && pos <= max_pos; i++) {
                Pos new_pos = m_summoned_pets[i].second;
                m_summoned_pets[i].second += (new_pos <= max_pos);
                max_pos += (new_pos == max_pos);
            }

            // Magic function that deals with inserting pet, and modify
            // the position of other pets accordingly
            pos += (!first && pos < 4);
            int n = m_order.size() - 1;
            max_pos = 4;

            // If last pet is at position more than 4 and while a pet is at an order
            // greater than its max possible position
            while (n >= 0 && m_order[n] >= max_pos) {
                // While on the left of inserted pet, set pos to max possible
                if (m_order[n] > pos) {
                    m_order[n] = max_pos;
                    // If there were no empty spaces before, move position once to the right
                    pos -= (max_pos == pos);

                // After the inserted pet, continue shifting right if needed
                } else {
                    m_order[n]--;
                }

                for (size_t j=index+1; j<m_summoned_pets.size(); j++) {
                    std::pair<Pet*, Pos>& pet_pos = m_summoned_pets[j];
                    pet_pos.second -= (pet_pos.second == max_pos);
                }

                max_pos--;
                n--;
            }
        }

        m_pets.push_back(new_pet);
        m_order.push_back(pos);
        sort_team();

        for (Pet* pet : m_pets) {
            if (pet != new_pet)
                pet->on_friend_summoned(new_pet);
        }
    }

    m_summoned_pets.clear();
}

void Team::remove_pet(size_t index) {
    m_pets.erase(m_pets.begin() + index);
    m_order.erase(m_order.begin() + index);
}

bool Team::remove_dead_pets() {
    bool removed;
    int nb_pets_removed = 0;
    // Continue removing while pets are dying because faint triggers can kill other
    do {
        removed = false;
        size_t i = 0;
        while (i < m_pets.size()) {
            if (!m_pets[i]->is_alive()) {
                faint(i);
                removed = true;
                nb_pets_removed++;
            } else {
                i++;
            }
        }
    } while (removed);
    return nb_pets_removed > 0;
}
