#define SDL_MAIN_HANDLED

import razor;

int main(int argc, char *argv[])
{
	std::cout << "Unit-testing..." << std::endl;
	int result = razor::serializationUnitTest();
	std::cout << "Serialization: " << (result==0 ? "Passed" : std::string("Failed ").append(std::to_string(result))) << std::endl;

	result = razor::networkingUnitTest();
	std::cout << "Networking: " << (result==0 ? "Passed" : std::string("Failed ").append(std::to_string(result))) << std::endl;
	
	auto r = new razor::Razor();
	
	return 0;
}