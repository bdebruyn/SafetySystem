// SafetyBox.h
#pragma once
#include <iostream>

class SafetyBox
{
public:
	SafetyBox() : mode(0), step(0) 
	{
		std::cout << "System initialized (off)" << std::endl;
	}

	// One big "driver" for commands
	// 0=powerOn, 1=powerOff, 2=fault, 3=start, 4=doorOpened, 5=plateArrived, 6=doorClosed
	void run(int cmd)
	{
		if (cmd == 0)
		{
			if (mode == 0 || mode == 2)
			{
				mode = 1; step = 0;
				std::cout << "Power on" << std::endl;
			}
		}
		else if (cmd == 1)
		{
			mode = 0; step = 0;
			std::cout << "Power off" << std::endl;
		}
		else if (cmd == 2)
		{
			mode = 2; step = 0;
			std::cout << "Fault detected!" << std::endl;
		}
		else if (cmd == 3)
		{
			if (mode == 1 && step == 0)
			{
				std::cout << "Loader started" << std::endl;
				std::cout << "Request door open" << std::endl;
				step = 1;
			}
		}
		else if (cmd == 4)
		{
			if (mode == 1 && step == 1)
			{
				std::cout << "Door opened, loading plate" << std::endl;
				step = 2;
			}
		}
		else if (cmd == 5)
		{
			if (mode == 1 && step == 2)
			{
				std::cout << "Plate arrived, closing door" << std::endl;
				step = 3;
			}
		}
		else if (cmd == 6)
		{
			if (mode == 1 && step == 3)
			{
				std::cout << "Door closed, workflow complete" << std::endl;
				step = 0;
			}
		}
		else
		{
			std::cout << "Unknown command" << std::endl;
		}
	}

	// Quick status dump (ugly procedural style)
	void dump()
	{
		std::cout << "[Mode=" << mode << " Step=" << step << "]" << std::endl;
	}

private:
	int mode; // 0=off, 1=on, 2=fault
	int step; // 0=idle, 1=waiting-open, 2=waiting-plate, 3=waiting-close
};

