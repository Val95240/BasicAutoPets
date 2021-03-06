#ifndef HEADER_OBJECTS_APPLE
#define HEADER_OBJECTS_APPLE

#include "object.hpp"
#include "shop.hpp"
#include "team.hpp"


class Apple : public Object {
    public:
        Apple(Team* team, Shop* shop);
        ~Apple() override;

        void on_buy(size_t index) override;
};


#endif // HEADER_OBJECTS_APPLE
