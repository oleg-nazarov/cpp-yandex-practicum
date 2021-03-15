#pragma once
#include <chrono>
#include <iostream>
#include <string>

#define PROFILE_CONCAT_INTERNAL(X, Y) X##Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, __LINE__)
#define LOG_DURATION(func_name) LogDuration UNIQUE_VAR_NAME_PROFILE(func_name)
#define LOG_DURATION_STREAM(func_name, os) LogDuration UNIQUE_VAR_NAME_PROFILE(func_name, os)

class LogDuration {
   public:
    LogDuration(const std::string& id, std::ostream& os = std::cerr) : id_(id), os_(os) {}

    ~LogDuration() {
        using namespace std::chrono;

        const auto end_time = steady_clock::now();
        const auto dur = end_time - start_time_;
        os_ << id_ << ": " << duration_cast<milliseconds>(dur).count() << " ms" << std::endl;
    }

   private:
    const std::string id_;
    std::ostream& os_;
    const std::chrono::steady_clock::time_point start_time_ = std::chrono::steady_clock::now();
};