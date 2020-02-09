#include "MyClass.hpp"
#include <wiringPi.h>

#include <utility>
#include <memory>
#include <iostream>
#include <vector>
#include <string>

auto initialize()
{
	if(wiringPiSetup() != 0) {
		std::cerr << "wiringPiSetup() error" << std::endl; 
		std::exit(EXIT_FAILURE);
	}
}

int main(int argc, char* argv[])
{
	initialize();

	std::vector<std::string> args;
	args.reserve(argc);
	for(auto i = 0u; i < static_cast<uint>(argc); ++i)
		args.push_back(argv[i]);

	for(auto it : args)
		std::cout << it << std::endl;

}