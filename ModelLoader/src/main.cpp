#include <iostream>
#include "ModelLoader/ModelLoader.h"


int main()
{
	ModelLoader* app = new ModelLoader();
	try{
		app->run();
		}catch(const std::exception& e){
			std::cerr << e.what() << std::endl;
			return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
	

	return 0;
}