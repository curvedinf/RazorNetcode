#define SDL_MAIN_HANDLED

#include "razor.h"

int main(int argc, char *argv[])
{
	std::cout << "==Razor Unit Tests==" << std::endl;
	int result = razor::serializationUnitTest();
	std::cout << "Serialization: " << (
			result==0 ? 
			"Passed" : 
			std::string("Failed ").append(std::to_string(result))
		) << std::endl;

	result = razor::networkingUnitTest();
	std::cout << "Networking: " << (
			result==0 ? 
			"Passed" : 
			std::string("Failed ").append(std::to_string(result))
		) << std::endl;
	
	result = razor::razorUnitTest();
	std::cout << "Razor: " << (
			result==0 ? 
			"Passed" : 
			std::string("Failed ").append(std::to_string(result))
		) << std::endl;
	
	std::cout << "Hello?" << std::endl;
	
	return 0;
}