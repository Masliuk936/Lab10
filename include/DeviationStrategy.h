#pragma once
#include <cmath>
#include <memory>

class DeviationStrategy
{
public:
    virtual ~DeviationStrategy() = default;
    virtual double calculate(double consumed, double normal, double allowedDeviation) const = 0;
};

class AbsoluteDeviationStrategy : public DeviationStrategy
{
public:
    double calculate(double consumed, double normal, double) const override
    {
        return std::abs(consumed - normal);
    }
};

class RelativeDeviationStrategy : public DeviationStrategy
{
public:
    double calculate(double consumed, double normal, double) const override
    {
        if (normal == 0.0) return 0.0;
        return std::abs(consumed - normal) / normal;
    }
};

class NormalizedDeviationStrategy : public DeviationStrategy
{
public:
    double calculate(double consumed, double normal, double allowedDeviation) const override
    {
        if (allowedDeviation == 0.0) return 0.0;
        return std::abs(consumed - normal) / allowedDeviation;
    }
};

class DeviationCalculator
{
private:
    std::unique_ptr<DeviationStrategy> strategy;

public:
    explicit DeviationCalculator(std::unique_ptr<DeviationStrategy> initialStrategy)
        : strategy(std::move(initialStrategy)) {
    }

    void setStrategy(std::unique_ptr<DeviationStrategy> newStrategy)
    {
        strategy = std::move(newStrategy);
    }

    double calculate(double consumed, double normal, double allowedDeviation) const
    {
        return strategy->calculate(consumed, normal, allowedDeviation);
    }
};