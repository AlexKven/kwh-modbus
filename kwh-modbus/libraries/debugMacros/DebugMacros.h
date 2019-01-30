#pragma once

#define DEBUG_CATEGORY_NONE bool isDebugCategoryEnabled(char* category) { return false; }
#define DEBUG_CATEGORY_ALL bool isDebugCategoryEnabled(char* category) { return true; }
#define DEBUG_CATEGORY(CATEGORIES) bool isDebugCategoryEnabled(char* category) \
{ \
	char *debugCategories = #CATEGORIES;\
	int curCategorySearchIndex = 0; \
	int categorySearchMatchStart = 0; \
	if (category[0] == '|' && category[1] == '\0') return true; \
	while (debugCategories[curCategorySearchIndex] != '\0') \
	{ \
		if (debugCategories[curCategorySearchIndex] == '|') \
		{ \
			if (categorySearchMatchStart != -1) \
				return true; \
			else \
				categorySearchMatchStart = curCategorySearchIndex + 1; \
		} \
		else if (categorySearchMatchStart != -1) \
		{ \
			if (debugCategories[curCategorySearchIndex] != category[curCategorySearchIndex - categorySearchMatchStart]) \
			categorySearchMatchStart = -1; \
		} \
		curCategorySearchIndex++; \
	} \
	return (categorySearchMatchStart != -1); \
}

#ifndef PRINTLN(MSG)
#define PRINTLN(MSG)
#endif
#ifndef PRINT(MSG)
#define PRINT(MSG)
#endif
#ifndef P_TIME()
#define P_TIME()
#endif
#ifndef WRITE(CHR)
#define WRITE(CHR)
#endif

#ifndef CONSOLE_DEBUG
#define DEBUG(CATEGORY, ...)
#else
#define DEBUG(CATEGORY, ...) if (isDebugCategoryEnabled(#CATEGORY)) \
{ \
	__VA_ARGS__; \
}
#endif

#ifndef CONSOLE_INFO
#define INFO(CATEGORY, ...)
#else
#define INFO(CATEGORY, ...) if (isDebugCategoryEnabled(#CATEGORY)) \
{ \
	__VA_ARGS__; \
}
#endif

#ifndef CONSOLE_VERBOSE
#define VERBOSE(CATEGORY, ...)
#else
#define VERBOSE(CATEGORY, ...) if (isDebugCategoryEnabled(#CATEGORY)) \
{ \
	__VA_ARGS__; \
}
#endif

