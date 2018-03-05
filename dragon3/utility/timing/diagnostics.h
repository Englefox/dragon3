#ifndef UTILITY_DIAGNOSTICS_H
#define UTILITY_DIAGNOSTICS_H

#include <iostream>
#include <string>
#include <intrin.h>
#include "../std_types.h"

#define CLOCK(name) diagnostics::closk clock##name(#name)

namespace diagnostics
{
	/*
	Clock is a general purpose CPU clock timer that automatically
	destructs itself based on variable scope. This way there is no
	need to keep track of beginning and ending the cpu timer; simply
	create an instance of clock and it will automatically diagnose
	the respective block.
	*/
	class clock
	{
	private:
		std::string m_Name;
		u64         m_StartClock;
	public:
		clock(std::string name)
		{
			m_Name = name;
			m_StartClock = (u64)__rdtsc();
		}
		float runningTime()
		{
			u64 endClock = (u64)__rdtsc();
			u64 delta = endClock - m_StartClock;
			const u32 clockSpeed = 3600000000;
			float clockTime = (float)delta / (float)clockSpeed * 1000.0f;
			return clockTime;
		}
		~clock()
		{
			u64 endClock = (u64)__rdtsc();
			u64 delta = endClock - m_StartClock;
			//clock is mostly used for internal time diagnosis, so for ease of use I simply 
			//directly set my CPU's clock speed (requesting this requires OS specific calls). If you want
			//to diagnose your own syste, simply replace the value below with your own CPU's clock speed
			//or use something like windows' QueryPerformanceFrequency to retrieve the clock speed.
			const u32 clockSpeed = 3600000000;
			float clockTime = (float)delta / (float)clockSpeed * 1000.0f;
			std::cout << m_Name << ":  " << delta << " cycles | " << clockTime << " ms" << std::endl;
		}
	};
}
#endif
