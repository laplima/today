//
// TODAY
// Copyright (C) 2025 by Luiz Lima Jr.
// All rights reserved.
//

#include <chrono>
#include <print>
#include <iostream>
#include <string>
#include <span>
#include <csignal>
#include <cstdlib>
#include <unistd.h>		// sleep better than this_thread::sleep_for()
#include <stack>
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <fmt/color.h>
#include "Activity.h"

using namespace std::chrono;
using json = nlohmann::json;
namespace fs = std::filesystem;

const fs::path actfile = "activities.json";
constexpr auto my_timezone = -3h;

enum class SelectionType : char {
	none,
	nextactivity,
	resumelast,
	newactivity,
	delactivity,
	exit
};

struct Selection {
	explicit Selection(SelectionType t) : type{t} {}
	Selection(SelectionType t, int v) : type{t}, value{v} {}
	operator SelectionType() const { return type; }
 	SelectionType type = SelectionType::none;
 	int value = 0;
};

fs::path make_dbpath(const fs::path& file_name);

std::string input(const std::string& prompt);

std::atomic<bool> chronometer_on{false};
// bool chronometer_on = false;

void handler(int ns);
void chronometer(const Activity& a);
void list_activities(const Activities& acts);
void read_new_activities(Activities& acts);
bool read_db(const fs::path& p, Activities& activities);
bool save_db(const fs::path& p, const Activities& activities);

// json helpers
void to_json(json& j, const Activity& a);
void from_json(const json& j, Activity& a);

// ---------- MAIN PROGRAM -----------------------------------------------------

int main(int argc, char* argv[])
{
	using namespace std;
	span args(argv, argc);

	// time zone, though specified (C++20) is not yet implemente?
	//
	// auto tz = std::chrono::current_zone();
	// auto local = tz->to_local(Clock::now());

	auto today = Clock::now();
	println("\nTODAY - {:%d/%m/%Y}\n(C)2025 Luiz Lima Jr.\n", today + my_timezone);
	
	Activities activities;
	stack<Activity_ptr> completed_activities;

	// read activities from file
	auto dbfile = make_dbpath(actfile);

	if (read_db(dbfile, activities))
		println("* Incomplete activities read");
	else
		read_new_activities(activities);

	auto select_activity = [&activities]() {
		list_activities(activities);
		auto istr = input("[#] [ENTER] [n]ew [d]elete e[x]it: ");
		if (istr.empty())
			return Selection{SelectionType::resumelast};
		if (istr.starts_with("n"))
			return Selection{SelectionType::newactivity};
		if (istr.starts_with("d")) {
			auto s = istr.substr(1).empty() ? 0 : stoi(istr.substr(1));
			return Selection(SelectionType::delactivity, s - 1);
		}
		if (istr.starts_with("x"))
			return Selection{SelectionType::exit};

		try {
		    auto i = std::clamp(stoi(istr) - 1, 0,
		                        static_cast<int>(activities.size()) - 1);
		    return Selection{SelectionType::nextactivity, i};
		} catch (const std::exception&) {
		    return Selection{SelectionType::none};
		}
	};

	auto selection = select_activity();

	while (!activities.empty()) {

		if (selection == SelectionType::exit) break;	// exit
		if (selection == SelectionType::resumelast) {
			// resume last activity
			selection = Selection{SelectionType::nextactivity, 0};
		} else if (selection == SelectionType::delactivity) {
			// delete activity
			auto del = activities.remove(selection.value);
			del->end();
			println("\n{}", *del);
			completed_activities.push(std::move(del));
			if (activities.empty()) break;
		} else if (selection == SelectionType::newactivity) {
			// new activity
			println();
			read_new_activities(activities);
		}

		if (selection == SelectionType::nextactivity) {
			// start next activity
			activities.start(selection.value);
			auto& curr = activities.current();
			println("\n{:%d/%m/%Y-%H:%M} (created)",
				curr.time_created() + my_timezone);
			if (Clock::to_time_t(curr.time_stopped()) != 0)
				println("{:%d/%m/%Y-%H:%M} (last stopped)",
					curr.time_stopped() + my_timezone);
			println("{}", string(curr.name().size(), '-'));
			print(fmt::emphasis::bold | fg(fmt::color::white), "{}\n", curr.name());
			println("{}", string(curr.name().size(), '-'));

			chronometer(curr);

			activities.stop();

			auto resp = input(format("\n\nIs \"{}\" completed? ", curr.name()));
			if (resp.starts_with("y") || resp.starts_with("Y")) {
				// complete current activity
				auto old = activities.remove(0);
				old->end();
				println("\n{}", *old);
				completed_activities.push(std::move(old));
				if (activities.empty()) break;
			}
		}

		selection = select_activity();
	}

	println();
	if (!activities.empty()) {
		print("* Saving incomplete activities... ");
		if (save_db(dbfile, activities))
			println("OK");
		else
			println("FAIL");
	} else {
		if (fs::exists(dbfile)) {
			print("* All activities completed, removing database... ");
			fs::remove(dbfile);
			println("OK");
		}
	}

	fs::path home_path;
	const auto* home = getenv("HOME");
	if (home != nullptr)
		home_path = home;
	else {
		home = getenv("HOMEDRIVE");
		if (home != nullptr) {
			home_path = home;
			home = getenv("HOMEPATH");
			if (home != nullptr)
				home_path /= home;
			else
				home_path = ".";
		} else
			home_path = ".";
	}

	ofstream history{home_path / "today.txt", ios::app};
	if (!completed_activities.empty()) {
		println("* Completed today:\n");
		while (!completed_activities.empty()) {
			auto a = std::move(completed_activities.top());
			completed_activities.pop();
			println("{}",*a);
			println(history, "{}", *a);
			println("-------------------");
		}
	}
	history.close();
	println();
}

// ---------------------------------

std::string input(const std::string& prompt)
{
	using namespace std;
	print("{}", prompt);
	std::string resp;
	getline(cin, resp);
	return resp;
}

fs::path make_dbpath(const fs::path& file_name)
{
	// read activities from file
	fs::path home = getenv("HOME");
	fs::path db_dir = home / "Library" / "Application Support" / "today";
	fs::create_directories(db_dir);
	return db_dir / file_name;
}

void handler(int ns)
{
	// switch the cursor on
	// system("tput cnorm");
	std::print("\x1b[?25h");
	signal(SIGINT, SIG_DFL);

	std::println("{}[2D  ", (char)0x1b);	// clear ^C
	chronometer_on = false;
}

void chronometer(const Activity& a)
{
	// cannot be affected by system sleep, so using only a counter wouldn't do
	signal(SIGINT, handler);

	// switch the cursor off
	// system("tput civis");
	std::print("\x1b[?25l");

	auto count = a.duration().count();
	Time_t start_chono = Clock::system_clock::now();
	chronometer_on = true;
	while (chronometer_on) {
		auto d = duration_cast<Unit_t>(Clock::now() - start_chono).count();
		print(fg(fmt::color::green), "{}\n", tformat(d+count));
		print(fmt::emphasis::bold | fg(fmt::color::green_yellow), "{}\n", tformat(d));

		// beep
		auto sec = d%60;
		auto min = static_cast<int>(d/60)%60;
		if (min%15 == 0 && sec == 0) {
			if (min != 0) {
				auto p = fork();
				if (p==0) {
#ifdef __APPLE__
					execl("/usr/bin/afplay", "afplay",
						"/Users/laplima/Music/Sounds/pop.mp3", nullptr);
#endif
					// std::println(stderr, "tic failed! {}", error);
					_exit(1); // just in case - avoids flushing stdio
				}
				waitpid(p, nullptr, WNOHANG);	// prevents zombies + doesn't hang up parent
			}
		}
		// std::this_thread::sleep_for(1s);
		sleep(1);	// this one is interrupted immediately
		std::print("{}[8D{}[2A", (char)0x1b, (char)0x1b);
	}
}

void list_activities(const Activities& acts)
{
	using std::println;
	println();
	for (int i=0; i<acts.size(); ++i)
		if (acts[i].started())
			println("({}) {}", i+1, acts[i].name());
		else
			println("[{}] {}", i+1, acts[i].name());
	println();
}

void read_new_activities(Activities& acts)
{
	auto a = input("New activity: ");
	while (!a.empty()) {
		acts.add(a);
		a = input("New activity: ");
	}
}

bool read_db(const fs::path& p, Activities& activities)
{
	using namespace std;

	ifstream f{p};
	if (!f)
		return false;

	activities.clear();
	json jacts = json::parse(f);
	for (const json& a : jacts) {
		auto act = make_unique<Activity>();
		from_json(a, *act);
		activities.add(std::move(act));
	}
	return !activities.empty();
}

bool save_db(const fs::path& p, const Activities& activities)
{
	json jacts = json::array();
	for (int i=0; i<activities.size(); ++i)
		jacts.push_back(activities[i]);
	std::ofstream out{p};
	if (!out)
		return false;
	out << jacts.dump(2) << '\n';
	return true;
}

void to_json(json& j, const Activity& a)
{
	j = json{
		{"name", a.name()},
		{"created", Clock::to_time_t(a.time_created())},
		{"started", Clock::to_time_t(a.time_started())},
		{"stopped", Clock::to_time_t(a.time_stopped())},
		{"ended", Clock::to_time_t(a.time_ended())},
		{"duration", a.duration().count()}
	};
	// add history
	j["history"] = json::array();
	for (const auto& [s,e] : a.history()) {
		json h = {
			{ "start", Clock::to_time_t(s) },
			{ "end", Clock::to_time_t(e) }
		};
		j["history"].emplace_back(h);
	}
}

void from_json(const json& j, Activity& a)
{
	using namespace std;
	using namespace std::chrono;
	string aux;
    j.at("name").get_to(aux);
    a.name(aux);
    time_t t{};
    j.at("created").get_to(t);
    a.time_created(system_clock::from_time_t(t));
    j.at("started").get_to(t);
    a.time_started(system_clock::from_time_t(t));
    j.at("stopped").get_to(t);
    a.time_stopped(system_clock::from_time_t(t));
    j.at("ended").get_to(t);
    a.time_ended(system_clock::from_time_t(t));
    long long d{};
    j.at("duration").get_to(d);
    a.duration(Unit_t{d});

    // history
    for (const auto& h : j.at("history"))
    	a.add_history({
    		system_clock::from_time_t(h["start"]),
    		system_clock::from_time_t(h["end"])
    	});
}
