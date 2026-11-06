// Rebind.c - minimal symbol-pointer rebinding for arm64 iOS. One registry for the whole dylib.
#include "Rebind.h"
#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <mach/mach.h>
#include <string.h>
#include <stdint.h>

#ifndef AP_SAFE_EMBEDDED
#define AP_SAFE_EMBEDDED 0
#endif

#define AP_MAX_REBINDS 24

typedef struct { const char *name; void *rep; int system; } Entry;
static Entry  g_rb[AP_MAX_REBINDS];
static size_t g_n = 0;
static const void *g_self = NULL;

typedef void (*APHookFunction)(void *symbol, void *replacement, void **original);
static APHookFunction g_runtimeHook = NULL;
static const char *g_backendName = "fishhook fallback";
static int g_backendChecked = 0;

static void ap_detect_hook_backend(void)
{
    if (g_backendChecked) return;
    g_backendChecked = 1;
    // ElleKit exports the Substrate-compatible API. Prefer it when present.
    g_runtimeHook = (APHookFunction)dlsym(RTLD_DEFAULT, "MSHookFunction");
    if (g_runtimeHook) { g_backendName = "ElleKit/Substrate API"; return; }
    // libhooker exposes a function-hook API with the same three-argument ABI.
    g_runtimeHook = (APHookFunction)dlsym(RTLD_DEFAULT, "LHHookFunction");
    if (g_runtimeHook) { g_backendName = "libhooker"; return; }
}

const char *ap_hook_backend_name(void)
{
    ap_detect_hook_backend();
    return g_backendName;
}

int ap_hook_symbol(const char *name, void *replacement, void **originalStorage, int system)
{
    if (!name || !replacement || !originalStorage) return 0;
    ap_detect_hook_backend();
    void *target = dlsym(RTLD_DEFAULT, name);
    if (!target) return 0;
    if (g_runtimeHook) {
        g_runtimeHook(target, replacement, originalStorage);
        return *originalStorage != NULL;
    }
    *originalStorage = target;
    ap_rebind_add(name, replacement, system);
    return 1;
}

void ap_rebind_add(const char *name, void *rep, int system)
{
    if (g_n < AP_MAX_REBINDS && name && rep) {
        g_rb[g_n].name = name; g_rb[g_n].rep = rep; g_rb[g_n].system = system;
        g_n++;
    }
}

static int is_system_path(const char *p)
{
    if (!p) return 0;
    return strncmp(p, "/System/", 8) == 0 || strncmp(p, "/usr/lib/", 9) == 0 ||
           strncmp(p, "/Developer/", 11) == 0 || strncmp(p, "/usr/libexec/", 13) == 0;
}

static void patch_section(const struct section_64 *sect, intptr_t slide,
                          const struct nlist_64 *symtab, uint32_t nsyms,
                          const char *strtab, uint32_t strsize,
                          const uint32_t *indirect, int isConst, int sysImage)
{
    if (!sect || !symtab || !strtab || !indirect) return;
#if AP_SAFE_EMBEDDED
    // Stock iOS: do not try to make __DATA_CONST writable. This avoids a
    // protected-page write path on signed, non-jailbroken devices.
    if (isConst) return;
#endif
    const uint32_t *isyms = indirect + sect->reserved1;
    void **slots = (void **)(uintptr_t)(sect->addr + slide);
    size_t count = sect->size / sizeof(void *);
    int unprotected = 0;

    for (size_t i = 0; i < count; i++) {
        uint32_t idx = isyms[i];
        if (idx >= nsyms) continue;
        if (idx == INDIRECT_SYMBOL_ABS || idx == INDIRECT_SYMBOL_LOCAL ||
            idx == (INDIRECT_SYMBOL_LOCAL | INDIRECT_SYMBOL_ABS)) continue;
        uint32_t strx = symtab[idx].n_un.n_strx;
        if (strx >= strsize) continue;
        const char *name = strtab + strx;
        if (!memchr(name, '\0', strsize - strx) || name[0] != '_') continue;
        name++;
        for (size_t k = 0; k < g_n; k++) {
            if (sysImage && !g_rb[k].system) continue;
            if (strcmp(name, g_rb[k].name) != 0 || slots[i] == g_rb[k].rep) continue;
            if (isConst && !unprotected) {
                uintptr_t pg = (uintptr_t)vm_page_size;
                uintptr_t s = (uintptr_t)slots & ~(pg - 1);
                uintptr_t e = ((uintptr_t)slots + sect->size + pg - 1) & ~(pg - 1);
                kern_return_t kr = vm_protect(mach_task_self(), (vm_address_t)s, (vm_size_t)(e - s), 0,
                                              VM_PROT_READ | VM_PROT_WRITE | VM_PROT_COPY);
                if (kr != KERN_SUCCESS) return;   // cannot make it writable: skip instead of crashing
                unprotected = 1;
            }
            slots[i] = g_rb[k].rep;
        }
    }
}

static void on_image(const struct mach_header *mh, intptr_t slide)
{
    if (!mh || mh->magic != MH_MAGIC_64 || g_n == 0) return;
    if ((const void *)mh == g_self) return;                       // never patch ourselves
    Dl_info info;
    int sysImage = (dladdr(mh, &info) && is_system_path(info.dli_fname));

    const struct mach_header_64 *h = (const struct mach_header_64 *)mh;
    const struct symtab_command *st = NULL;
    const struct dysymtab_command *dst = NULL;
    const struct segment_command_64 *le = NULL;

    const uint8_t *cmdStart = (const uint8_t *)(h + 1);
    const uint8_t *cmdEnd = cmdStart + h->sizeofcmds;
    const uint8_t *p = cmdStart;
    for (uint32_t i = 0; i < h->ncmds && p + sizeof(struct load_command) <= cmdEnd; i++) {
        const struct load_command *lc = (const struct load_command *)p;
        if (lc->cmdsize < sizeof(struct load_command) || p + lc->cmdsize > cmdEnd) return;
        if (lc->cmd == LC_SEGMENT_64 && lc->cmdsize >= sizeof(struct segment_command_64)) {
            const struct segment_command_64 *seg = (const struct segment_command_64 *)p;
            if (strcmp(seg->segname, SEG_LINKEDIT) == 0) le = seg;
        } else if (lc->cmd == LC_SYMTAB && lc->cmdsize >= sizeof(struct symtab_command)) {
            st = (const struct symtab_command *)p;
        } else if (lc->cmd == LC_DYSYMTAB && lc->cmdsize >= sizeof(struct dysymtab_command)) {
            dst = (const struct dysymtab_command *)p;
        }
        p += lc->cmdsize;
    }
    if (!st || !dst || !le || dst->nindirectsyms == 0) return;

    uintptr_t base = (uintptr_t)slide + le->vmaddr - le->fileoff;
    const struct nlist_64 *symtab = (const struct nlist_64 *)(base + st->symoff);
    const char *strtab = (const char *)(base + st->stroff);
    const uint32_t *indirect = (const uint32_t *)(base + dst->indirectsymoff);

    p = cmdStart;
    for (uint32_t i = 0; i < h->ncmds && p + sizeof(struct load_command) <= cmdEnd; i++) {
        const struct load_command *lc = (const struct load_command *)p;
        if (lc->cmdsize < sizeof(struct load_command) || p + lc->cmdsize > cmdEnd) return;
        if (lc->cmd == LC_SEGMENT_64 && lc->cmdsize >= sizeof(struct segment_command_64)) {
            const struct segment_command_64 *seg = (const struct segment_command_64 *)p;
            int isData = strcmp(seg->segname, "__DATA") == 0;
            int isConst = strcmp(seg->segname, "__DATA_CONST") == 0;
            if (isData || isConst) {
                if (seg->nsects > (lc->cmdsize - sizeof(struct segment_command_64)) / sizeof(struct section_64)) return;
                const struct section_64 *secs = (const struct section_64 *)(seg + 1);
                for (uint32_t j = 0; j < seg->nsects; j++) {
                    uint32_t type = secs[j].flags & SECTION_TYPE;
                    if ((type == S_LAZY_SYMBOL_POINTERS || type == S_NON_LAZY_SYMBOL_POINTERS) &&
                        secs[j].reserved1 <= dst->nindirectsyms &&
                        secs[j].size / sizeof(void *) <= dst->nindirectsyms - secs[j].reserved1)
                        patch_section(&secs[j], slide, symtab, st->nsyms, strtab, st->strsize,
                                      indirect, isConst, sysImage);
                }
            }
        }
        p += lc->cmdsize;
    }
}

void ap_rebind_start(void)
{
    Dl_info me;
    if (dladdr((const void *)&ap_rebind_start, &me)) g_self = me.dli_fbase;
    _dyld_register_func_for_add_image(on_image);   // also fires for already-loaded images
}
