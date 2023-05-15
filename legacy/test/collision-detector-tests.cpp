#define _USE_MATH_DEFINES

#include "../src/collision_detector.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_contains.hpp>
#include <catch2/matchers/catch_matchers_predicate.hpp>

#include <sstream>

using namespace std::literals;
using Catch::Matchers::Contains; 

static const double __EPSILON__ = 1e-6;

namespace Catch {
    template<>
    struct StringMaker<collision_detector::GatheringEvent> {
        static std::string convert(collision_detector::GatheringEvent const& value) {
            std::ostringstream tmp;
            tmp << "(" << value.gatherer_id << "," << value.item_id << "," << value.sq_distance << "," << value.time << ")";

            return tmp.str();
        }
    };

    inline bool Comparator(const collision_detector::GatheringEvent& lhs, const collision_detector::GatheringEvent& rhs) {

        bool gatherer_id_check = (lhs.gatherer_id == rhs.gatherer_id);
        bool item_id_check = (lhs.item_id == rhs.item_id);

        bool distance_check = ((std::max(std::abs(lhs.sq_distance), std::abs(rhs.sq_distance))
            - std::min(std::abs(lhs.sq_distance), std::abs(rhs.sq_distance)) < __EPSILON__));

        bool time_check = ((std::max(std::abs(lhs.time), std::abs(rhs.time))
            - std::min(std::abs(lhs.time), std::abs(rhs.time)) < __EPSILON__));

        return gatherer_id_check && item_id_check && distance_check && time_check;
    }

    struct IsPermutationMatcher : Catch::Matchers::MatcherGenericBase {
        IsPermutationMatcher(std::vector<collision_detector::GatheringEvent> range)
            : range_{ std::move(range) } {
            std::sort(std::begin(range_), std::end(range_),
                [](const collision_detector::GatheringEvent& lhs, const collision_detector::GatheringEvent& rhs) {
                return lhs.time < rhs.time;
                });
        }
        IsPermutationMatcher(IsPermutationMatcher&&) = default;

        inline bool match(std::vector<collision_detector::GatheringEvent> other) const {
            using std::begin;
            using std::end;

            std::sort(begin(other), end(other), 
                [](const collision_detector::GatheringEvent& lhs, const collision_detector::GatheringEvent& rhs) {
                    return lhs.time < rhs.time;
                });


            return std::equal(begin(range_), end(range_), 
                begin(other), end(other), Comparator);
        }

        std::string describe() const override {
            // Описание свойства, проверяемого матчером:
            return "Is permutation of: "s + Catch::rangeToString(range_);
        }

    private:
        std::vector<collision_detector::GatheringEvent> range_;
    };

    IsPermutationMatcher IsPermutation(std::vector<collision_detector::GatheringEvent>&& range) {
        return IsPermutationMatcher{std::forward<std::vector<collision_detector::GatheringEvent>>(range)};
    }

}  // namespace Catch


using Items = std::vector<collision_detector::Item>;
using Gatherers = std::vector<collision_detector::Gatherer>;

class TestGameCollector : public collision_detector::ItemGathererProvider{
public:
    TestGameCollector() = default;

    TestGameCollector(const Items& items, const Gatherers& gatherers) 
        : items_(items), gatherers_(gatherers) {}

    TestGameCollector(Items&& items, Gatherers&& gatherers)
        : items_(std::move(items)), gatherers_(std::move(gatherers)) {}

    inline TestGameCollector& AddItem(const collision_detector::Item& item) {
        items_.push_back(item);
        return *this;
    }
    inline TestGameCollector& AddItem(collision_detector::Item&& item) {
        items_.push_back(std::move(item));
        return *this;
    }
    inline TestGameCollector& AddItem(double x, double y, double w) {
        items_.push_back(std::move(collision_detector::Item{ {x, y}, w }));
        return *this;
    }
    inline TestGameCollector& AddItems(const Items& items) {
        items_ = items;
        return *this;
    }
    inline TestGameCollector& AddItems(Items&& items) {
        items_ = std::move(items);
        return *this;
    }

    size_t ItemsCount() const override {
        return items_.size();
    }
    collision_detector::Item GetItem(size_t idx) const override {
        return items_[idx];
    }

    inline TestGameCollector& AddGatherer(const collision_detector::Gatherer& item) {
        gatherers_.push_back(item);
        return *this;
    }
    inline TestGameCollector& AddGatherer(collision_detector::Gatherer&& item) {
        gatherers_.push_back(std::move(item));
        return *this;
    }
    inline TestGameCollector& AddGatherer(geom::Point2D start, geom::Point2D end, double w) {
        gatherers_.push_back(std::move(collision_detector::Gatherer{ start, end, w }));
        return *this;
    }
    inline TestGameCollector& AddGatherer(double x1, double y1, double x2, double y2, double w) {
        gatherers_.push_back(std::move(collision_detector::Gatherer{ {x1, y1}, {x2, y2},w }));
        return *this;
    }
    inline TestGameCollector& AddGatherers(const Gatherers& gatherers) {
        gatherers_ = gatherers;
        return *this;
    }
    inline TestGameCollector& AddGatherers(Gatherers&& gatherers) {
        gatherers_ = std::move(gatherers);
        return *this;
    }

    size_t GatherersCount() const override {
        return gatherers_.size();
    }
    collision_detector::Gatherer GetGatherer(size_t idx) const override {
        return gatherers_[idx];
    }

private:
    Items items_;
    Gatherers gatherers_;
};

SCENARIO("TestGameCollector", "[TestGamCollector]") {

    using namespace collision_detector;

    GIVEN("A TestGameCollector") {

        TestGameCollector collector;

        GIVEN("a new Gatherer and Item") {

            Item item{ {0.0, 3.0}, 0.3 };
            Gatherer gatherer{ {0.0, 0.0}, {0.0, 12.0}, 0.5 };

            THEN("We can add their into collector, and check that") {
                collector
                    .AddItem(item)
                    .AddGatherer(gatherer);

                CHECK(collector.GatherersCount() == 1);
                CHECK(collector.ItemsCount() == 1);
            }
        }

        GIVEN("Two base data massive with 4 items and 2 gatherers") {

            Items items = { Item{{0.0, 3.0}, 0.3 }, Item{{0.0, 9.0}, 0.2 }, Item{{5.0, 0.0}, 0.2 }, Item{{12.0, 0.0}, 0.5 } };
            Gatherers gatherers = { Gatherer{{0.0, 0.0}, {0.0, 12.0}, 0.5}, Gatherer{{0.0, 0.0}, {15.0, 0.0}, 0.5} };

            THEN("We add data massives to collector, and check that") {
                collector
                    .AddItems(items)
                    .AddGatherers(gatherers);

                CHECK(collector.ItemsCount() == 4);
                CHECK(collector.GatherersCount() == 2);
                
                AND_THEN ("We can become same item by index") {

                    for (size_t i = 0; i != collector.ItemsCount(); ++i) {
                        CHECK(items[i] == collector.GetItem(i));
                    }
                }

                AND_THEN("We can become same gatherer by index") {

                    for (size_t i = 0; i != collector.GatherersCount(); ++i) {
                        CHECK(gatherers[i] == collector.GetGatherer(i));
                    }
                }
            }
        }
    }
}

SCENARIO("Collision Detector Check", "[ColDetCheck]") {

    using namespace collision_detector;

    GIVEN("A TestGameCollector") {

        TestGameCollector collector;

        GIVEN("Add one gatherer and one item in to collector") {

            collector
                .AddItem({ {0.0, 3.0}, 0.3 })
                .AddGatherer({ {0.0, 0.0}, {0.0, 12.0}, 0.5 });

            THEN("We check collision, must be one collision") {

                CHECK(!FindGatherEvents(collector).empty());
                CHECK(FindGatherEvents(collector).size() == 1);

                AND_THEN("We check FindGatherEvents result") {

                    CHECK(Catch::Comparator({ 0, 0, 0.0, 0.25 }, FindGatherEvents(collector)[0]));
                }
            }
        }

        GIVEN("Add one gatherer and one item with a coordinate difference of less than width + width in to collector") {

            collector
                .AddItem({ {0.7, 3.0}, 0.3 })
                .AddGatherer({ {0.0, 0.0}, {0.0, 12.0}, 0.5 });

            THEN("We check collision, must be one collision") {

                CHECK(!FindGatherEvents(collector).empty());
                CHECK(FindGatherEvents(collector).size() == 1);

                AND_THEN("We check FindGatherEvents result") {

                    CHECK(Catch::Comparator({ 0, 0, 0.49, 0.25 }, FindGatherEvents(collector)[0]));
                }
            }
        }

        GIVEN("Add one gatherer and one item with a coordinate difference equal to width + width in to collector") {

            collector
                .AddItem({ {0.8, 3.0}, 0.3 })
                .AddGatherer({ {0.0, 0.0}, {0.0, 12.0}, 0.5 });

            THEN("We check collision, must be no collision") {

                CHECK(FindGatherEvents(collector).empty());
            }
        }

        GIVEN("Add one gatherer and one item with a coordinate difference of greather than width + width in to collector") {

            collector
                .AddItem({ {0.9, 3.0}, 0.3 })
                .AddGatherer({ {0.0, 0.0}, {0.0, 12.0}, 0.5 });

            THEN("We check collision, must be no collision") {

                CHECK(FindGatherEvents(collector).empty());
            }
        }

        GIVEN("Two base data massive with 4 items and 2 gatherers, and add their in to collector") {

            Items items = { Item{{0.0, 3.0}, 0.3 }, Item{{0.0, 9.0}, 0.2 }, Item{{5.0, 0.0}, 0.2 }, Item{{12.0, 0.0}, 0.5 } };
            Gatherers gatherers = { Gatherer{{0.0, 0.0}, {0.0, 12.0}, 0.5}, Gatherer{{0.0, 0.0}, {15.0, 0.0}, 0.5} };

            collector
                .AddItems(items)
                .AddGatherers(gatherers);

            GIVEN("A match result reference") {
                std::vector<GatheringEvent> reference = { {0, 0, 0.0, 0.25}, {2, 1, 0.0, 0.333333}, {1, 0, 0.0, 0.75}, {3, 1, 0.0, 0.80} };

                THEN("We check collision, all items must be collected") {

                    std::vector<GatheringEvent> result = FindGatherEvents(collector);
                    CHECK(result.size() == 4);
                        
                    AND_THEN("We check result with reference") {
                        CHECK_THAT(reference, Catch::IsPermutation(std::move(result)));
                    }
                }
            }

            GIVEN("A unmatch result reference") {
                std::vector<GatheringEvent> reference = { {0, 0, 0.0, 0.25}, {2, 1, 0.0, 0.40}, {1, 0, 0.0, 0.75}, {3, 1, 0.0, 0.80} };

                THEN("We check collision, all items must be collected") {

                    std::vector<GatheringEvent> result = FindGatherEvents(collector);
                    CHECK(result.size() == 4);

                    AND_THEN("We check result with unmatch reference") {
                        CHECK_THAT(reference, !Catch::IsPermutation(std::move(result)));
                    }
                }
            }
        }
    }
}


// Напишите здесь тесты для функции collision_detector::FindGatherEvents