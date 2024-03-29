#ifndef HEADER_UI_CLI
#define HEADER_UI_CLI

#include <string>
#include <vector>

#include "UI/user_interface.hpp"
#include "game.hpp"
#include "object.hpp"
#include "pet.hpp"


class CLI : public UserInterface {
    public:
        CLI(Game* game);
        ~CLI();

        bool run() override;
        void display_game() const override;

    private:
        std::vector<std::string> m_curr_cmd;
        std::vector<std::string> m_prev_cmd;

        void get_curr_cmd(std::string line);
        std::vector<int> get_args(size_t n) const;

        bool play_again() const;
        bool act();

        void help() const;
        void buy();
        void combine();
        void sell();
        void roll();
        void freeze();
        void move();
        void fight();
        bool end_turn();

        void download() const;
        void load();

        void disp_game_state() const;
        void disp_pet(int padding, Pet const* pet,
                      std::string& objects, std::string& pets,
                      std::string& stats, std::string& xps, bool in_fight) const;
        void disp_team() const;
        void disp_shop() const;
        void disp_fight() const;
        void disp_logs() const;
};


#endif // HEADER_UI_CLI
