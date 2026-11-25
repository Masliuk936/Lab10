#pragma once
#include <string>

struct MeterReading
{
    int id = 0;
    int buildingId = 0;
    std::string readingDate;
    std::string meterType;
    double consumedValue = 0.0;
    std::string status = "Отримане";
};

struct ReadingStatusHistory
{
    int id = 0;
    int readingId = 0;
    std::string oldStatus;
    std::string newStatus;
    std::string changedAt;
};