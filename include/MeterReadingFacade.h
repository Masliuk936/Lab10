#pragma once
#include <memory>
#include "IBuildingRepository.h"
#include "ReadingState.h"
#include "DeviationStrategy.h"
#include "ReadingObserver.h"

class MeterReadingFacade
{
private:
    IBuildingRepository& repo;
    DeviationCalculator calculator;
    ReadingSubject subject;

public:
    MeterReadingFacade(IBuildingRepository& repository, std::unique_ptr<DeviationStrategy> strategy)
        : repo(repository), calculator(std::move(strategy)) {
    }

    void setDeviationStrategy(std::unique_ptr<DeviationStrategy> newStrategy)
    {
        calculator.setStrategy(std::move(newStrategy));
    }

    void subscribe(const std::shared_ptr<IReadingObserver>& observer)
    {
        subject.subscribe(observer);
    }

    void unsubscribe(const std::shared_ptr<IReadingObserver>& observer)
    {
        subject.unsubscribe(observer);
    }

    int processReading(MeterReading reading, double normalValue, double allowedDeviation, double maxDeviationLimit)
    {
        reading.status = "Отримане";
        int id = repo.addMeterReading(reading);

        subject.notify({ ReadingEventType::ReadingReceived, id, reading.meterType, reading.consumedValue, 0.0 });

        ReadingContext ctx(id, repo, std::make_unique<ReceivedState>());
        ctx.verify();

        double deviation = calculator.calculate(reading.consumedValue, normalValue, allowedDeviation);

        if (deviation > maxDeviationLimit)
        {
            ctx.markInvalid();
            subject.notify({ ReadingEventType::AnomalyDetected, id, reading.meterType, reading.consumedValue, deviation });
        }
        else
        {
            ctx.process();
        }

        return id;
    }
};