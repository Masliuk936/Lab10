#pragma once
#include <memory>
#include <stdexcept>
#include <string>
#include "IBuildingRepository.h"

class ReadingContext;

class ReadingState
{
public:
    virtual ~ReadingState() = default;
    virtual void verify(ReadingContext& ctx) = 0;
    virtual void process(ReadingContext& ctx) = 0;
    virtual void markInvalid(ReadingContext& ctx) = 0;
    virtual std::string name() const = 0;
};

class ReadingContext
{
private:
    int readingId;
    IBuildingRepository& repo;
    std::unique_ptr<ReadingState> state;

public:
    ReadingContext(int id, IBuildingRepository& repository, std::unique_ptr<ReadingState> initialState)
        : readingId(id), repo(repository), state(std::move(initialState)) {
    }

    void transitionTo(std::unique_ptr<ReadingState> newState, const std::string& timestamp = "2026-11-25 12:00")
    {
        std::string nextName = newState->name();
        repo.updateReadingStatusWithHistory(readingId, nextName, timestamp);
        state = std::move(newState);
    }

    void verify() { state->verify(*this); }
    void process() { state->process(*this); }
    void markInvalid() { state->markInvalid(*this); }
    std::string getStateName() const { return state->name(); }
};

class ProcessedState : public ReadingState
{
public:
    void verify(ReadingContext&) override { throw std::logic_error("Показання вже оброблене"); }
    void process(ReadingContext&) override { throw std::logic_error("Показання вже оброблене"); }
    void markInvalid(ReadingContext&) override { throw std::logic_error("Неможливо скасувати оброблене показання"); }
    std::string name() const override { return "Оброблене"; }
};

class InvalidState : public ReadingState
{
public:
    void verify(ReadingContext&) override { throw std::logic_error("Некоректне показання не можна перевірити"); }
    void process(ReadingContext&) override { throw std::logic_error("Некоректне показання не можна обробити"); }
    void markInvalid(ReadingContext&) override { throw std::logic_error("Показання вже має статус Некоректне"); }
    std::string name() const override { return "Некоректне"; }
};

class VerifiedState : public ReadingState
{
public:
    void verify(ReadingContext&) override { throw std::logic_error("Показання вже перевірене"); }
    void process(ReadingContext& ctx) override { ctx.transitionTo(std::make_unique<ProcessedState>()); }
    void markInvalid(ReadingContext& ctx) override { ctx.transitionTo(std::make_unique<InvalidState>()); }
    std::string name() const override { return "Перевірене"; }
};

class ReceivedState : public ReadingState
{
public:
    void verify(ReadingContext& ctx) override { ctx.transitionTo(std::make_unique<VerifiedState>()); }
    void process(ReadingContext&) override { throw std::logic_error("Спочатку потрібно перевірити показання"); }
    void markInvalid(ReadingContext& ctx) override { ctx.transitionTo(std::make_unique<InvalidState>()); }
    std::string name() const override { return "Отримане"; }
};