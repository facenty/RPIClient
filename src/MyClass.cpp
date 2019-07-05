#include "MyClass.hpp"

#include <iostream>

MyClass::MyClass(int variable) : myVariable_(variable)
{
	std::cout << "MyClass constructor + variable: "<< variable << std::endl;
}

MyClass::~MyClass()
{
	std::cout << "MyClass destructor" << std::endl;
}