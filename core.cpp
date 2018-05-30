// © Copyright (c) 2018 SqYtCO

#include "core.h"
#include <fstream>
#include <experimental/filesystem>
#include <iomanip>					// std::put_time
#include <chrono>
#include <sstream>

Core* Core::get_instance()
{
	static Core core;

	return &core;
}

Core::Core() : cells(nullptr), generation(0)
{
	new_system();
}

Core::~Core()
{
	delete cells;
}

bool Core::save(std::string file)
{
	if(file.empty())
	{
		try
		{
			// check if path exists and create it if not
			if(!std::experimental::filesystem::exists(config.get_save_path()))
				std::experimental::filesystem::create_directories(config.get_save_path());
		}
		catch(...)	// if write permission is not granted
		{
			return false;
		}

		// create string with current date
		std::time_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::string date;
		std::ostringstream oss;
		oss << std::put_time(std::localtime(&time), "%F_%H-%M");	// %y-%m-%d_%H-%M = YY-MM-DD_HH-min
		date = oss.str();

		// attached num if file exists already
		std::size_t save_num = 0;
		while(true)
		{
			// first try without num
			if(save_num == 0)
				file = config.get_save_path() + date + ".gol";
			else
				file = config.get_save_path() + date + "_" + std::to_string(save_num) + ".gol";

			// check if file exists already
			std::ifstream in(file);
			// if file does not exist, break loop
			if(!in)
				break;
			else
				++save_num;
		}
	}

	std::ofstream out(file);
	// if creating fails
	if(!out)
		return false;

	out << generation << '\n';

	for(std::size_t row = 0; row < config.get_size_y(); ++row)
	{
		for(std::size_t column = 0; column < config.get_size_x(); ++column)
		{
			out << static_cast<int>(cells->get_cell_state(column, row)) << ' ';
		}

		out << '\n';
	}

	// if writing fails
	if(!out)
		return false;

	return true;
}

bool Core::load(const std::string& file)
{
	// return on empty file
	if(file.empty())
		return false;

	// check if ending is equal to ".gol"; if not, return false
	if(file.substr(file.size() - 4, 4) != ".gol")
		return false;

	std::ifstream in(file);
	// if reading fails, return false
	if(!in)
		return false;

	// skip saved generation num
	in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// count size; size_x is determined by the longest row
	std::string temp_s;
	char temp_c;
	std::size_t size_x_counter = 0, size_x_max = 0, size_y = 0;
	while(in)
	{
		// read in line
		std::getline(in, temp_s);

		// count columns
		for(std::size_t i = 0; i < temp_s.size(); ++i)
		{
			temp_c = temp_s[i];

			// count only valid cell states
			if(temp_c == '1' || temp_c == '0')
				++size_x_counter;
		}

		// count (filles) rows
		if(!temp_s.empty())
			++size_y;

		// longest row (most columns) determinates size_x
		if(size_x_counter > size_x_max)
			size_x_max = size_x_counter;

		// reset column counter
		size_x_counter = 0;
	}

	// reopen file to reset read position and EOF-state
	in.close();
	in.open(file);

	// create new Cell_System
	delete cells;
	cells = new Cell_System(size_x_max, size_y, config.get_border_behavior(), config.get_live_rules(), config.get_reborn_rules(), config.get_num_of_threads());

	// read in saved generation
	std::size_t saved_generation;
	in >> saved_generation;

	// read in cell states
	std::size_t column = 0, row = 0;
	while(in)
	{
		// read in line
		std::getline(in, temp_s);

		// read states
		for(std::size_t i = 0; i < temp_s.size(); ++i)
		{
			temp_c = temp_s[i];

			// set cell to read state
			if(temp_c == '1' || temp_c == '0')
				cells->set_cell(column++, row, static_cast<Cell_State>(temp_c - '0'));
		}

		// count up rows
		if(!temp_s.empty())
			++row;

		column = 0;
	}

	// set current generation to read generation
	generation = saved_generation;
	// calc next generation
	cells->calc_next_generation();

	// return true on success
	return true;
}

void Core::new_system()
{
	if(cells)
		delete cells;

	cells = new Cell_System(config.get_size_x(), config.get_size_y(), config.get_border_behavior(),
							config.get_live_rules(), config.get_reborn_rules(), config.get_num_of_threads());
	if(config.get_start_random())
		cells->random_cells(config.get_relation_alive(), config.get_relation_dead());
	generation = 0;
}

const std::size_t& Core::next_generation()
{
	cells->next_generation();

	return ++generation;
}

void Core::calc_next_generation()
{
	cells->calc_next_generation();
}