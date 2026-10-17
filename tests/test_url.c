// Host check for the address bar: gcc -Isource tests/test_url.c && ./a.out
#include <assert.h>
#include "url.h"

static const char *u(const char *in) { static char b[256]; to_url(in, b, sizeof b); return b; }

int main(void) {
    assert(!strcmp(u("https://x.com/a?b"), "https://x.com/a?b"));
    assert(!strcmp(u("youtube.com"), "https://youtube.com"));
    assert(!strcmp(u("  github.com/ad3333m"), "https://github.com/ad3333m"));
    assert(!strcmp(u("switch homebrew"), "https://duckduckgo.com/?q=switch+homebrew"));
    assert(!strcmp(u("c++ & 50%"), "https://duckduckgo.com/?q=c%2B%2B+%26+50%25"));
    puts("ok");
    return 0;
}
