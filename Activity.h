#ifndef ACTIVITIES_H
#define ACTIVITIES_H

#include <format>
#include <string>
#include <vector>
#include <chrono>
#include <optional>
#include <memory>
#include <list>
#include <utility>

using Clock  = std::chrono::system_clock;
using Unit_t = std::chrono::seconds;
using Time_t = std::chrono::time_point<Clock>;

// constexpr auto my_timezone = std::chrono::hours(-3);

class Activity {
public:
	Activity() = default;
	explicit Activity(std::string name);
	Activity(const Activity& a) = default;
	~Activity();
	void start();
	void stop();
	void end();

	// getters/setters
	[[nodiscard]] const std::string& name() const { return name_; }
	void name(const std::string& n) { name_ = n; }
	[[nodiscard]] bool started() const;
	[[nodiscard]] Unit_t duration() const;
	void duration(const Unit_t& d) { duration_ = d; }
	[[nodiscard]] Unit_t idle() const;
	[[nodiscard]] Time_t time_stopped() const { return last_stopped_; }
	void time_stopped(const Time_t& t) { last_stopped_ = t; }
	[[nodiscard]] Time_t time_started() const { return started_; }
	void time_started(const Time_t& t) { started_ = t; }
	[[nodiscard]] Time_t time_ended() const { return ended_; }
	void time_ended(const Time_t& t) { ended_ = t; }
	[[nodiscard]] Time_t time_created() const { return created_; }
	void time_created(const Time_t& t) { created_ = t; }
	[[nodiscard]] bool is_running() const { return running_; }
	[[nodiscard]] const auto& history() const { return history_; }
	void add_history(const std::pair<Time_t,Time_t>& h);
private:
	std::string name_;
	Time_t created_;	// time created
	Time_t started_;	// time first started
	Time_t ended_;	 	// time completed
	Time_t last_stopped_;
	Time_t resumed_;	// time of last resume
	Unit_t duration_{};	// current activity duration
	bool running_ = false;

	std::list<std::pair<Time_t,Time_t>> history_;	// history of running intervals
};

std::string sec_to_str(int secs); // seconds to XX:XX:XX
std::string fmt_localtime(const std::string& fmt, const Time_t& t);

template<>
struct std::formatter<Activity> : std::formatter<std::string> {
	template<typename Context>
	constexpr auto format(const Activity& a, Context& ctx) const {
		string hist;
		for (const auto& [start,end] : a.history())
			hist += std::format("     \t{} - {}\n",
				fmt_localtime("%d/%m/%Y %H:%M", start),
				fmt_localtime("%d/%m/%Y %H:%M", end));
		return format_to(ctx.out(), R"([{}]
------------
     CREATED = {}
     STARTED = {}
       ENDED = {}
    DURATION = {}
        IDLE = {}
     HISTORY:
{})",
		a.name(),
		fmt_localtime("%d/%m/%Y-%H:%M", a.time_created()),
		(a.started() ? fmt_localtime("%d/%m/%Y-%H:%M", a.time_started()) : "NEVER"),
		fmt_localtime("%d/%m/%Y-%H:%M", a.time_ended()),
		(a.started() ? sec_to_str(a.duration().count()) : "--"),
		(a.started() ? sec_to_str(a.idle().count()) : "--"),
		hist);
	}
};

using Activity_ptr = std::unique_ptr<Activity>;

// ----------------------------------------------------------

class Activities {
public:
	void add(Activity_ptr a);		// will take ownweship
	void add(const std::string& a);	// return index
	Activity_ptr remove(int i);
	Activity_ptr remove_current();
	[[nodiscard]] std::optional<int> find(const std::string& a) const;
	[[nodiscard]] const Activity& operator[](int i) const;
	[[nodiscard]] Activity& operator[](int i);
	[[nodiscard]] const Activity& current() const;
	[[nodiscard]] Activity& current();
	[[nodiscard]] auto size() const { return activities_.size(); }
	[[nodiscard]] bool empty() const { return activities_.empty(); }
	[[nodiscard]] bool running() const;
	[[nodiscard]] bool is_valid(int i) const;
	void clear() { activities_.clear(); }

	void start(int i);	// starts i, stops the previous running
	void stop(); 		// stop current
private:
	void verify_range(int i) const;
	std::vector<Activity_ptr> activities_;
};

#endif
