// SHA-256vk.h: declares the functions for computing SHA-256 hashes with Vulkan
//

#if !defined (__SHA_256_VK_H__)
#define __SHA_256_VK_H__

// Includes
//

// Local Project Headers
#include "../common/SHA-256defs.h"

// Functions
//

// Calculates (and prints out) the SHA-256 of the inputs using Vulkan
int vkSha256(int, const char* argv[]);

#endif // __SHA_256_VK_H__
