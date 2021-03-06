
#include "Objects/Foods/Cupcake.hpp"

#include <vector>

#include "utils.hpp"


Cupcake::Cupcake(Team* team, Shop* shop) : Object("Cupcake", team, shop, ObjType::FOOD) {
    repr = "🧁";
    id = 3;
    tier = 2;
}

Cupcake::~Cupcake() { }

void Cupcake::on_buy(size_t index) {
    utils::vector_logs.push_back(name + " bought !");
    std::vector<Pet*> team_pets = team->get_pets();

    team_pets[index]->buff(3, 3, true);
}
