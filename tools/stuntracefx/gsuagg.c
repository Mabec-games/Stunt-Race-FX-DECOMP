/*
 * gsuagg.c: the Super FX's code log (sfxbox SFXBOX_GSUCYCLOG with
 * SFXBOX_GSUCYCLOG_OPS) as its distinct intervals, for gsutime.py.
 *
 *     gsuagg LOG FNMAP.bin CTX.txt > AGG
 *
 * Each line: count, the routine called, the record's type, the probes the
 * C passed (in order), the original's code fetches the C did not state
 * (runs of addresses), and the C's stated ones the original did not fetch.
 * The probes of functions called from more than one place are qualified by
 * where they were called from: such a probe passed is written id + 131072
 * * ctx, ctx (from 1) a line of CTX.txt, the call-site probes of the
 * shared functions on the stack, the outermost first. FNMAP.bin: one int32
 * per probe id, its function's number, -1 for a header's.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#define TB (1u << 23)
static uint64_t th[TB]; static uint32_t tc[TB]; static char *ts[TB];
static uint64_t fnv(const void *p, size_t n) { const uint8_t *s = p; uint64_t h = 1469598103934665603ull; for (size_t i = 0; i < n; i++) { h ^= s[i]; h *= 1099511628211ull; } return h | 1; }
static int cmpu(const void *a, const void *b) { uint32_t x = *(const uint32_t *)a, y = *(const uint32_t *)b; return x < y ? -1 : x > y; }
static int runs(char *b, int k, int cap, uint32_t *v, int n)
{
    int first = 1;
    for (int i = 0; i < n && k < cap - 32;) {
        int j = i; while (j + 1 < n && v[j + 1] == v[j] + 1) j++;
        k += snprintf(b + k, cap - k, first ? "%06X:%d" : ",%06X:%d", v[i], j - i + 1); first = 0; i = j + 1;
    }
    return k;
}

static int32_t *fnof; static uint32_t nprobe;
/* call sites seen per function: a small set each */
#define MAXF 8192
static uint32_t sites[MAXF][64]; static int nsites[MAXF];
static void site_add(int f, uint32_t s) { for (int i = 0; i < nsites[f]; i++) if (sites[f][i] == s) return; if (nsites[f] < 64) sites[f][nsites[f]++] = s; }

/* the stack of functions entered: (function, the probe before its first) */
static int sf[256]; static uint32_t ss[256]; static int sn;
static uint32_t last;

static void enter(uint32_t q)
{
    int f = q < nprobe ? fnof[q] : -1;
    if (f < 0) return;                       /* a header's: no frame */
    if (sn && sf[sn - 1] == f) { last = q; return; }
    for (int i = sn - 1; i >= 0; i--) if (sf[i] == f) { sn = i + 1; last = q; return; }   /* back in a caller */
    if (sn < 256) { sf[sn] = f; ss[sn] = last; sn++; }
    last = q;
}

/* contexts */
#define CT (1u << 20)
static uint64_t ch[CT]; static uint32_t cid[CT]; static uint32_t nctx;
static FILE *ctxf;
static uint32_t ctx_of(void)
{
    uint32_t key[256]; int k = 0;
    for (int i = 0; i < sn; i++) if (nsites[sf[i]] > 1) { key[k++] = (uint32_t)sf[i]; key[k++] = ss[i]; }
    if (!k) return 0;
    uint64_t h = fnv(key, 4 * (size_t)k); uint32_t s = (uint32_t)h & (CT - 1);
    while (ch[s] && ch[s] != h) s = (s + 1) & (CT - 1);
    if (!ch[s]) {
        ch[s] = h; cid[s] = ++nctx;
        for (int i = 0; i < k; i += 2) fprintf(ctxf, i ? " %u:%u" : "%u:%u", key[i], key[i + 1]);
        fputc('\n', ctxf);
    }
    return cid[s];
}

int main(int argc, char **argv)
{
    int fd = open(argv[1], O_RDONLY); struct stat st; fstat(fd, &st);
    const uint8_t *d = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    FILE *fm = fopen(argv[2], "rb"); fseek(fm, 0, SEEK_END); nprobe = (uint32_t)(ftell(fm) / 4); rewind(fm);
    fnof = malloc(4 * (size_t)nprobe); if (fread(fnof, 4, nprobe, fm) != nprobe) return 1; fclose(fm);
    ctxf = fopen(argv[3], "w");
    size_t n = st.st_size;
    static char buf[1 << 22]; static uint32_t of[1 << 20], mf[1 << 20], pl[1 << 20], mi[1 << 20];
    for (int pass = 0; pass < 2; pass++) {
        size_t i = 0; uint32_t pc = 0; sn = 0; last = 0;
        while (i < n) {
            uint8_t t = d[i++];
            if (t == 'O') continue;
            if (t == 'S') { memcpy(&pc, d + i, 4); i += 4; sn = 0; last = 0; continue; }
            uint32_t h[4]; memcpy(h, d + i, 16); i += 16;
            const uint8_t *ids = d + i; i += 4 * (size_t)h[3];
            uint32_t m; memcpy(&m, d + i, 4); i += 4;
            const uint8_t *ops = d + i; i += 5 * (size_t)m;
            int k = 0;
            if (pass == 1) k = snprintf(buf, sizeof buf, "%06X\t%c\t", pc, t);
            for (uint32_t q = 0; q < h[3]; q++) {
                uint32_t v; memcpy(&v, ids + 4 * q, 4);
                int depth = sn;
                enter(v);
                if (pass == 0) { if (sn > depth && sn > 0) site_add(sf[sn - 1], ss[sn - 1]); continue; }
                uint32_t c = ctx_of();
                if (k < (int)sizeof buf - 64) k += snprintf(buf + k, sizeof buf - k, q ? ",%u" : "%u", v + 131072u * c);
            }
            if (pass == 0 || h[0] == 0xFFFFFFFFu) continue;
            int no = 0, nm = 0;
            for (uint32_t q = 0; q < m; q++) {
                uint8_t o = ops[5 * q]; uint32_t a; memcpy(&a, ops + 5 * q + 1, 4);
                if (o == 8 && no < (1 << 20)) of[no++] = a;
                if (o == 110 && nm < (1 << 20)) mf[nm++] = a;
                /* a CACHE run: the original's (its address), the C's (R15, one past it) */
                if (o == 11 && no < (1 << 20)) of[no++] = 0xE00000u | (a & 0xFFFF);
                if (o == 111 && nm < (1 << 20)) mf[nm++] = 0xE00000u | ((a - 1) & 0xFFFF);
            }
            qsort(of, no, 4, cmpu); qsort(mf, nm, 4, cmpu);
            int a = 0, b = 0, np = 0, nn = 0;
            while (a < no || b < nm) {
                if (b >= nm || (a < no && of[a] < mf[b])) pl[np++] = of[a++];
                else if (a >= no || mf[b] < of[a]) mi[nn++] = mf[b++];
                else { a++; b++; }
            }
            buf[k++] = '\t'; k = runs(buf, k, sizeof buf, pl, np);
            buf[k++] = '\t'; k = runs(buf, k, sizeof buf, mi, nn);
            uint64_t hh = fnv(buf, k); uint32_t s = (uint32_t)hh & (TB - 1);
            while (th[s] && th[s] != hh) s = (s + 1) & (TB - 1);
            if (!th[s]) { th[s] = hh; buf[k] = 0; ts[s] = strdup(buf); }
            tc[s]++;
        }
        if (pass == 0) {
            int shared = 0; for (int f = 0; f < MAXF; f++) if (nsites[f] > 1) shared++;
            fprintf(stderr, "functions called from more than one place: %d\n", shared);
        }
    }
    for (uint32_t s = 0; s < TB; s++) if (th[s]) printf("%u\t%s\n", tc[s], ts[s]);
    fprintf(stderr, "contexts %u\n", nctx);
    fclose(ctxf);
    return 0;
}
