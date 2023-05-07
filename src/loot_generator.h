#pragma once
#include <chrono>
#include <functional>

namespace loot_gen {

    struct LootGeneratorConfig {
        double period_ = 0.0;            // период генерации задается в СЕКУНДАХ!!!!!
        double probability_ = 0.0;       // вероятность генерации лута за указанный период

        LootGeneratorConfig& SetPeriod(double);
        LootGeneratorConfig& SetProbability(double);

        double GetPeriod() const {
            return period_;
        }
        double GetProbability() const {
            return probability_;
        }
    };

    using RandomGenerator = std::function<double()>;
    using TimeInterval = std::chrono::milliseconds;

    /*
     *  Генератор трофеев
     */
    class LootGenerator {
    public:
        LootGenerator(LootGeneratorConfig config, RandomGenerator random_gen = DefaultGenerator)
            : base_interval_{ static_cast<int>(config.GetPeriod() * 1000) }
            , probability_{ config.probability_ }
            , random_generator_{ std::move(random_gen)} {
        }

        /*
         * base_interval - базовый отрезок времени > 0
         * probability - вероятность появления трофея в течение базового интервала времени
         * random_generator - генератор псевдослучайных чисел в диапазоне от [0 до 1]
         */
        LootGenerator(TimeInterval base_interval, double probability,
                      RandomGenerator random_gen = DefaultGenerator)
            : base_interval_{base_interval}
            , probability_{probability}
            , random_generator_{std::move(random_gen)} {
        }

        /*
         * Возвращает количество трофеев, которые должны появиться на карте спустя
         * заданный промежуток времени.
         * Количество трофеев, появляющихся на карте не превышает количество мародёров.
         *
         * time_delta - отрезок времени, прошедший с момента предыдущего вызова Generate
         * loot_count - количество трофеев на карте до вызова Generate
         * looter_count - количество мародёров на карте
         */
        unsigned Generate(TimeInterval time_delta, unsigned loot_count, unsigned looter_count);

    private:
        static double DefaultGenerator() noexcept {
            return 1.0;
        };
        TimeInterval base_interval_;
        double probability_;
        TimeInterval time_without_loot_{};
        RandomGenerator random_generator_;
    };

}  // namespace loot_gen