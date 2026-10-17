#pragma once
#include <ctype.h>
#include <stdio.h>
#include <string.h>

// Address-bar input -> URL: keeps full URLs, adds https:// to bare domains,
// anything else becomes a DuckDuckGo search.
static void to_url(const char *in, char *out, size_t n) {
    while (*in == ' ') in++;
    if (strstr(in, "://")) { snprintf(out, n, "%s", in); return; }
    if (!strchr(in, ' ') && strchr(in, '.')) { snprintf(out, n, "https://%s", in); return; }
    size_t o = snprintf(out, n, "https://duckduckgo.com/?q=");
    for (const unsigned char *p = (const unsigned char *)in; *p && o + 4 < n; p++) {
        if (isalnum(*p) || strchr("-_.~", *p)) out[o++] = *p;
        else if (*p == ' ') out[o++] = '+';
        else o += sprintf(out + o, "%%%02X", *p);
    }
    out[o] = 0;
}
