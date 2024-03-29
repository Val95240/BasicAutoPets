#ifndef HEADER_SHOP
#define HEADER_SHOP

#include <string>
#include <vector>

#include "pet.hpp"
#include "object.hpp"
#include "team.hpp"


class Shop {
    friend class UserInterface;
    friend class Environment;

    public:
        static Shop* unserialize(Team* team, std::string shop_str);
        static size_t get_max_pets(int turn);
        static size_t get_max_objects(int turn);

        Shop(Team* team);
        ~Shop();

        int get_cost_object(size_t index) const;
        std::string get_pet_name(size_t index) const;

        void begin_turn();
        void roll();
        void create_bonus_pet();

        Pet* buy_pet(size_t index);
        void buy_object(size_t index, size_t index_target);

        void freeze_pet(size_t index);
        void freeze_object(size_t index);

        void upgrade(int attack, int life, bool tmp);

        std::string serialize() const;

    private:
        Team* m_team;

        int m_turn;
        std::vector<Pet*> m_pets;
        std::vector<Object*> m_objects;
        std::vector<bool> m_frozen_pets;
        std::vector<bool> m_frozen_objects;
        int m_attack_buff;
        int m_life_buff;

        Pet* create_pet();
        Object* create_object();
        void check_size_pets(std::string action, size_t index) const;
        void check_size_objects(std::string action, size_t index) const;
};


#endif // HEADER_SHOP
