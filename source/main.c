// Light Browser: a tiny launcher around the Switch's own web applet (the
// system WebKit), so pages render natively and the .nro stays a few hundred KB.
#include <switch.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "url.h"

#define DIR       "sdmc:/switch/LightBrowser"
#define BOOKMARKS DIR "/bookmarks.txt"
#define STATE     DIR "/state.txt"
#define URLMAX    0xC00 // the applet's own URL limit
#define MAXBM     64
#define ROWS      34    // bookmark rows that fit on the 80x45 console

static char bm[MAXBM][URLMAX];
static int nbm;
static char last[URLMAX];
static bool fit = true; // "Fit to screen": mobile layouts + auto-hiding footer
static char msg[160];

static void strip(char *s) { s[strcspn(s, "\r\n")] = 0; }

static void load(void) {
    FILE *f = fopen(BOOKMARKS, "r");
    if (f) {
        while (nbm < MAXBM && fgets(bm[nbm], URLMAX, f)) { strip(bm[nbm]); if (bm[nbm][0]) nbm++; }
        fclose(f);
    } else {
        const char *def[] = {"https://duckduckgo.com", "https://www.youtube.com",
                             "https://en.m.wikipedia.org", "https://github.com"};
        for (; nbm < 4; nbm++) strcpy(bm[nbm], def[nbm]);
    }
    if ((f = fopen(STATE, "r"))) {
        char line[8] = "1";
        fgets(line, sizeof line, f);
        fit = line[0] != '0';
        if (fgets(last, URLMAX, f)) strip(last);
        fclose(f);
    }
}

static void save(void) {
    mkdir(DIR, 0777);
    FILE *f = fopen(BOOKMARKS, "w");
    if (f) { for (int i = 0; i < nbm; i++) fprintf(f, "%s\n", bm[i]); fclose(f); }
    if ((f = fopen(STATE, "w"))) { fprintf(f, "%d\n%s\n", fit, last); fclose(f); }
}

static void go(const char *url) {
    WebCommonConfig cfg;
    WebCommonReply reply;
    Result rc = webPageCreate(&cfg, url);
    if (R_SUCCEEDED(rc)) {
        webConfigSetWhitelist(&cfg, "^http*");
        webConfigSetTouchEnabledOnContents(&cfg, true);
        webConfigSetPageCache(&cfg, true);
        webConfigSetWebAudio(&cfg, true);
        if (fit) {
            // ponytail: the applet only lets us append to its UA, not replace it;
            // "Mobile" is enough for most responsive sites to serve a 1280x720-friendly layout.
            webConfigSetUserAgentAdditionalString(&cfg, "Mobile Safari/605.1.15");
            webConfigSetFooterFixedKind(&cfg, WebFooterFixedKind_Hidden);
        }
        rc = webConfigShow(&cfg, &reply);
    }
    if (R_FAILED(rc)) {
        snprintf(msg, sizeof msg, "Couldn't open the browser (0x%x). Launch via a game (hold R) instead of the Album.", rc);
        return;
    }
    char page[URLMAX];
    size_t n;
    if (R_FAILED(webReplyGetLastUrl(&reply, page, sizeof page, &n)) || !page[0]) snprintf(page, sizeof page, "%s", url);
    strcpy(last, page); // url may be `last` itself
    msg[0] = 0;
    save();
}

static void ask_and_go(void) {
    SwkbdConfig k;
    char in[URLMAX] = "", url[URLMAX];
    if (R_FAILED(swkbdCreate(&k, 0))) return;
    swkbdConfigMakePresetDefault(&k);
    swkbdConfigSetGuideText(&k, "Address or search");
    swkbdConfigSetOkButtonText(&k, "Go");
    swkbdConfigSetStringLenMax(&k, 1000);
    Result rc = swkbdShow(&k, in, sizeof in);
    swkbdClose(&k);
    if (R_SUCCEEDED(rc) && in[0]) { to_url(in, url, sizeof url); go(url); }
}

static void row(bool on, const char *label, const char *text) {
    printf("%s %c %s%.70s\x1b[0m\n", on ? "\x1b[36;1m" : "", on ? '>' : ' ', label, text);
}

static void draw(int sel, int top) {
    consoleClear();
    printf("\x1b[1m Light Browser\x1b[0m%*s\x1b[%sm[Fit to screen: %s]\x1b[0m\n",
           42, "", fit ? "32" : "33", fit ? "ON " : "OFF");
    printf(" ------------------------------------------------------------------------------\n\n");
    row(sel == 0, "Go to address / search", "");
    row(sel == 1, "Resume: ", last[0] ? last : "(nothing yet)");
    printf("\n Bookmarks\n");
    for (int i = top; i < nbm && i < top + ROWS; i++) row(sel == i + 2, "", bm[i]);
    if (!nbm) printf("   (none - press Y on Resume to add the last page)\n");
    printf("\x1b[42;1H\x1b[31m %.78s\x1b[0m", msg);
    printf("\x1b[44;1H A open   X fit to screen   Y bookmark last page   - remove   + exit");
    consoleUpdate(NULL);
}

int main(void) {
    consoleInit(NULL);
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);
    load();

    AppletType at = appletGetAppletType();
    if (at != AppletType_Application && at != AppletType_SystemApplication)
        snprintf(msg, sizeof msg, "Album mode: pages may not open. Hold R while launching a game for full mode.");

    int sel = 0, top = 0;
    bool dirty = true;
    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 k = padGetButtonsDown(&pad);
        int n = 2 + nbm;
        if (k & HidNpadButton_Plus) break;
        if (k & HidNpadButton_AnyDown) sel = (sel + 1) % n;
        if (k & HidNpadButton_AnyUp) sel = (sel + n - 1) % n;
        if (k & HidNpadButton_A) {
            if (sel == 0) ask_and_go();
            else if (sel == 1) { if (last[0]) go(last); }
            else go(bm[sel - 2]);
        }
        if (k & HidNpadButton_X) { fit = !fit; save(); }
        if ((k & HidNpadButton_Y) && last[0] && nbm < MAXBM) {
            bool dup = false;
            for (int i = 0; i < nbm; i++) dup |= !strcmp(bm[i], last);
            if (!dup) { strcpy(bm[nbm++], last); save(); }
        }
        if ((k & HidNpadButton_Minus) && sel >= 2) {
            memmove(bm[sel - 2], bm[sel - 1], (size_t)(nbm - sel + 1) * URLMAX);
            nbm--;
            if (sel >= 2 + nbm) sel--;
            save();
        }
        if (sel >= 2) {
            if (sel - 2 < top) top = sel - 2;
            if (sel - 2 >= top + ROWS) top = sel - 2 - ROWS + 1;
        }
        if (k) dirty = true;
        if (dirty) { draw(sel, top); dirty = false; }
    }
    consoleExit(NULL);
    return 0;
}
