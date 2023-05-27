// SHA-256vk.h: declares the functions for computing SHA-256 hashes with Vulkan
//

#if !defined (__SHA_256_VK_H__)
#define __SHA_256_VK_H__

// Includes
//

// Local Project Headers
#include "SHA-256defs.h"

// Types
//

typedef struct _VkSha256Result {
    uint32_t data[SHA256_WC];
} VkSha256Result;

// Functions
//

// Calculates (and prints out) the SHA-256 of the input using Vulkan
int vkSha256(const char*, size_t);

#endif // __SHA_256_VK_H__
