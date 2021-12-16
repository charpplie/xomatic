#pragma once

#if defined(EDITOR_COMMON_EXPORTS)

#define EDITOR_COMMON_API __declspec(dllexport)

#elif defined(EDITOR_COMMON_IMPORTS)

#define EDITOR_COMMON_API __declspec(dllimport)

#else

#define EDITOR_COMMON_API 

#endif
