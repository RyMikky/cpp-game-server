#define _USE_MATH_DEFINES

#include "../src/collision_handler.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_contains.hpp>
#include <catch2/matchers/catch_matchers_predicate.hpp>

#include <sstream>

using namespace std::literals;
using namespace game_handler;
using Catch::Matchers::Contains;

// будем тестировать с точностью до второго знака после зап€той
static const double __TEST__EPSILON__ = 1e-2;

namespace Catch {
    template<>
    struct StringMaker<game_handler::CollisionEvent> {
        static std::string convert(game_handler::CollisionEvent const& value) {
            std::ostringstream tmp;
            tmp << "(";
            if (value.type == game_handler::CollisionEventType::GATHERING) {
                tmp << "Gathering";
            }
            else {
                tmp << "Return";
            }
            tmp << **value.player_token << "," << value.object_id << "," << value.sq_distance << "," << value.time << ")";

            return tmp.str();
        }
    };

    inline bool Comparator(const game_handler::CollisionEvent& lhs, const game_handler::CollisionEvent& rhs) {

        bool event_type_check = (lhs.type == rhs.type);
        bool gatherer_id_check = (lhs.player_token == rhs.player_token);
        bool item_id_check = (lhs.object_id == rhs.object_id);

        bool distance_check = ((std::max(std::abs(lhs.sq_distance), std::abs(rhs.sq_distance))
            - std::min(std::abs(lhs.sq_distance), std::abs(rhs.sq_distance)) < __TEST__EPSILON__));

        bool time_check = ((std::max(std::abs(lhs.time), std::abs(rhs.time))
            - std::min(std::abs(lhs.time), std::abs(rhs.time)) < __TEST__EPSILON__));

        return event_type_check && gatherer_id_check && item_id_check && distance_check && time_check;
    }

    struct IsPermutationMatcher : Catch::Matchers::MatcherGenericBase {
        IsPermutationMatcher(std::vector<game_handler::CollisionEvent> range)
            : range_{ std::move(range) } {
            std::sort(begin(range_), end(range_),
                [](const game_handler::CollisionEvent& lhs, const game_handler::CollisionEvent& rhs) {
                    return lhs.time < rhs.time;
                });
        }
        IsPermutationMatcher(IsPermutationMatcher&&) = default;

        inline bool match(std::vector<game_handler::CollisionEvent> other) const {
            using std::begin;
            using std::end;

            std::sort(begin(other), end(other),
                [](const game_handler::CollisionEvent& lhs, const game_handler::CollisionEvent& rhs) {
                    return lhs.time < rhs.time;
                });


            return std::equal(begin(range_), end(range_),
                begin(other), end(other), Comparator);
        }

        std::string describe() const override {
            // ќписание свойства, провер€емого матчером:
            return "Is permutation of: "s + Catch::rangeToString(range_);
        }

    private:
        std::vector<game_handler::CollisionEvent> range_;
    };

    IsPermutationMatcher IsPermutation(std::vector<game_handler::CollisionEvent>&& range) {
        return IsPermutationMatcher{ std::forward<std::vector<game_handler::CollisionEvent>>(range) };
    }

}  // namespace Catch

/*
* “ак как collision_handler.h подключает нам domain.h, а тот в свою очередь player.h, а тот model.h
* “о "изобретать" тестовых игрока, лут и офисы не надо. Ќужно только сделать сурогатный аналог GameSession
*/

static const std::vector<Token> __TEST_TOKENS__ = {
    Token{"first_token"}, Token{"second_token"}
};

class TestGameSession : public game_handler::CollisionProvider {
public:
    TestGameSession() = default;

    // ----------------- блок наполн€ющих методов ------------------------------------

    inline TestGameSession& AddPlayer(const Token* token, Player&& player) {
        players_.emplace(std::pair{ token, std::move(player) });
        return *this;
    }

    inline TestGameSession& AddLoot(size_t id, GameLoot&& loot) {
        loots_.emplace(std::pair{ id, std::move(loot) });
        return *this;
    }

    inline TestGameSession& AddOffice(model::Office&& office) {
        offices_.push_back(std::move(office));
        return *this;
    }

    // ----------------- блок об€зательных наследуемых методов интерфейса ------------

    // возвращает количество офисов бюро находок на карте игровой сессии
    size_t OfficesCount() const override {
        return offices_.size();
    }
    // возвращает офис бюро находок по индексу
    const model::Office& GetOffice(size_t idx) const override {
        // проверками на out of range в тесте пренебрежем, 
        //так как не будет €вно выходить за пределы
        return offices_[idx];
    }
    // возвращает ссылку на константную мапу игроков
    const SessionPlayers& GetPlayers() const override {
        return players_;
    }
    // возвращает ссылку на константную мапу лута
    const SessionLoots& GetLoots() const override {
        return loots_;
    }

private:
    SessionPlayers players_;
    SessionLoots loots_;
    model::Offices offices_;
};

static const std::vector<model::LootType> __LOOT_TYPES__ = {
    model::LootType{"key", "assets/key.obj", "obj", 90, "#338844", 0.03, 10},
    model::LootType{"wallet", "assets/wallet.obj", "obj", 0, "#883344", 0.01, 30},
    model::LootType{"pencil", "assets/pencil.obj", "obj", 45, "#3311AA", 0.01, 20},
    model::LootType{"gold", "assets/coin.obj", "obj", 0, "#3311AA", 0.01, 100}
};

SCENARIO("TestCollisionProvider", "[TestCollisionProvider]") {

    GIVEN("A TestGameSesion") {
    
        TestGameSession session;

        GIVEN("a new Player, Loot and Office") {

            GameLoot item{ __LOOT_TYPES__[0], 0, 0 , {10.0, 0.0} };
            Player player{0, "Vasiliy", nullptr, 3 };
            model::Office office{ model::Office::Id{"Buro"}, {20, 0}, {5, 0}};

            THEN("We can add their into collector, and check that") {
                session
                    .AddLoot(0, std::move(item))
                    .AddPlayer(&__TEST_TOKENS__[0],
                        std::move(player.SetFuturePosition(30, 0)))
                    .AddOffice(std::move(office));

                CHECK(session.GetPlayers().size() == 1);
                CHECK(session.GetLoots().size() == 1);
                CHECK(session.OfficesCount() == 1);

                AND_THEN("we can check collisions") {
                    
                    auto collisions = FindCollisionEvents(session);
                    CHECK(!collisions.empty());
                    CHECK(collisions.size() == 2);

                    CHECK(collisions[0].type == CollisionEventType::GATHERING);
                    CHECK(collisions[1].type == CollisionEventType::RETURN);

                }
            }
        }

        GIVEN("Add one player and one item with a coordinate difference of less than width / 2 + width / 2 in to session") {

            session
                .AddLoot(0,
                    std::move(GameLoot{ __LOOT_TYPES__[0], 0, 0 , {10.0, 0.2} }))
                .AddPlayer(&__TEST_TOKENS__[0],
                    std::move(Player{ 0, "Vasiliy", nullptr, 3 }.SetFuturePosition(30, 0)));

            THEN("We check collision, must be one collision") {

                auto collisions = FindCollisionEvents(session);
                CHECK(!collisions.empty());
                CHECK(collisions.size() == 1);

                AND_THEN("We check FindGatherEvents result") {

                    CHECK(Catch::Comparator(
                        { CollisionEventType::GATHERING, 0, &__TEST_TOKENS__[0], 0.04, 0.33 }, collisions[0]));
                }
            }
        }

        GIVEN("Add one player and one item with a coordinate difference equal to width / 2 + width / 2 in to session") {

            session
                .AddLoot(0,
                    std::move(GameLoot{ __LOOT_TYPES__[0], 0, 0 , {10.0, 0.3} }))
                .AddPlayer(&__TEST_TOKENS__[0],
                    std::move(Player{ 0, "Vasiliy", nullptr, 3 }.SetFuturePosition(30, 0)));

            THEN("We check collision, must be no collision") {

                CHECK(FindCollisionEvents(session).empty());

            }
        }

        GIVEN("Add one player and one item with a coordinate difference of greather than width / 2 + width / 2 in to session") {

            session
                .AddLoot(0,
                    std::move(GameLoot{ __LOOT_TYPES__[0], 0, 0 , {10.0, 0.4} }))
                .AddPlayer(&__TEST_TOKENS__[0],
                    std::move(Player{ 0, "Vasiliy", nullptr, 3 }.SetFuturePosition(30, 0)));

            THEN("We check collision, must be no collision") {

                CHECK(FindCollisionEvents(session).empty());

            }
        }

        GIVEN("Add 4 items and 2 players in to session") {

            session
                .AddLoot(0,
                    std::move(GameLoot{ __LOOT_TYPES__[0], 0, 0 , {0.0, 3.0} }))
                .AddLoot(1,
                    std::move(GameLoot{ __LOOT_TYPES__[1], 1, 1 , {0.0, 9.0} }))
                .AddLoot(2,
                    std::move(GameLoot{ __LOOT_TYPES__[2], 2, 2 , {5.0, 0.0} }))
                .AddLoot(3,
                    std::move(GameLoot{ __LOOT_TYPES__[3], 3, 3 , {12.0, 0.0} }))
                .AddPlayer(&__TEST_TOKENS__[0],
                    std::move(Player{ 0, "Vasiliy", nullptr, 3 }.SetFuturePosition(0, 12)))
                .AddPlayer(&__TEST_TOKENS__[1],
                    std::move(Player{ 0, "Mariya", nullptr, 3 }.SetFuturePosition(15, 0)));


            GIVEN("A match result reference") {


                std::vector<CollisionEvent> reference = { 
                    { CollisionEventType::GATHERING, 0, &__TEST_TOKENS__[0], 0.00, 0.25 },
                    { CollisionEventType::GATHERING, 2, &__TEST_TOKENS__[1], 0.00, 0.33 },
                    { CollisionEventType::GATHERING, 1, &__TEST_TOKENS__[0], 0.00, 0.75 },
                    { CollisionEventType::GATHERING, 3, &__TEST_TOKENS__[1], 0.00, 0.80 },
                };

                THEN("We check collision, all items must be collected") {

                    std::vector<CollisionEvent> result = FindCollisionEvents(session);
                    CHECK(result.size() == 4);

                    AND_THEN("We check result with reference") {
                        CHECK_THAT(reference, Catch::IsPermutation(std::move(result)));
                    }
                }
            }

            GIVEN("A unmatch result reference") {

                std::vector<CollisionEvent> reference = {
                    { CollisionEventType::GATHERING, 0, &__TEST_TOKENS__[0], 0.00, 0.25 },
                    { CollisionEventType::GATHERING, 2, &__TEST_TOKENS__[1], 0.00, 0.40 },
                    { CollisionEventType::GATHERING, 1, &__TEST_TOKENS__[0], 0.00, 0.75 },
                    { CollisionEventType::GATHERING, 3, &__TEST_TOKENS__[1], 0.00, 0.80 },
                };

                THEN("We check collision, all items must be collected") {

                    std::vector<CollisionEvent> result = FindCollisionEvents(session);
                    CHECK(result.size() == 4);

                    AND_THEN("We check result with unmatch reference") {
                        CHECK_THAT(reference, !Catch::IsPermutation(std::move(result)));
                    }
                }
            }
        }

        GIVEN("Add 7 items, 2 players and 2 office in to session") {

            session
                .AddLoot(0,
                    std::move(GameLoot{ __LOOT_TYPES__[0], 0, 0 , {0.0, 3.0} }))
                .AddLoot(1,
                    std::move(GameLoot{ __LOOT_TYPES__[3], 3, 1 , {0.0, 7.0} }))
                .AddLoot(2,
                    std::move(GameLoot{ __LOOT_TYPES__[2], 2, 2 , {0.0, 12.0} }))
                .AddLoot(3,
                    std::move(GameLoot{ __LOOT_TYPES__[3], 3, 3 , {0.0, 18.0} }))
                .AddLoot(4,
                    std::move(GameLoot{ __LOOT_TYPES__[2], 2, 4 , {5.0, 0.0} }))
                .AddLoot(5,
                    std::move(GameLoot{ __LOOT_TYPES__[1], 1, 5 , {9.0, 0.0} }))
                .AddLoot(6,
                    std::move(GameLoot{ __LOOT_TYPES__[0], 0, 6 , {21.0, 0.0} }))
                .AddOffice(std::move(
                    model::Office{ model::Office::Id{"Buro"}, {15, 0}, {5, 0} }))
                .AddOffice(std::move(
                    model::Office{ model::Office::Id{"Buro"}, {0, 30}, {5, 0} }))
                .AddPlayer(&__TEST_TOKENS__[0],
                    std::move(Player{ 0, "Vasiliy", nullptr, 3 }.SetFuturePosition(0, 30)))
                .AddPlayer(&__TEST_TOKENS__[1],
                    std::move(Player{ 0, "Mariya", nullptr, 3 }.SetFuturePosition(30, 0)));

            GIVEN("A match result reference") {

                std::vector<CollisionEvent> reference = {
                    { CollisionEventType::GATHERING, 0, &__TEST_TOKENS__[0], 0.00, 0.10 },
                    { CollisionEventType::GATHERING, 4, &__TEST_TOKENS__[1], 0.00, 0.166666 },
                    { CollisionEventType::GATHERING, 1, &__TEST_TOKENS__[0], 0.00, 0.233333 },
                    { CollisionEventType::GATHERING, 5, &__TEST_TOKENS__[1], 0.00, 0.299999 },
                    { CollisionEventType::GATHERING, 2, &__TEST_TOKENS__[0], 0.00, 0.40 },
                    { CollisionEventType::RETURN, 0, &__TEST_TOKENS__[1], 0.00, 0.50 },
                    { CollisionEventType::GATHERING, 3, &__TEST_TOKENS__[0], 0.00, 0.599999 },
                    { CollisionEventType::GATHERING, 6, &__TEST_TOKENS__[1], 0.00, 0.699999 },
                    { CollisionEventType::RETURN, 1, &__TEST_TOKENS__[0], 0.00, 1.00 }
                };

                THEN("We check collision, all events must be collected in correct order") {

                    std::vector<CollisionEvent> result = FindCollisionEvents(session);
                    CHECK(result.size() == 9);

                    AND_THEN("We check result with reference") {
                        CHECK_THAT(reference, Catch::IsPermutation(std::move(result)));
                    }
                }
            }
        }
    }
}