// Time-scaling core: hooks the time sources games use to measure delta time.
// Virtual time advances at (real delta * scale); switching scale never causes a jump.
#include "SpeedCore.h"
#include "Rebind.h"
#include <dlfcn.h>
#include <mach/mach_time.h>
#include <os/lock.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>

static uint64_t (*o_mat)(void);
static uint64_t (*o_mct)(void);
static int      (*o_cgt)(clockid_t, struct timespec *);
static uint64_t (*o_cgtn)(clockid_t);
static int      (*o_gtod)(struct timeval *, void *);

static mach_timebase_info_data_t g_tb = {1, 1};
static os_unfair_lock g_lock = OS_UNFAIR_LOCK_INIT;
static double   g_scale = 1.0;
static bool     g_enabled = false;
static bool     g_wall = false;
static uint64_t g_lastReal, g_fake, g_start;

static inline uint64_t t2ns(uint64_t t) {
    return (uint64_t)(((unsigned __int128)t * g_tb.numer) / g_tb.denom);
}

static void advance(uint64_t *real, uint64_t *fake)
{
    os_unfair_lock_lock(&g_lock);
    uint64_t r = o_mat();
    uint64_t d = r > g_lastReal ? r - g_lastReal : 0;
    double s = g_enabled ? g_scale : 1.0;
    g_fake += (uint64_t)((double)d * s);
    g_lastReal = r;
    *real = r;
    *fake = g_fake;
    os_unfair_lock_unlock(&g_lock);
}

static int64_t offset_ns(void)
{
    uint64_t r, f;
    advance(&r, &f);
    return (int64_t)t2ns(f) - (int64_t)t2ns(r);
}

static bool applies(clockid_t id) { return id != CLOCK_REALTIME || g_wall; }

static uint64_t h_mat(void) { uint64_t r, f; advance(&r, &f); return f; }

static uint64_t h_mct(void) { uint64_t r, f; advance(&r, &f); return o_mct() + (f - r); }

static int h_cgt(clockid_t id, struct timespec *ts)
{
    int rc = o_cgt(id, ts);
    if (rc == 0 && ts && applies(id)) {
        int64_t off = offset_ns();
        int64_t total = (int64_t)ts->tv_sec * 1000000000LL + ts->tv_nsec + off;
        ts->tv_sec = total / 1000000000LL;
        ts->tv_nsec = total % 1000000000LL;
    }
    return rc;
}

static uint64_t h_cgtn(clockid_t id)
{
    uint64_t v = o_cgtn(id);
    if (v && applies(id)) v = (uint64_t)((int64_t)v + offset_ns());
    return v;
}

static int h_gtod(struct timeval *tv, void *tz)
{
    int rc = o_gtod(tv, tz);
    if (rc == 0 && tv && g_wall) {
        int64_t total = (int64_t)tv->tv_sec * 1000000LL + tv->tv_usec + offset_ns() / 1000;
        tv->tv_sec = total / 1000000LL;
        tv->tv_usec = (int)(total % 1000000LL);
    }
    return rc;
}

void MBSpeedInstall(void)
{
    static bool done = false;
    if (done) return;
    done = true;

    mach_timebase_info(&g_tb);
    // The adapter prefers ElleKit/Substrate or libhooker when loaded by a jailbreak.
    // Otherwise it registers imports for the project's limited fallback rebinder.
    ap_hook_symbol("mach_absolute_time", (void *)h_mat, (void **)&o_mat, 0);
    if (!o_mat) return;
    ap_hook_symbol("mach_continuous_time", (void *)h_mct, (void **)&o_mct, 0);
    ap_hook_symbol("clock_gettime", (void *)h_cgt, (void **)&o_cgt, 0);
    ap_hook_symbol("clock_gettime_nsec_np", (void *)h_cgtn, (void **)&o_cgtn, 0);
    ap_hook_symbol("gettimeofday", (void *)h_gtod, (void **)&o_gtod, 0);

    g_lastReal = g_fake = g_start = o_mat();
    // ap_rebind_start() is called once by ap_install_hooks() after all hooks register.
}

void   MBSpeedSetScale(double s) { if (s < 0.1) s = 0.1; if (s > 10) s = 10; uint64_t r, f; if (o_mat) advance(&r, &f); g_scale = s; }
double MBSpeedGetScale(void)     { return g_scale; }
void   MBSpeedSetEnabled(bool on){ uint64_t r, f; if (o_mat) advance(&r, &f); g_enabled = on; }
bool   MBSpeedIsEnabled(void)    { return g_enabled; }
void   MBSpeedSetWallClock(bool on){ g_wall = on; }
bool   MBSpeedGetWallClock(void) { return g_wall; }

void MBSpeedStats(double *game, double *real)
{
    if (!o_mat) { *game = *real = 0; return; }
    uint64_t r, f;
    advance(&r, &f);
    *game = (double)t2ns(f - g_start) / 1e9;
    *real = (double)t2ns(r - g_start) / 1e9;
}
