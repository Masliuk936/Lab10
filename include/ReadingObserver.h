#pragma once
#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

enum class ReadingEventType
{
    ReadingReceived,
    AnomalyDetected
};

struct ReadingEvent
{
    ReadingEventType type;
    int readingId = 0;
    std::string meterType;
    double value = 0.0;
    double deviation = 0.0;
};

class IReadingObserver
{
public:
    virtual ~IReadingObserver() = default;
    virtual void update(const ReadingEvent& event) = 0;
};

class ReadingSubject
{
private:
    std::vector<std::weak_ptr<IReadingObserver>> observers;

public:
    void subscribe(const std::shared_ptr<IReadingObserver>& observer)
    {
        observers.push_back(observer);
    }

    void unsubscribe(const std::shared_ptr<IReadingObserver>& observer)
    {
        std::erase_if(observers, [&](const std::weak_ptr<IReadingObserver>& item) {
            auto ptr = item.lock();
            return !ptr || ptr == observer;
            });
    }

    void notify(const ReadingEvent& event)
    {
        std::erase_if(observers, [](const std::weak_ptr<IReadingObserver>& item) {
            return item.expired();
            });

        for (const auto& item : observers)
        {
            if (auto obs = item.lock())
            {
                obs->update(event);
            }
        }
    }
};

class ReadingLogger : public IReadingObserver
{
public:
    std::vector<std::string> logs;

    void update(const ReadingEvent& event) override
    {
        std::string msg = (event.type == ReadingEventType::ReadingReceived)
            ? "[Журнал] Надійшло показання ID=" + std::to_string(event.readingId) + " (" + event.meterType + ")"
            : "[Журнал] АНОМАЛІЯ у показанні ID=" + std::to_string(event.readingId) + ", відхилення=" + std::to_string(event.deviation);
        logs.push_back(msg);
        std::cout << msg << '\n';
    }
};

class ConsumptionStats : public IReadingObserver
{
public:
    int receivedCount = 0;
    int anomaliesCount = 0;

    void update(const ReadingEvent& event) override
    {
        if (event.type == ReadingEventType::ReadingReceived)
        {
            receivedCount++;
        }
        else if (event.type == ReadingEventType::AnomalyDetected)
        {
            anomaliesCount++;
        }
    }
};