#include <cstdio>
#include <memory>
#include <stdexcept>
#include <gtest/gtest.h>

#include "Building.h"
#include "MeterReading.h"
#include "SQLiteBuildingRepository.h"
#include "ReadingState.h"
#include "DeviationStrategy.h"
#include "ReadingObserver.h"
#include "MeterReadingFacade.h"

TEST(StatePatternTest, TransitionsAndHistoryPersistence)
{
    const std::string dbFile = "test_state.db";
    std::remove(dbFile.c_str());
    SQLiteBuildingRepository repo(dbFile);

    int bId = repo.addBuilding({ 0, "вул. Тестова, 1", "Центральний", "Житловий" });
    int rId = repo.addMeterReading({ 0, bId, "2026-11-25", "Електроенергія", 250.0, "Отримане" });

    ReadingContext ctx(rId, repo, std::make_unique<ReceivedState>());

    EXPECT_THROW(ctx.process(), std::logic_error);
    EXPECT_EQ(ctx.getStateName(), "Отримане");

    ctx.verify();
    EXPECT_EQ(ctx.getStateName(), "Перевірене");

    ctx.process();
    EXPECT_EQ(ctx.getStateName(), "Оброблене");
    EXPECT_THROW(ctx.markInvalid(), std::logic_error);

    auto saved = repo.getReadingById(rId);
    ASSERT_TRUE(saved.has_value());
    EXPECT_EQ(saved->status, "Оброблене");

    auto history = repo.getStatusHistory(rId);
    ASSERT_EQ(history.size(), 2);
    EXPECT_EQ(history[0].oldStatus, "Отримане");
    EXPECT_EQ(history[0].newStatus, "Перевірене");
    EXPECT_EQ(history[1].oldStatus, "Перевірене");
    EXPECT_EQ(history[1].newStatus, "Оброблене");
}

TEST(StrategyPatternTest, DynamicSwitchingAndCalculations)
{
    DeviationCalculator calc(std::make_unique<AbsoluteDeviationStrategy>());
    EXPECT_NEAR(calc.calculate(240.0, 200.0, 50.0), 40.0, 1e-6);

    calc.setStrategy(std::make_unique<RelativeDeviationStrategy>());
    EXPECT_NEAR(calc.calculate(240.0, 200.0, 50.0), 0.2, 1e-6);

    calc.setStrategy(std::make_unique<NormalizedDeviationStrategy>());
    EXPECT_NEAR(calc.calculate(240.0, 200.0, 50.0), 0.8, 1e-6);
}

TEST(ObserverPatternTest, SubscribeUnsubscribeAndEvents)
{
    ReadingSubject subject;
    auto logger = std::make_shared<ReadingLogger>();
    auto stats = std::make_shared<ConsumptionStats>();

    subject.subscribe(logger);
    subject.subscribe(stats);

    subject.notify({ ReadingEventType::ReadingReceived, 1, "Вода", 15.0, 0.0 });
    EXPECT_EQ(stats->receivedCount, 1);
    EXPECT_EQ(logger->logs.size(), 1);

    subject.unsubscribe(logger);
    subject.notify({ ReadingEventType::AnomalyDetected, 1, "Вода", 95.0, 80.0 });
    EXPECT_EQ(stats->anomaliesCount, 1);
    EXPECT_EQ(logger->logs.size(), 1);
}

TEST(FacadePatternTest, FullReadingProcessingWorkflow)
{
    const std::string dbFile = "test_facade.db";
    std::remove(dbFile.c_str());
    SQLiteBuildingRepository repo(dbFile);

    int bId = repo.addBuilding({ 0, "вул. Фасадна, 5", "Північний", "Комерційний" });

    MeterReadingFacade facade(repo, std::make_unique<NormalizedDeviationStrategy>());
    auto stats = std::make_shared<ConsumptionStats>();
    facade.subscribe(stats);

    int okId = facade.processReading({ 0, bId, "2026-11-25", "Вода", 12.0 }, 10.0, 5.0, 1.0);
    EXPECT_EQ(repo.getReadingById(okId)->status, "Оброблене");

    int badId = facade.processReading({ 0, bId, "2026-11-25", "Вода", 25.0 }, 10.0, 5.0, 1.0);
    EXPECT_EQ(repo.getReadingById(badId)->status, "Некоректне");

    EXPECT_EQ(stats->receivedCount, 2);
    EXPECT_EQ(stats->anomaliesCount, 1);
}