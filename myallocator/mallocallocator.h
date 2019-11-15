#ifndef MY_MALLOC_ALLOCATOR
#define MY_MALLOC_ALLOCATOR

#include <cstdlib>
#include <limits>

namespace MyAllocator{

	struct MallocAllocator{
		static void *allocate(std::size_t const size) noexcept{
			return malloc(size);
		}

		static void deallocate(void *p) noexcept{
			return free(p);
		}

		constexpr static bool need_deallocate() noexcept{
			return true;
		}

		constexpr static bool reset() noexcept{
			return false;
		}

		constexpr static std::size_t getFreeMemory() noexcept{
			return std::numeric_limits<std::size_t>::max();
		}

		constexpr static std::size_t getUsedMemory() noexcept{
			return std::numeric_limits<std::size_t>::max();
		}
	};

} // namespace MyAllocator

#endif

