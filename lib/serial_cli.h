/**
 * Simple command line parser
 *
 * MIT
 */
#ifndef SERIAL_CLI_H
#define SERIAL_CLI_H

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CMD_LEN
#define CMD_LEN 0x7F // big enough for our needs
#endif

#define CLI_EOK      0 // success
#define CLI_EARG    -1 // invalid argument
#define CLI_ENOTSUP -2 // command not supported
#define CLI_ENODEV  -3 // device communication error

// command line processing, returns CLI_E* above
typedef int8_t cli_processor(char *buf, void *ptr);
int8_t cli(char *buf, void *ptr);

void   cli_init(void);
int8_t cli_interact(cli_processor *process, void *ptr);

/* helper functions */
char *get_arg(char *str);
const char *is_on(uint8_t val);
int8_t str_is(const char *str, const char *cmd);

#ifdef __cplusplus
}
#endif
#endif
