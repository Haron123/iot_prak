#include "logging.h"

void logg(const char* prefix, const char* file, const char* function, uint32_t line, const char* format, ...)
{
	(void)file;
	//printf("[%s] %s:%ld in %s | ", prefix, file, line, function);
	printf("[%s] In %s:%ld | ", prefix, function, line);

	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);

	printf("\n");
}