#include <cstdio>
#include <iostream>
#include <memory>
#include <windows.h>

#include "Building.h"
#include "MeterReading.h"
#include "SQLiteBuildingRepository.h"
#include "ReadingState.h"
#include "DeviationStrategy.h"
#include "ReadingObserver.h"
#include "MeterReadingFacade.h"

int main()
{
    SetConsoleOutputCP(65001);

    const std::string appDb = "buildings_lr10.db";
    std::remove(appDb.c_str());

    SQLiteBuildingRepository repo(appDb);
    int b1 = repo.addBuilding({ 0, "вул. Шевченка, 12", "Центральний", "Житловий" });
    int b2 = repo.addBuilding({ 0, "пр. Перемоги, 45", "Північний", "Комерційний" });

    auto logger = std::make_shared<ReadingLogger>();
    auto stats = std::make_shared<ConsumptionStats>();

    MeterReadingFacade facade(repo, std::make_unique<AbsoluteDeviationStrategy>());
    facade.subscribe(logger);
    facade.subscribe(stats);

    std::cout << "\n--- 1. Стратегія: абсолютне відхилення (поріг = 50.0) ---\n";
    int id1 = facade.processReading({ 0, b1, "2026-11-20", "Електроенергія", 230.0 }, 200.0, 50.0, 50.0);
    int id2 = facade.processReading({ 0, b1, "2026-11-21", "Електроенергія", 285.0 }, 200.0, 50.0, 50.0);

    std::cout << "\n--- 2. Стратегія: відносне відхилення (поріг = 0.25) ---\n";
    facade.setDeviationStrategy(std::make_unique<RelativeDeviationStrategy>());
    int id3 = facade.processReading({ 0, b1, "2026-11-22", "Вода", 16.5 }, 15.0, 3.0, 0.25);
    int id4 = facade.processReading({ 0, b2, "2026-11-22", "Вода", 24.0 }, 15.0, 3.0, 0.25);

    std::cout << "\n--- 3. Стратегія: нормоване відхилення (поріг = 1.0) + відписка Logger ---\n";
    facade.setDeviationStrategy(std::make_unique<NormalizedDeviationStrategy>());
    facade.unsubscribe(logger);
    std::cout << "(Підписник ReadingLogger відключений — у консоль логи не виводяться)\n";
    int id5 = facade.processReading({ 0, b2, "2026-11-23", "Газ", 115.0 }, 80.0, 15.0, 1.0);

    std::cout << "\n--- 4. Перевірка захисту переходів патерну State ---\n";
    int rawId = repo.addMeterReading({ 0, b1, "2026-11-24", "Газ", 75.0, "Отримане" });
    ReadingContext manualCtx(rawId, repo, std::make_unique<ReceivedState>());
    std::cout << "Поточний стан запису ID=" << rawId << ": " << manualCtx.getStateName() << "\n";
    try
    {
        std::cout << "Спроба перевести в 'Оброблене' без перевірки: ";
        manualCtx.process();
    }
    catch (const std::logic_error& e)
    {
        std::cout << "ВІДХИЛЕНО (" << e.what() << ")\n";
    }
    manualCtx.verify();
    manualCtx.process();
    std::cout << "Після коректної послідовності переходів: " << manualCtx.getStateName() << "\n";

    std::cout << "\n--- 5. Підсумкова статистика ---\n";
    std::cout << "  Усього надійшло показань через фасад: " << stats->receivedCount << "\n";
    std::cout << "  Виявлено аномалій: " << stats->anomaliesCount << "\n";

    std::cout << "\n--- 6. Збережені показання у базі даних SQLite ---\n";
    for (const auto& b : repo.getAllBuildings())
    {
        std::cout << "Будівля: " << b.address << " (" << b.buildingType << ")\n";
        for (const auto& r : repo.getReadingsByBuildingId(b.id))
        {
            std::cout << "  [ID=" << r.id << "] " << r.readingDate << " | "
                << r.meterType << " | " << r.consumedValue
                << " -> Статус: " << r.status << "\n";
        }
    }

    std::cout << "\n--- 7. Історія переходів станів у БД для аномального запису ID=" << id5 << " ---\n";
    for (const auto& h : repo.getStatusHistory(id5))
    {
        std::cout << "  " << h.oldStatus << " -> " << h.newStatus << " (" << h.changedAt << ")\n";
    }

    return 0;
}