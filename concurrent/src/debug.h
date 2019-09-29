#pragma once

#define printf_error(format,...) \
	printf("\033[;31mFile: " __FILE__", Line: %d: \033[0m" format "\n", __LINE__, ##__VA_ARGS__)

#define printf_info(format,...) \
	printf("\033[;32mFile: " __FILE__", Line: %d: \033[0m" format "\n", __LINE__, ##__VA_ARGS__)

#define printf_warn(format,...) \
	printf("\033[;33mFile: " __FILE__", Line: %d: \033[0m" format "\n", __LINE__, ##__VA_ARGS__)
