
#include "Pets/Beaver.hpp"

#include <vector>

#include "utils.hpp"


Beaver::Beaver(Team* team, Shop* shop) : Pet("Beaver", team, shop) {
    repr = "🦫";
    id = 2;
    pack = PACK_STANDARD | PACK_DLC1;
    tier = 1;
    attack = 2;
    life = 2;
    reset_stats();
}

Beaver::~Beaver() { }

void Beaver::on_sell() {
    utils::vector_logs.push_back(name + " sold !");
    std::vector<Pet*> team_pets = team->get_pets();

    int lvl = get_level();
    std::vector<Pet*> targets = utils::choice(team_pets, 2);
    for (Pet* target : targets)
        target->buff(0, lvl, false);
}
