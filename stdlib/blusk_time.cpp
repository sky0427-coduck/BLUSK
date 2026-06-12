// =============================================================
//  BLUSK stdlib/blusk_time.cpp
// =============================================================
#include "blusk_time.h"
#include <chrono>
#include <thread>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace BluskStd::Time {

using namespace std::chrono;

static int64_t epochMs() {
    return duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()).count();
}
static double perfUs() {
    return (double)duration_cast<microseconds>(
        steady_clock::now().time_since_epoch()).count() / 1000.0;
}

Value now(const std::vector<Value>&) { return Value::Int(epochMs()); }
Value perfNow(const std::vector<Value>&) { return Value::Float(perfUs()); }

Value sleep(const std::vector<Value>& args) {
    int64_t ms = args.empty() ? 0 : args[0].toInt();
    std::this_thread::sleep_for(milliseconds(ms));
    return Value::Nil();
}

Value timestamp(const std::vector<Value>&) {
    auto t = system_clock::to_time_t(system_clock::now());
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
    return Value::String(oss.str());
}

Value date(const std::vector<Value>&) {
    auto t = system_clock::to_time_t(system_clock::now());
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&t), "%Y-%m-%d");
    return Value::String(oss.str());
}

Value elapsed(const std::vector<Value>& args) {
    int64_t start = args.empty() ? 0 : args[0].toInt();
    return Value::Int(epochMs() - start);
}

} // namespace BluskStd::Time
