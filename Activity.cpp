#include "Activity.h"
#include <chrono>
#include <format>
// #include <algorithm>
#include <stdexcept>

using namespace std;
using namespace std::chrono;

// constexpr auto time_zone = -3h;
std::string tformat(int secs)
{
	int hours = secs/3600;
	int minutes = (secs/60) % 60;
	int seconds = secs % 60;
	return std::format("{:02d}:{:02d}:{:02d}", hours, minutes, seconds);
}

// --- ACTIVITY ----------------------------------------------

Activity::Activity(string name)
	: name_{std::move(name)}, created_{ Clock::now() }
{
}

Activity::~Activity()
{
	if (running_)
		stop();
}

bool Activity::started() const
{
	return (Clock::to_time_t(started_) != 0);
}

void Activity::start()
{
	if (!running_) {
		running_ = true;
		resumed_ = Clock::now();
		// is this the 1st time?
		if (!started())
			started_ = resumed_;
	}
}

void Activity::stop()
{
	if (running_) {
		// updates duration
		last_stopped_ = Clock::now();
		duration_ += duration_cast<Unit_t>(last_stopped_ - resumed_);
		running_ = false;

		add_history({resumed_, last_stopped_});
	}
}

void Activity::end()
{
	if (running_)
		stop();
	ended_ = Clock::now();
}

Unit_t Activity::duration() const
{
	if (running_)
		return duration_ + duration_cast<Unit_t>(Clock::now() - resumed_);
	return duration_;
}

Unit_t Activity::idle() const
{
	if (!started())
		return {};
	auto total_duration = duration_cast<Unit_t>(Clock::now() - started_);
	return total_duration - duration_;
}

void Activity::add_history(const std::pair<Time_t,Time_t>& h)
{
	history_.emplace_back(h);
}


// --- ACTIVITIES -------------------------------------------------------

void Activities::add(std::unique_ptr<Activity> a)
{
	activities_.emplace_back(std::move(a));
}

void Activities::add(const std::string& a)
{
	activities_.emplace_back(make_unique<Activity>(a));
}

Activity_ptr Activities::remove(int i)
{
	Activity_ptr removed;
	if (is_valid(i)) {
		removed = std::move(activities_[i]);
		activities_.erase(activities_.begin() + i);
	}
	return removed;
}

Activity_ptr Activities::remove_current()
{
	return remove(0);
}

optional<int> Activities::find(const std::string& a) const
{
	for (int i=0; i<size(); ++i)
		if (activities_[i]->name() == a)
			return i;
	return {};
}

const Activity& Activities::operator[](int i) const
{
	verify_range(i);
	return *activities_[i];
}

Activity& Activities::operator[](int i)
{
	verify_range(i);
	return *activities_[i];
}

bool Activities::is_valid(int i) const
{
	return (i>=0 && i <size());
}

const Activity& Activities::current() const
{
	if (empty())
		throw runtime_error{"no current activity"};
	return *activities_.front();
}

Activity& Activities::current()
{
	if (empty())
		throw runtime_error{"no current activity"};
	return *activities_.front();
}

// there can only be a single activity running in a given
// moment, and its index is 0
void Activities::start(int i)
{
	if (i == 0) {
		if (!activities_[0]->is_running())
			activities_[0]->start();
		return;		// is already the current
	}

	verify_range(i);
	// activities is not empty, because i is valid
	activities_[0]->stop(); // if running

	// put current on top
	auto a = std::move(activities_[i]);
	for (int j=i; j>0; --j)
		activities_[j] = std::move(activities_[j-1]);
	activities_[0] = std::move(a);
	activities_[0]->start();
}

void Activities::stop()
{
	if (!empty())
	 	activities_.front()->stop();
}

bool Activities::running() const
{
	if (empty())
		return false;
	return activities_.front()->is_running();
}

void Activities::verify_range(int i) const
{
	if (!is_valid(i))
		throw out_of_range{to_string(i)};
}
