#include <Renderer.h>

int main(){
	Renderer renderer;

	try{
		renderer.run();
	}catch (const std::exception& e){
		std::cerr << e.what() << '\n';
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}