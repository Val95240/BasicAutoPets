
#include "Objects/Items/Honey.hpp"

#include "Pets/all_pets.hpp"
#include "utils.hpp"


Honey::Honey(Team* team, Shop* shop) : Object("Honey", team, shop, ObjType::ITEM) {
    repr = "🍯";
    id = 2;
    tier = 1;
}

Honey::~Honey() { }

void Honey::on_faint(Pos pos) {
    utils::vector_logs.push_back(name + " on " + attached_pet->name + " summons a bee !");

    Pet* bee = AllPets::create_new_pet("bee", team, shop);
    bee->bind(attached_pet->get_adv_team());
    team->summon(pos, bee);
}
