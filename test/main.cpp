#define SDL_MAIN_HANDLED

#include "razor.h"

int main(int argc, char *argv[])
{
	std::cout << "==Razor Unit Tests==" << std::endl;
	int result = razor::serializationUnitTest();
	std::cout << "Serialization: " << (result==0 ? "Passed" : std::string("Failed ").append(std::to_string(result))) << std::endl;

	result = razor::networkingUnitTest();
	std::cout << "Networking: " << (result==0 ? "Passed" : std::string("Failed ").append(std::to_string(result))) << std::endl;
	
	std::cout << "==Razor Client/Server Test==" << std::endl;
	std::cout << "Creating server..." << std::endl;
	auto s = new razor::Razor();
	s->setDaemon();
	s->setPort(12320);
	
	std::cout << "Creating client..." << std::endl;
	auto c = new razor::Razor();
	c->setPort(12321);
	
	delete s;
	delete c;
	return 0;
}