/*
 * entropy.c — Shannon Entropy, Rényi entropy, mutual information,
 *             Jensen-Shannon divergence, typical sets, block entropy,
 *             entropy rate estimation.
 *
 * L3: Probability distributions and information measures.
 * L5: Entropy estimation algorithms for empirical data.
 *
 * Reference: Cover & Thomas "Elements of Information Theory" §2, §4, §11
 * Courses: MIT 6.441, Stanford EE376A, Berkeley EE229A, CMU 15-859
 */

#include "../include/kolmogorov.h"
#include "../include/entropy.h"
#include "../include/compression.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

/* ══════════════════════════════════════════════════════════════
   Frequency Distribution
   ══════════════════════════════════════════════════════════════ */

FrequencyDist* freqdist_create(int n_symbols) {
    FrequencyDist* fd = (FrequencyDist*)malloc(sizeof(FrequencyDist));
    assert(fd != NULL);
    fd->n_symbols = n_symbols;
    fd->probs = (double*)calloc((size_t)n_symbols, sizeof(double));
    assert(fd->probs != NULL);
    fd->entropy = 0.0;
    return fd;
}

void freqdist_compute_from_string(FrequencyDist* fd, const KString* s) {
    if (!fd || !s || s->len == 0) return;
    int* counts = (int*)calloc((size_t)fd->n_symbols, sizeof(int));
    assert(counts != NULL);

    for (size_t i = 0; i < s->len; i++) {
        int idx = (int)(s->data[i]);
        if (idx < fd->n_symbols)
            counts[idx]++;
    }

    for (int i = 0; i < fd->n_symbols; i++) {
        fd->probs[i] = (double)counts[i] / (double)s->len;
        if (fd->probs[i] > 0.0)
            fd->entropy -= fd->probs[i] * log2(fd->probs[i]);
    }
    free(counts);
}

void freqdist_compute_block(FrequencyDist* fd, const KString* s, int block_size) {
    if (!fd || !s || block_size < 1 || s->len == 0) return;
    /* Treat each block of 'block_size' bytes as a symbol */
    int symbol_space = 1;
    for (int i = 0; i < block_size && i < 4; i++) symbol_space *= 256;

    /* For blocks > 2 bytes, use a simplified hash-based approach */
    #define BLOCK_HASH_SIZE 65536
    int* counts = (int*)calloc(BLOCK_HASH_SIZE, sizeof(int));
    assert(counts != NULL);
    size_t n_blocks = s->len / (size_t)block_size;
    int used_slots = 0;
    int* slots = (int*)malloc(BLOCK_HASH_SIZE * sizeof(int));
    assert(slots != NULL);

    for (size_t i = 0; i < n_blocks; i++) {
        unsigned int hash = 0;
        for (int j = 0; j < block_size; j++) {
            hash = hash * 31 + s->data[i * (size_t)block_size + (size_t)j];
        }
        hash %= BLOCK_HASH_SIZE;
        if (counts[hash] == 0) slots[used_slots++] = (int)hash;
        counts[hash]++;
    }

    int symbols_to_use = fd->n_symbols < used_slots ? fd->n_symbols : used_slots;
    for (int i = 0; i < symbols_to_use; i++) {
        int h = slots[i];
        fd->probs[i] = (double)counts[h] / (double)n_blocks;
        if (fd->probs[i] > 0.0)
            fd->entropy -= fd->probs[i] * log2(fd->probs[i]);
    }
    free(counts);
    free(slots);
}

void freqdist_free(FrequencyDist* fd) {
    if (fd) { free(fd->probs); free(fd); }
}

/* ══════════════════════════════════════════════════════════════
   Shannon Entropy
   ══════════════════════════════════════════════════════════════ */

double shannon_entropy_bits(const KString* s) {
    if (!s || s->len == 0) return 0.0;
    int freq[256] = {0};
    for (size_t i = 0; i < s->len; i++) freq[s->data[i]]++;
    double H = 0.0;
    double n = (double)s->len;
    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) {
            double p = (double)freq[i] / n;
            H -= p * log2(p);
        }
    }
    return H;
}

double shannon_entropy_nats(const KString* s) {
    if (!s || s->len == 0) return 0.0;
    int freq[256] = {0};
    for (size_t i = 0; i < s->len; i++) freq[s->data[i]]++;
    double H = 0.0;
    double n = (double)s->len;
    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) {
            double p = (double)freq[i] / n;
            H -= p * log(p);
        }
    }
    return H;
}

double empirical_entropy(const KString* s) {
    return shannon_entropy_bits(s);
}

double block_entropy(const KString* s, int block_size) {
    if (!s || block_size < 1 || s->len < (size_t)block_size) return 0.0;
    size_t n_blocks = s->len / (size_t)block_size;

    /* Use hash map for block frequency */
    /* Simple approach: collect block hashes, sort, count runs */
    unsigned int* hashes = (unsigned int*)malloc(n_blocks * sizeof(unsigned int));
    assert(hashes != NULL);
    for (size_t i = 0; i < n_blocks; i++) {
        unsigned int h = 0;
        for (int j = 0; j < block_size; j++)
            h = h * 31 + s->data[i * (size_t)block_size + (size_t)j];
        hashes[i] = h;
    }

    /* Sort by insertion (OK for moderate n_blocks) */
    for (size_t i = 1; i < n_blocks; i++) {
        unsigned int key = hashes[i];
        size_t j = i;
        while (j > 0 && hashes[j - 1] > key) {
            hashes[j] = hashes[j - 1];
            j--;
        }
        hashes[j] = key;
    }

    double H = 0.0;
    size_t run_start = 0;
    while (run_start < n_blocks) {
        size_t run_end = run_start + 1;
        while (run_end < n_blocks && hashes[run_end] == hashes[run_start])
            run_end++;
        size_t count = run_end - run_start;
        double p = (double)count / (double)n_blocks;
        H -= p * log2(p);
        run_start = run_end;
    }
    free(hashes);
    return H / (double)block_size; /* per-symbol entropy */
}

double conditional_entropy(const KString* x, const KString* y) {
    /*
     * H(Y|X) = H(X,Y) - H(X)
     * Estimate using concatenated string.
     */
    if (!x || !y || x->len == 0 || y->len == 0) return 0.0;

    KString* xy = kstr_create(x->len + y->len);
    kstr_append_str(xy, x);
    kstr_append_str(xy, y);
    double H_xy = shannon_entropy_bits(xy);
    double H_x  = shannon_entropy_bits(x);
    kstr_free(xy);
    return H_xy - H_x;
}

double mutual_information(const KString* x, const KString* y) {
    /*
     * I(X;Y) = H(X) + H(Y) - H(X,Y)
     */
    if (!x || !y) return 0.0;
    double Hx = shannon_entropy_bits(x);
    double Hy = shannon_entropy_bits(y);
    KString* xy = kstr_create(x->len + y->len);
    kstr_append_str(xy, x);
    kstr_append_str(xy, y);
    double Hxy = shannon_entropy_bits(xy);
    kstr_free(xy);
    double I = Hx + Hy - Hxy;
    return I > 0.0 ? I : 0.0;
}

double entropy_rate_estimate(const KString* s, int max_block) {
    if (!s || max_block < 1) return 0.0;
    double sum = 0.0;
    int count = 0;
    for (int k = 1; k <= max_block && k <= (int)s->len; k++) {
        double Hk = block_entropy(s, k);
        if (Hk > 0.0) { sum += Hk; count++; }
    }
    return count > 0 ? sum / (double)count : 0.0;
}

/* ══════════════════════════════════════════════════════════════
   Shannon Lower Bound
   ══════════════════════════════════════════════════════════════ */

double shannon_lower_bound(const KString* s) {
    /*
     * Shannon's source coding theorem (1948):
     * For any uniquely decodable code for i.i.d. source X,
     * expected code length L ≥ H(X).
     * For string x from such source, total code length ≥ |x|·H.
     */
    if (!s || s->len == 0) return 0.0;
    return shannon_entropy_bits(s);
}

double optimal_code_length(const KString* s) {
    if (!s || s->len == 0) return 0.0;
    return (double)s->len * shannon_entropy_bits(s) / 8.0;
}

/* ══════════════════════════════════════════════════════════════
   Normalized measures
   ══════════════════════════════════════════════════════════════ */

double normalized_entropy(const KString* s, int alphabet_size) {
    if (!s || s->len == 0 || alphabet_size <= 1) return 0.0;
    double H_max = log2((double)alphabet_size);
    if (H_max == 0.0) return 0.0;
    return shannon_entropy_bits(s) / H_max;
}

double redundancy(const KString* s, int alphabet_size) {
    double nH = normalized_entropy(s, alphabet_size);
    return 1.0 - nH;
}

/* ══════════════════════════════════════════════════════════════
   Jensen-Shannon Divergence
   ══════════════════════════════════════════════════════════════ */

static void compute_dist(const KString* s, double* dist) {
    memset(dist, 0, 256 * sizeof(double));
    if (!s || s->len == 0) return;
    for (size_t i = 0; i < s->len; i++)
        dist[s->data[i]] += 1.0;
    for (int i = 0; i < 256; i++)
        dist[i] /= (double)s->len;
}

static double kl_divergence(const double* P, const double* Q, int n) {
    double d = 0.0;
    for (int i = 0; i < n; i++) {
        if (P[i] > 0.0 && Q[i] > 0.0)
            d += P[i] * log2(P[i] / Q[i]);
        else if (P[i] > 0.0 && Q[i] == 0.0)
            d += 1e10; /* penalty for zero Q */
    }
    return d;
}

double jensen_shannon_divergence(const KString* a, const KString* b) {
    if (!a || !b) return 0.0;
    double Pa[256], Pb[256], M[256];
    compute_dist(a, Pa);
    compute_dist(b, Pb);
    for (int i = 0; i < 256; i++)
        M[i] = 0.5 * (Pa[i] + Pb[i]);
    return 0.5 * kl_divergence(Pa, M, 256) + 0.5 * kl_divergence(Pb, M, 256);
}

/* ══════════════════════════════════════════════════════════════
   Typical sequences (AEP)
   ══════════════════════════════════════════════════════════════ */

int is_typical(const KString* s, double epsilon) {
    if (!s || s->len == 0) return 1;
    double H = shannon_entropy_bits(s);

    /* Compute empirical -log P(x)/n */
    int freq[256] = {0};
    for (size_t i = 0; i < s->len; i++) freq[s->data[i]]++;
    double logP = 0.0;
    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) {
            double p = (double)freq[i] / (double)s->len;
            logP += (double)freq[i] * log2(p);
        }
    }
    double H_emp = -logP / (double)s->len;
    return (fabs(H_emp - H) <= epsilon) ? 1 : 0;
}

size_t typical_set_size(const KString* s, double epsilon) {
    if (!s || s->len == 0) return 1;
    double H = shannon_entropy_bits(s);
    /* |A_ε| ≈ 2^{nH} (upper bound: 2^{n(H+ε)}) */
    return (size_t)pow(2.0, (double)s->len * (H + epsilon));
}

/* ══════════════════════════════════════════════════════════════
   Kolmogorov-Entropy gap
   ══════════════════════════════════════════════════════════════ */

double kolmogorov_entropy_gap(const KString* s) {
    if (!s || s->len == 0) return 0.0;
    double H = shannon_entropy_bits(s);
    /* K(x) estimate: use LZW-compressed size as heuristic */
    double K_est = lzw_ratio(s);
    return fabs(K_est - (H / 8.0));
}

/* ══════════════════════════════════════════════════════════════
   Min-entropy and Rényi entropy
   ══════════════════════════════════════════════════════════════ */

double min_entropy(const KString* s) {
    if (!s || s->len == 0) return 0.0;
    int freq[256] = {0};
    for (size_t i = 0; i < s->len; i++) freq[s->data[i]]++;
    double max_p = 0.0;
    for (int i = 0; i < 256; i++) {
        double p = (double)freq[i] / (double)s->len;
        if (p > max_p) max_p = p;
    }
    return -log2(max_p);
}

double renyi_entropy(const KString* s, double alpha) {
    /*
     * Rényi entropy of order α:
     *   H_α = (1/(1-α)) log₂ Σ p_i^α
     *
     * Special cases:
     *   α → 1: Shannon entropy
     *   α = 2:  Collision entropy H₂ = -log₂ Σ p_i²
     *   α → ∞: Min-entropy
     */
    if (!s || s->len == 0) return 0.0;
    int freq[256] = {0};
    for (size_t i = 0; i < s->len; i++) freq[s->data[i]]++;

    if (fabs(alpha - 1.0) < 1e-6) return shannon_entropy_bits(s);
    if (alpha > 100.0) return min_entropy(s);

    double sum_pow = 0.0;
    for (int i = 0; i < 256; i++) {
        double p = (double)freq[i] / (double)s->len;
        if (p > 0.0) sum_pow += pow(p, alpha);
    }
    if (sum_pow == 0.0) return 0.0;
    return log2(sum_pow) / (1.0 - alpha);
}

/* ══════════════════════════════════════════════════════════════
   Self-test
   ══════════════════════════════════════════════════════════════ */

#ifdef ENTROPY_MAIN
int main(void) {
    printf("=== Entropy Self-Test ===\n");

    KString* s1 = kstr_from_cstr("aaaaaa");
    KString* s2 = kstr_from_cstr("abcdef");
    KString* s3 = kstr_from_cstr("The rain in Spain stays mainly in the plain");

    printf("H(AAAAAA) = %.4f bits\n", shannon_entropy_bits(s1));
    printf("H(ABCDEF) = %.4f bits\n", shannon_entropy_bits(s2));
    printf("H(text)   = %.4f bits\n", shannon_entropy_bits(s3));

    printf("\nNormalized (alpha=256):\n");
    printf("  s1: %.4f\n", normalized_entropy(s1, 256));
    printf("  s2: %.4f\n", normalized_entropy(s2, 256));
    printf("  s3: %.4f\n", normalized_entropy(s3, 256));

    printf("\nBlock entropy (k=2):\n");
    printf("  s3: %.4f\n", block_entropy(s3, 2));

    printf("\nMin-entropy:\n");
    printf("  s1: %.4f, s2: %.4f, s3: %.4f\n",
           min_entropy(s1), min_entropy(s2), min_entropy(s3));

    printf("\nRényi entropy:\n");
    printf("  α=0.5: %.4f\n", renyi_entropy(s3, 0.5));
    printf("  α=2.0: %.4f\n", renyi_entropy(s3, 2.0));
    printf("  α=∞:   %.4f\n", min_entropy(s3));

    printf("\nMutual information: I(s2; s3) = %.4f\n",
           mutual_information(s2, s3));

    printf("\nJSD(s1, s2) = %.6f\n", jensen_shannon_divergence(s1, s2));

    printf("\nShannon lower bound (bytes): %.2f\n",
           shannon_lower_bound(s3));
    printf("Optimal code length (bytes): %.2f\n",
           optimal_code_length(s3));

    printf("\nTypicality (ε=0.5): %d\n", is_typical(s3, 0.5));
    printf("Typical set size: %zu\n", typical_set_size(s3, 0.5));

    printf("\nEntropy rate estimate: %.4f\n",
           entropy_rate_estimate(s3, 3));

    kstr_free(s1); kstr_free(s2); kstr_free(s3);
    printf("\n=== All entropy tests passed ===\n");
    return 0;
}
#endif /* ENTROPY_MAIN */
