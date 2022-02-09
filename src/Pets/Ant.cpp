
#include "Pets/Ant.hpp"

#include <algorithm>
#include <iterator>
#include <vector>

#include "utils.hpp"


Ant::Ant(Team* team, Shop* shop) : Pet("ant", team, shop) {
    id = 1;
    pack = PACK_STANDARD | PACK_DLC1;
    tier = 1;
    attack = 2;
    life = 1;
    reset_stats();
};

Ant::~Ant() { }

void Ant::on_faint() {
    spdlog::debug("{} fainting !", name);
    Pet::on_faint();

    std::vector<Pet*> team_pets = get_team_pets();
    std::vector<Pet*> living_pets;

    std::copy_if(team_pets.begin(), team_pets.end(),
                 std::back_inserter(living_pets),
                 [](Pet* pet) { return pet->is_alive(); });

    if (living_pets.empty())
        return;

    Pet* target = utils::choice(living_pets)[0];
    int lvl = get_level();
    target->buff(2*lvl, 1*lvl, team->is_fighting());
}