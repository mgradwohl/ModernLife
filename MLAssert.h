#pragma once

#define ML_DEBUG_BREAK __debugbreak()

#ifdef _DEBUG
#define ML_ENABLE_ASSERTS
#endif

#define ML_ENABLE_VERIFY

#ifdef ML_ENABLE_ASSERTS
#define ML_ASSERT_MESSAGE_INTERNAL(...)  ::Util::Log::PrintAssertMessage("Assertion Failed", __VA_ARGS__)

#define ML_ASSERT(condition, ...) { if(!(condition)) { ML_ASSERT_MESSAGE_INTERNAL(__VA_ARGS__); ML_DEBUG_BREAK; } }
#else
#define ML_ASSERT(condition, ...)
#endif

#ifdef ML_ENABLE_VERIFY
#define ML_VERIFY_MESSAGE_INTERNAL(...)  ::Util::Log::PrintAssertMessage(::Util::Log::Type::Client, "Verify Failed", __VA_ARGS__)

#define ML_VERIFY(condition, ...) { if(!(condition)) { ML_VERIFY_MESSAGE_INTERNAL(__VA_ARGS__); ML_DEBUG_BREAK; } }
#else
#define ML_VERIFY(condition, ...)
#endif