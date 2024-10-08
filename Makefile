CFLAGS = -std=c++17 -O2
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

VulkanTest: main.cpp
	g++ $(CFLAGS) -o VulkanStudy main.cpp -I. -I./3rdparty $(LDFLAGS)
	
.PHONY: test clean

test: VulkanStudy
	./VulkanStudy
	
clean:
	rm -f VulkanStudy
