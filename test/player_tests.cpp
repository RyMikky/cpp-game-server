#include <cmath>
#include <vector>
#include <catch2/catch_test_macros.hpp>

#include "../src/player.h"

using namespace std::literals;
using namespace game_handler;
using namespace model;

static const std::vector<LootType> __LOOT_TYPES__ = {
	LootType{"key", "assets/key.obj", "obj", 90, "#338844", 0.03, 10}, 
	LootType{"wallet", "assets/wallet.obj", "obj", 0, "#883344", 0.01, 30},
	LootType{"pencil", "assets/pencil.obj", "obj", 45, "#3311AA", 0.01, 20},
	LootType{"gold", "assets/coin.obj", "obj", 0, "#3311AA", 0.01, 100}
};

static const PlayerPosition __TEST_POSITION__ = { 10.0, 10.0 };
static const PlayerPosition __FUTURE_POSITION__ = { 15.0, 5.0 };

static const PlayerSpeed __ZERO_SPEED__ = { 0, 0 };
static const PlayerSpeed __TEST_SPEED__ = { 1, -1 };
static const PlayerPosition __ONE_SECOND_POSITION__ = { 11.0, 9.0 };
static const PlayerPosition __FIVE_SECOND_POSITION__ = { 15.0, 5.0 };
static const PlayerPosition __SIX_SECOND_POSITION__ = { 16.0, 4.0 };

SCENARIO("GamePlayer test module", "[GamePlayer]") {

	GIVEN("a GamePlayer") {

		Player player;

		THEN("we can set player position") {

			player.SetCurrentPosition(__TEST_POSITION__.x_ , __TEST_POSITION__.y_);
			CHECK(player.GetCurrentPosition() == __TEST_POSITION__);

			THEN("we can set player future position") {

				player.SetFuturePosition(__FUTURE_POSITION__.x_, __FUTURE_POSITION__.y_);
				CHECK(player.GetFuturePosition() == __FUTURE_POSITION__);

				THEN("we can update player position") {

					player.UpdateCurrentPosition();
					CHECK(player.GetCurrentPosition() == __FUTURE_POSITION__);
					CHECK(player.GetFuturePosition() == __FUTURE_POSITION__);
				}
			}

			AND_THEN("try update player position on one second without speed") {

				player.UpdateCurrentPosition(1);
				CHECK(player.GetCurrentPosition() == __TEST_POSITION__);
				CHECK(player.GetFuturePosition() == __TEST_POSITION__);
			}
		}

		THEN("we can set start position and add speed") {

			player.SetCurrentPosition(__TEST_POSITION__.x_, __TEST_POSITION__.y_);
			player.SetSpeed(__TEST_SPEED__.xV_, __TEST_SPEED__.yV_);
			CHECK(player.GetSpeed() == __TEST_SPEED__);

			THEN("try update player position on one second") {

				player.UpdateCurrentPosition(1);
				CHECK(player.GetCurrentPosition() == __ONE_SECOND_POSITION__);
				CHECK(player.GetFuturePosition() == __ONE_SECOND_POSITION__);
			}

			THEN("try update player position on five second") {

				player.UpdateCurrentPosition(5);
				CHECK(player.GetCurrentPosition() == __FIVE_SECOND_POSITION__);
				CHECK(player.GetFuturePosition() == __FIVE_SECOND_POSITION__);

				AND_THEN("try update player position on one second more") {

					player.UpdateCurrentPosition(1);
					CHECK(player.GetCurrentPosition() == __SIX_SECOND_POSITION__);
					CHECK(player.GetFuturePosition() == __SIX_SECOND_POSITION__);
				}
			}
		}

		THEN("we can set bag capacity") {

			player.SetBagCapacity(3);
			CHECK(player.GetBagCapacity() == 3);

			AND_GIVEN("a massiv with four GameLoots") {

				std::vector<GameLoot> four_game_loots = {
					GameLoot{__LOOT_TYPES__[0], 0, {0.0, 0.0}}, GameLoot{__LOOT_TYPES__[1], 1, {3.0, 3.0}},
					GameLoot{__LOOT_TYPES__[2], 2, {6.0, 6.0}}, GameLoot{__LOOT_TYPES__[3], 3, {7.0, 9.0}}
				};

				THEN("try add one loot item in to the player bag") {

					CHECK(player.AddLoot(0, &four_game_loots[0]));
					CHECK(player.GetBagSize() == 1);
					CHECK(player.GetScore() == 0);
					CHECK(player.GetLootTotalValue() == 10);

					AND_THEN("we check, four_game_loots[0] have ptr to player, item in the bag") {
						CHECK(four_game_loots[0].player_ == &player);
					}

					AND_THEN("we try return loot index 1") {
						CHECK(!player.ReturnLoot(0).IsDummy());
						CHECK(player.GetBagSize() == 0);
						CHECK(player.GetScore() == 10);
					}

					AND_THEN("we check that returned bagitem collect correct ptr") {
						auto bag_item = player.ReturnLoot(0);
						CHECK(bag_item.loot_ == &four_game_loots[bag_item.index_]);
					}
					
					AND_THEN("we try return loot with out of bag current size range") {
						CHECK_THROWS_AS(player.ReturnLoot(1), std::out_of_range);
					}

					AND_THEN("we try return loot with out of bag capacity range") {
						CHECK_THROWS_AS(player.ReturnLoot(4), std::out_of_range);
					}

					AND_THEN("we try remove loot index 1") {
						CHECK(player.RemoveLoot(0));
						CHECK(player.GetBagSize() == 0);
						CHECK(player.GetScore() == 0);
					}

					AND_THEN("we try remove loot with out of bag current size range") {
						CHECK_THROWS_AS(player.RemoveLoot(1), std::out_of_range);
					}

					AND_THEN("we try remove loot with out of bag capacity range") {
						CHECK_THROWS_AS(player.RemoveLoot(4), std::out_of_range);
					}
				}

				AND_THEN("try add two loot items in to the player bag") {

					player.AddLoot(0, &four_game_loots[0]);
					player.AddLoot(1, &four_game_loots[1]);
					CHECK(player.GetBagSize() == 2);
					CHECK(player.GetScore() == 0);
					CHECK(player.GetLootTotalValue() == 40);
				}

				AND_THEN("try add three loot items in to the player bag") {

					player.AddLoot(0, &four_game_loots[0]);
					player.AddLoot(1, &four_game_loots[1]);
					player.AddLoot(2, &four_game_loots[2]);
					CHECK(player.GetBagSize() == 3);
					CHECK(player.GetScore() == 0);
					CHECK(player.GetLootTotalValue() == 60);
				}

				AND_THEN("try add four loot items in to the player bag") {

					player.AddLoot(0, &four_game_loots[0]);
					player.AddLoot(1, &four_game_loots[1]);
					player.AddLoot(2, &four_game_loots[2]);
					CHECK(!player.AddLoot(3, &four_game_loots[3]));
					CHECK(player.GetBagSize() == 3);
					CHECK(player.GetScore() == 0);
					CHECK(player.GetLootTotalValue() == 60);

					AND_THEN("we check, four_game_loots[3] ptr to player == nullptr, item stay on map") {
						CHECK(four_game_loots[3].player_ == nullptr);
					}

					AND_THEN("we can return player loot items in to the office") {

						player.ReturnLootToTheOffice();
						CHECK(player.GetBagSize() == 0);
						CHECK(player.GetScore() == 60);
						CHECK(player.GetLootTotalValue() == 0);
					}
				}
			}
		}
	}
}