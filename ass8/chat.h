#include <stdint.h>
#include <inttypes.h>

#define MAX_PEERS 5
#define MAX_NAME 256

struct user_entry {
    char username[MAX_NAME];
    char ip[16];
    uint16_t port;
};

struct user_info {
    struct user_entry table[MAX_PEERS];
    int number_peers;
};