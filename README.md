# TODAY

This app tracks your daily tasks and helps you manage your time efficiently.

Type in all the tasks you have for **today**. Choose a task to execute and type CTRL-C to pause it, mark it as finished, or switch to another task. A report of completed tasks is saved in your "home" directory.

![today](images/today.png)

* `#` –  Type the number of the activity to start it.
* `ENTER` starts the first activity. 
* Type `n` to enter new activities. 
* Type `d` and the number of the activity to delete it (for instance, `d2` deletes task 2 in the list). 
* Type `x` to exit the application (incompleted tasks are automatically saved).

## Features

- Add, execute, and delete tasks
- Mark tasks as completed
- Track time spent in each task.

## Compiling and Running the Application

#### Library dependencies

* [nlohmann/json](https://github.com/nlohmann/json) – JSON library
* [libfmt](https://github.com/fmtlib/fmt) – just for text coloring

C++ 23 required.

#### Compile with (using CMake/Ninja):

```bash
cd today
cmake --preset=ninja --fresh
cmake --build build --clean-first
```

`make` preset also available.

#### Run the application with:

```bash
build/today
```

The activity list will be saved in the file `activities.json` located at `~/Library/Application Support/today` (on macOS).

### Future work

* [ ] Automatically set time zone (currently, -3).
* [ ] Save the activities file in the current system's configuration folder (currently, it is saved in macOS' application support directory).
* [ ] Command-line parameters (activity file location, custom time zone, sound alarms, etc.)
* [ ] Set deadlines and reminders.
* [ ] Organize tasks by categories
* [ ] Sync tasks across devices
* [ ] User-friendly interface

## Author

Copyright (C) 2025 by Luiz A. de P. Lima Jr.
