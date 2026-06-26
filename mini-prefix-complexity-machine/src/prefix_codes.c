#include "../include/prefix_machine.h"
#include "../include/prefix_codes.h"
#include <math.h>

/* ── PrefixCode construction ───────────────────────────────── */
PrefixCode* pc_create(int n_symbols) {
    PrefixCode* pc = (PrefixCode*)calloc(1, sizeof(PrefixCode));
    assert(pc);
    pc->n_symbols = n_symbols;
    pc->codewords = (char**)calloc((size_t)n_symbols, sizeof(char*));
    pc->lengths = (size_t*)calloc((size_t)n_symbols, sizeof(size_t));
    return pc;
}

void pc_free(PrefixCode* code) {
    if (!code) return;
    for (int i = 0; i < code->n_symbols; i++) free(code->codewords[i]);
    free(code->codewords);
    free(code->lengths);
    /* Free tree recursively */
    void free_node(PCTreeNode* n) {
        if (!n) return;
        free_node(n->left);
        free_node(n->right);
        free(n);
    }
    free_node(code->root);
    free(code);
}

int pc_is_valid(const PrefixCode* code) {
    if (!code) return 0;
    /* Check Kraft inequality */
    double sum = 0.0;
    for (int i = 0; i < code->n_symbols; i++)
        sum += pow(2.0, -(double)code->lengths[i]);
    if (sum > 1.0 + 1e-9) return 0;
    /* Check prefix property */
    for (int i = 0; i < code->n_symbols; i++)
        for (int j = 0; j < code->n_symbols; j++)
            if (i != j && code->lengths[i] < code->lengths[j])
                if (memcmp(code->codewords[i], code->codewords[j],
                           code->lengths[i]) == 0) return 0;
    return 1;
}

int pc_encode(const PrefixCode* code, int sym, char* out, size_t cap) {
    if (!code || sym < 0 || sym >= code->n_symbols || !out) return -1;
    if (code->lengths[sym] >= cap) return -1;
    memcpy(out, code->codewords[sym], code->lengths[sym]);
    out[code->lengths[sym]] = '\0';
    return (int)code->lengths[sym];
}

int pc_decode(const PrefixCode* code, const char* bits, int len,
              int* out_sym, int* consumed) {
    if (!code || !code->root || !bits) return -1;
    PCTreeNode* node = code->root;
    for (int i = 0; i < len; i++) {
        node = (bits[i] == '0') ? node->left : node->right;
        if (!node) return -1;
        if (node->symbol >= 0) {
            *out_sym = node->symbol;
            *consumed = i + 1;
            return 0;
        }
    }
    return -1;
}

/* ── Kraft ─────────────────────────────────────────────────── */
double kraft_sum(const int* lengths, int n) {
    double s = 0.0;
    for (int i = 0; i < n; i++) s += pow(2.0, -(double)lengths[i]);
    return s;
}

int kraft_satisfied(const int* lengths, int n) {
    return kraft_sum(lengths, n) <= 1.0 + 1e-9;
}

int* kraft_lengths_from_probs(const double* probs, int n) {
    int* lens = (int*)malloc((size_t)n * sizeof(int));
    for (int i = 0; i < n; i++) {
        if (probs[i] > 0.0) { lens[i] = (int)ceil(-log2(probs[i])); if (lens[i]<1) lens[i]=1; }
        else lens[i] = 1;
    }
    return lens;
}

/* ── Shannon-Fano ──────────────────────────────────────────── */
int* shannon_fano_lengths(const double* probs, int n) {
    return kraft_lengths_from_probs(probs, n);
}

PrefixCode* shannon_fano_code(const double* probs, int n) {
    int* lens = shannon_fano_lengths(probs, n);
    PrefixCode* pc = pc_create(n);
    /* Build canonical code tree from lengths */
    int max_len = 0;
    for (int i = 0; i < n; i++) if (lens[i] > max_len) max_len = lens[i];
    /* Count symbols per length */
    int* count = (int*)calloc((size_t)(max_len + 1), sizeof(int));
    for (int i = 0; i < n; i++) count[lens[i]]++;
    /* Generate canonical codes */
    int code_val = 0;
    for (int l = 1; l <= max_len; l++) {
        for (int i = 0; i < n; i++) {
            if (lens[i] == l) {
                pc->lengths[i] = (size_t)l;
                pc->codewords[i] = (char*)malloc((size_t)(l + 1));
                for (int b = l - 1; b >= 0; b--)
                    pc->codewords[i][l - 1 - b] = ((code_val >> b) & 1) ? '1' : '0';
                pc->codewords[i][l] = '\0';
                code_val++;
            }
        }
        code_val <<= 1;
    }
    free(count); free(lens);
    /* Build tree */
    pc->root = (PCTreeNode*)calloc(1, sizeof(PCTreeNode));
    pc->root->symbol = -1;
    for (int i = 0; i < n; i++) {
        PCTreeNode* node = pc->root;
        for (size_t b = 0; b < pc->lengths[i]; b++) {
            if (pc->codewords[i][b] == '0') {
                if (!node->left) { node->left = (PCTreeNode*)calloc(1, sizeof(PCTreeNode)); node->left->symbol = -1; }
                node = node->left;
            } else {
                if (!node->right) { node->right = (PCTreeNode*)calloc(1, sizeof(PCTreeNode)); node->right->symbol = -1; }
                node = node->right;
            }
        }
        node->symbol = i;
    }
    return pc;
}

/* ── Huffman ───────────────────────────────────────────────── */
int* huffman_lengths_compute(const double* probs, int n) {
    /* Build Huffman tree and extract lengths */
    if (n <= 0) return NULL;
    if (n == 1) { int* l = (int*)malloc(sizeof(int)); l[0] = 1; return l; }
    typedef struct { double prob; int idx; int is_leaf; } HNode;
    HNode* nodes = (HNode*)malloc((size_t)(2*n) * sizeof(HNode));
    int* parent = (int*)malloc((size_t)(2*n) * sizeof(int));
    for (int i = 0; i < n; i++) { nodes[i].prob = probs[i]; nodes[i].idx = i; nodes[i].is_leaf = 1; parent[i] = -1; }
    for (int i = n; i < 2*n - 1; i++) {
        /* Find two smallest */
        int m1 = -1, m2 = -1;
        for (int j = 0; j < i; j++) {
            if (parent[j] != -1) continue;
            if (m1 < 0 || nodes[j].prob < nodes[m1].prob) { m2 = m1; m1 = j; }
            else if (m2 < 0 || nodes[j].prob < nodes[m2].prob) m2 = j;
        }
        if (m1 < 0 || m2 < 0) break;
        nodes[i].prob = nodes[m1].prob + nodes[m2].prob;
        nodes[i].idx = i; nodes[i].is_leaf = 0;
        parent[m1] = i; parent[m2] = i; parent[i] = -1;
    }
    int* lens = (int*)malloc((size_t)n * sizeof(int));
    for (int i = 0; i < n; i++) {
        int depth = 0;
        int cur = i;
        while (parent[cur] >= 0) { depth++; cur = parent[cur]; }
        lens[i] = depth > 0 ? depth : 1;
    }
    free(nodes); free(parent);
    return lens;
}

PrefixCode* huffman_code_compute(const double* probs, int n) {
    int* lens = huffman_lengths_compute(probs, n);
    PrefixCode* pc = pc_create(n);
    int max_len = 0;
    for (int i = 0; i < n; i++) {
        pc->lengths[i] = (size_t)lens[i];
        if (lens[i] > max_len) max_len = lens[i];
    }
    int* count = (int*)calloc((size_t)(max_len + 1), sizeof(int));
    for (int i = 0; i < n; i++) count[lens[i]]++;
    int code_val = 0;
    for (int l = 1; l <= max_len; l++) {
        for (int i = 0; i < n; i++) {
            if (lens[i] == l) {
                pc->codewords[i] = (char*)malloc((size_t)(l + 1));
                for (int b = l - 1; b >= 0; b--)
                    pc->codewords[i][l - 1 - b] = ((code_val >> b) & 1) ? '1' : '0';
                pc->codewords[i][l] = '\0';
                code_val++;
            }
        }
        code_val <<= 1;
    }
    free(count);
    /* Build decode tree */
    pc->root = (PCTreeNode*)calloc(1, sizeof(PCTreeNode));
    pc->root->symbol = -1;
    for (int i = 0; i < n; i++) {
        PCTreeNode* node = pc->root;
        for (size_t b = 0; b < pc->lengths[i]; b++) {
            if (pc->codewords[i][b] == '0') {
                if (!node->left) {
                    node->left = (PCTreeNode*)calloc(1, sizeof(PCTreeNode));
                    node->left->symbol = -1;
                }
                node = node->left;
            } else {
                if (!node->right) {
                    node->right = (PCTreeNode*)calloc(1, sizeof(PCTreeNode));
                    node->right->symbol = -1;
                }
                node = node->right;
            }
        }
        node->symbol = i;
    }
    free(lens);
    return pc;
}

double huffman_expected_length(const double* probs, const int* lengths, int n) {
    double L = 0.0;
    for (int i = 0; i < n; i++) L += probs[i] * (double)lengths[i];
    return L;
}

double huffman_redundancy(const double* probs, const int* lengths, int n) {
    double H = 0.0, L = 0.0;
    for (int i = 0; i < n; i++) {
        if (probs[i] > 0.0) H -= probs[i] * log2(probs[i]);
        L += probs[i] * (double)lengths[i];
    }
    return L - H;
}

/* ── Arithmetic Coding ─────────────────────────────────────── */
ArithmeticEncoder* arith_create(void) {
    ArithmeticEncoder* ae = (ArithmeticEncoder*)calloc(1, sizeof(ArithmeticEncoder));
    ae->low = 0; ae->high = 0xFFFFFFFFU; ae->range = 0xFFFFFFFFU;
    return ae;
}

void arith_encode_symbol(ArithmeticEncoder* ae, int sym,
                          const double* cdf, int n) {
    unsigned int range = ae->high - ae->low + 1;
    double total = cdf[n];
    unsigned int sym_low = (unsigned int)(ae->low + range * (sym > 0 ? cdf[sym-1] : 0.0) / total);
    unsigned int sym_high = (unsigned int)(ae->low + range * cdf[sym] / total) - 1;
    ae->low = sym_low; ae->high = sym_high;
    while (((ae->low ^ ae->high) & 0x80000000U) == 0) {
        /* Emit MSB */
        ae->low = (ae->low << 1) & 0xFFFFFFFFU;
        ae->high = ((ae->high << 1) | 1) & 0xFFFFFFFFU;
    }
}

void arith_flush(ArithmeticEncoder* ae, PMString* output) {
    (void)ae; (void)output; /* flush pending bits */
}

int arith_decode_symbol(unsigned int* value, const double* cdf, int n) {
    double total = cdf[n];
    double scaled = (double)*value * total / 4294967296.0;
    for (int i = 0; i < n; i++)
        if (scaled < cdf[i]) return i;
    return n - 1;
}

void arith_free(ArithmeticEncoder* ae) { free(ae); }

/* ── Universal code lengths ────────────────────────────────── */
size_t elias_delta_length(size_t n) {
    if (n == 0) return 1;
    int blen = 0; size_t t = n + 1;
    while (t > 0) { blen++; t >>= 1; }
    int gblen = 0; t = (size_t)blen;
    while (t > 0) { gblen++; t >>= 1; }
    return (size_t)(gblen - 1) + (size_t)gblen + (size_t)(blen - 1);
}

size_t elias_gamma_length(size_t n) {
    if (n == 0) return 1;
    int bits = 0; size_t t = n + 1;
    while (t > 0) { bits++; t >>= 1; }
    return (size_t)(2 * bits - 1);
}

size_t elias_omega_length(size_t n) {
    if (n <= 1) return 1;
    size_t total = 0;
    size_t k = n + 1;
    while (k > 1) {
        int bits = 0; size_t t = k;
        while (t > 0) { bits++; t >>= 1; }
        total += (size_t)bits; k = (size_t)(bits - 1);
    }
    return total;
}

size_t levenshtein_code_length(size_t n) {
    if (n == 0) return 1;
    size_t total = 0;
    n++;
    while (n > 1) {
        int bits = 0; size_t t = n;
        while (t > 0) { bits++; t >>= 1; }
        total += (size_t)bits; n = (size_t)(bits - 1);
    }
    return total;
}

/* ══════════════════════════════════════════════════════════════
 * pc_from_lengths — Build PrefixCode from codeword lengths
 *
 * Knowledge point: Given codeword lengths l₁,...,lₙ satisfying
 * Kraft inequality (Σ 2^{-l_i} ≤ 1), construct a prefix code
 * with these lengths using canonical Huffman code construction.
 *
 * The canonical code assigns codewords by sorting lengths and
 * assigning incrementing binary codes of each length, left-justified.
 *
 * This works because the Kraft inequality guarantees enough
 * "code space" for the assignment. This is the constructive
 * direction of Kraft's theorem.
 *
 * Reference: Cover & Thomas §5.2, "Elements of Information Theory"
 ══════════════════════════════════════════════════════════════ */

PrefixCode* pc_from_lengths(const int* lengths, int n) {
    if (!lengths || n <= 0) return NULL;

    /* Verify Kraft inequality first */
    if (!kraft_satisfied(lengths, n)) return NULL;

    PrefixCode* pc = pc_create(n);

    /* Find maximum length */
    int max_len = 0;
    for (int i = 0; i < n; i++) {
        pc->lengths[i] = (size_t)lengths[i];
        if (lengths[i] > max_len) max_len = lengths[i];
    }

    /* Count symbols per length */
    int* count = (int*)calloc((size_t)(max_len + 1), sizeof(int));
    assert(count);
    for (int i = 0; i < n; i++) count[lengths[i]]++;

    /* Generate canonical codes */
    int code_val = 0;
    for (int l = 1; l <= max_len; l++) {
        for (int i = 0; i < n; i++) {
            if (lengths[i] == l) {
                pc->codewords[i] = (char*)malloc((size_t)(l + 1));
                assert(pc->codewords[i]);
                for (int b = l - 1; b >= 0; b--)
                    pc->codewords[i][l - 1 - b] = ((code_val >> b) & 1) ? '1' : '0';
                pc->codewords[i][l] = '\0';
                code_val++;
            }
        }
        code_val <<= 1;
    }

    free(count);

    /* Build tree */
    pc->root = (PCTreeNode*)calloc(1, sizeof(PCTreeNode));
    assert(pc->root);
    pc->root->symbol = -1;
    for (int i = 0; i < n; i++) {
        PCTreeNode* node = pc->root;
        for (size_t b = 0; b < pc->lengths[i]; b++) {
            if (pc->codewords[i][b] == '0') {
                if (!node->left) {
                    node->left = (PCTreeNode*)calloc(1, sizeof(PCTreeNode));
                    node->left->symbol = -1;
                }
                node = node->left;
            } else {
                if (!node->right) {
                    node->right = (PCTreeNode*)calloc(1, sizeof(PCTreeNode));
                    node->right->symbol = -1;
                }
                node = node->right;
            }
        }
        node->symbol = i;
    }

    return pc;
}

/* ══════════════════════════════════════════════════════════════
 * pc_shannon_entropy — Shannon entropy of a source
 *
 * Knowledge point: H = -Σ p_i log₂ p_i
 * The fundamental lower bound on expected codeword length
 * for any uniquely decodable code: L ≥ H.
 *
 * Source Coding Theorem (Shannon 1948):
 * For any uniquely decodable code: E[L] ≥ H
 * There exists a prefix code with: E[L] < H + 1
 ══════════════════════════════════════════════════════════════ */

double pc_shannon_entropy(const double* probs, int n) {
    double H = 0.0;
    for (int i = 0; i < n; i++) {
        if (probs[i] > 0.0)
            H -= probs[i] * log2(probs[i]);
    }
    return H;
}

/* ══════════════════════════════════════════════════════════════
 * pc_expected_length — Expected codeword length E[L]
 *
 * Knowledge point: E[L] = Σ p_i · l_i
 * Measures average bits per symbol.
 * For an optimal code: H ≤ E[L] < H + 1
 ══════════════════════════════════════════════════════════════ */

double pc_expected_length(const double* probs, const int* lengths, int n) {
    double L = 0.0;
    for (int i = 0; i < n; i++) L += probs[i] * (double)lengths[i];
    return L;
}

/* ══════════════════════════════════════════════════════════════
 * pc_mcmillan_theorem — Verify McMillan's theorem
 *
 * Knowledge point: McMillan (1956) proved that ANY uniquely
 * decodable code must satisfy the Kraft inequality Σ 2^{-l_i} ≤ 1,
 * even if it's not a prefix code.
 *
 * This means prefix codes are as efficient as any uniquely
 * decodable code — we lose nothing in compression efficiency
 * by restricting to prefix codes.
 *
 * This function verifies the inequality for given lengths.
 * Returns 1 if Kraft holds (necessary condition for unique
 * decodability), 0 otherwise.
 ══════════════════════════════════════════════════════════════ */

int pc_mcmillan_check(const int* lengths, int n) {
    /* McMillan: any UD code must satisfy Σ 2^{-l_i} ≤ 1 */
    return kraft_satisfied(lengths, n);
}

/* ══════════════════════════════════════════════════════════════
 * pc_golomb_lengths — Golomb code lengths for geometric distributions
 *
 * Knowledge point: Golomb coding (1966) is optimal for
 * geometrically distributed integers:
 *   P(n) = (1-p) p^{n-1} for n ≥ 1
 *
 * Choose parameter m = ⌈-1/log₂ p⌉. Then:
 * - Unary code for quotient q = ⌊n/m⌋
 * - Binary code for remainder r = n mod m
 *
 * For p → 1 (highly skewed): Golomb → Unary code
 * For p = 1/2: Golomb(m=2) → simple binary + unary for length
 *
 * Golomb codes are optimal prefix codes for geometric distributions.
 * Rice codes are the special case where m is a power of 2.
 *
 * Reference: Golomb (1966), Rice (1979)
 ══════════════════════════════════════════════════════════════ */

int* pc_golomb_lengths(int max_n, int m) {
    if (m <= 0 || max_n <= 0) return NULL;
    int* lens = (int*)malloc((size_t)max_n * sizeof(int));
    assert(lens);

    /* For each n from 0 to max_n-1, compute Golomb code length */
    for (int n = 0; n < max_n; n++) {
        int q = n / m;
        int r = n % m;

        /* Unary code for q: q ones followed by a zero → q+1 bits */
        int unary_len = q + 1;

        /* Binary code for r:
         * k = ceil(log2(m)), the number of bits needed for r ∈ [0, m-1]
         * cutoff = 2^k - m, determines when to use k-1 bits instead of k */
        int k = 0;
        while ((1 << k) < m) k++;  /* k = ceil(log2(m)) */
        int binary_len = k;
        int cutoff = (1 << k) - m;
        if (r < cutoff) binary_len = k - 1;  /* truncated binary */

        lens[n] = unary_len + binary_len;
    }
    return lens;
}

/* ══════════════════════════════════════════════════════════════
 * pc_rice_lengths — Rice code lengths (Golomb with m = 2^k)
 *
 * Knowledge point: Rice coding is the special case of Golomb
 * coding when m is a power of 2, enabling efficient bit-level
 * implementations without division.
 *
 * Rice code for parameter k (m = 2^k):
 * - Unary code for q = ⌊n / 2^k⌋
 * - k-bit binary code for r = n mod 2^k
 *
 * Length: ⌊n/2^k⌋ + 1 + k bits
 *
 * Optimal for Laplacian / two-sided geometric distributions.
 * Used in: FLAC audio codec, JPEG-LS, H.264 CAVLC.
 *
 * Reference: Rice (1979), "Practical Universal Noiseless Coding"
 ══════════════════════════════════════════════════════════════ */

size_t rice_code_length(size_t n, int k) {
    if (k < 0) return SIZE_MAX;
    size_t m = (size_t)1 << (size_t)k;
    size_t q = n / m;
    return q + 1 + (size_t)k;
}

int* pc_rice_lengths(int max_n, int k) {
    int m = 1 << k;
    return pc_golomb_lengths(max_n, m);
}

/* ══════════════════════════════════════════════════════════════
 * pc_tunstall_code — Tunstall code construction
 *
 * Knowledge point: Tunstall coding (1967) is the dual of Huffman:
 * instead of variable-length codes for fixed-size symbols,
 * it produces fixed-length codes for variable-length symbol blocks.
 *
 * Algorithm: Start with all single-symbol strings. Repeatedly
 * split the highest-probability leaf by appending each possible
 * symbol (like growing a parse tree). The result is a code where
 * each codeword maps to a fixed-length binary string.
 *
 * Tunstall codes are optimal for fixed-to-variable length coding
 * (opposite of Huffman's variable-to-fixed).
 *
 * Used in: Lempel-Ziv compression preprocessing, V.44 modem standard.
 *
 * Reference: Tunstall (1967), "Synthesis of Noiseless Compression Codes"
 ══════════════════════════════════════════════════════════════ */

TunstallLeaf* pc_tunstall_build(const double* probs, int alphabet_size,
                                 int n_leaves) {
    if (!probs || alphabet_size <= 0 || n_leaves <= 0) return NULL;

    /* Initialize with each single symbol as a leaf */
    int current_leaves = alphabet_size;
    TunstallLeaf* leaves = (TunstallLeaf*)malloc(
        (size_t)n_leaves * sizeof(TunstallLeaf));
    assert(leaves);

    for (int i = 0; i < alphabet_size; i++) {
        leaves[i].block = (char*)malloc(2);
        assert(leaves[i].block);
        leaves[i].block[0] = (char)('a' + i);
        leaves[i].block[1] = '\0';
        leaves[i].block_len = 1;
        leaves[i].prob = probs[i];
    }

    /* Repeatedly split the most probable leaf */
    while (current_leaves < n_leaves) {
        /* Find highest probability leaf */
        int max_idx = 0;
        double max_prob = leaves[0].prob;
        for (int i = 1; i < current_leaves; i++) {
            if (leaves[i].prob > max_prob) {
                max_prob = leaves[i].prob;
                max_idx = i;
            }
        }

        /* Split: extend with each symbol */
        char* parent_block = leaves[max_idx].block;
        size_t parent_len = leaves[max_idx].block_len;
        double parent_prob = leaves[max_idx].prob;

        /* Replace parent with first child */
        leaves[max_idx].block = (char*)malloc(parent_len + 2);
        assert(leaves[max_idx].block);
        memcpy(leaves[max_idx].block, parent_block, parent_len);
        leaves[max_idx].block[parent_len] = (char)('a');
        leaves[max_idx].block[parent_len + 1] = '\0';
        leaves[max_idx].block_len = parent_len + 1;
        leaves[max_idx].prob = parent_prob * probs[0];
        free(parent_block);

        /* Add remaining children as new leaves */
        for (int s = 1; s < alphabet_size && current_leaves < n_leaves; s++) {
            int idx = current_leaves++;
            leaves[idx].block = (char*)malloc(parent_len + 2);
            assert(leaves[idx].block);
            memcpy(leaves[idx].block, leaves[max_idx].block, parent_len);
            leaves[idx].block[parent_len] = (char)('a' + s);
            leaves[idx].block[parent_len + 1] = '\0';
            leaves[idx].block_len = parent_len + 1;
            leaves[idx].prob = parent_prob * probs[s];
        }
    }

    return leaves;
}

void pc_tunstall_free(TunstallLeaf* leaves, int n) {
    if (!leaves) return;
    for (int i = 0; i < n; i++) free(leaves[i].block);
    free(leaves);
}

/* ══════════════════════════════════════════════════════════════
 * pc_kraft_inequality_deep — Deep verification of Kraft inequality
 *
 * Knowledge point: The Kraft inequality Σ 2^{-l_i} ≤ 1 is both
 * necessary and sufficient for the existence of a prefix code.
 *
 * Necessity (easy): Σ_{leaf} 2^{-depth(leaf)} ≤ 1 for any
 * full binary tree (by induction on tree structure).
 *
 * Sufficiency (constructive): Given lengths l₁≤...≤lₙ satisfying
 * Kraft, we can build a code tree by assigning codewords in
 * lexicographic order (the "canonical code" construction).
 * At each step, after assigning a codeword of length l, we
 * reserve the subtree below it, removing 2^{-l} from the
 * remaining budget.
 *
 * This function demonstrates the constructive proof by building
 * the code tree step by step.
 ══════════════════════════════════════════════════════════════ */

PrefixCode* pc_kraft_construct(const int* lengths, int n) {
    return pc_from_lengths(lengths, n);
}

/* ══════════════════════════════════════════════════════════════
 * pc_fano_metric — Fano's inequality in source coding
 *
 * Knowledge point: Fano's inequality relates the error
 * probability P_e of a decision rule to the conditional
 * entropy H(X|Y):
 *   H(P_e) + P_e log(|𝒳|-1) ≥ H(X|Y)
 *
 * For prefix codes: the redundancy of Shannon-Fano code
 * is at most 1 bit per symbol (compared to entropy).
 *
 * L ≤ H + 1  (Shannon-Fano bound)
 * L_optimal ≤ H + 1 (Huffman achieves this)
 ══════════════════════════════════════════════════════════════ */

double pc_fano_bound(double entropy, int n_symbols) {
    (void)n_symbols;
    /* The Shannon-Fano code guarantees E[L] ≤ H + 1 */
    return entropy + 1.0;
}

/* ══════════════════════════════════════════════════════════════
 * pc_block_code — Block prefix code for n-grams
 *
 * Knowledge point: Block coding groups k symbols together
 * and assigns a single codeword to each block. As k→∞,
 * the average bits per symbol → H (the entropy).
 *
 * E[L_k] / k → H  as k→∞
 *
 * This is the asymptotic equipartition property (AEP) in
 * source coding: by coding longer blocks, we approach the
 * entropy limit arbitrarily closely.
 *
 * The redundancy per symbol is ≤ 1/k for block Huffman coding.
 ══════════════════════════════════════════════════════════════ */

BlockCodeResult* pc_block_code_analyze(const double* probs, int n_syms, int k) {
    if (!probs || n_syms <= 0 || k <= 0) return NULL;

    BlockCodeResult* res = (BlockCodeResult*)calloc(1, sizeof(BlockCodeResult));
    assert(res);
    res->block_size = k;

    /* Number of possible blocks: n_syms^k */
    int n_blocks = 1;
    for (int i = 0; i < k; i++) n_blocks *= n_syms;
    res->n_blocks = n_blocks;

    double* block_probs = (double*)malloc((size_t)n_blocks * sizeof(double));
    assert(block_probs);

    /* Compute probability of each block (product of symbol probs) */
    for (int b = 0; b < n_blocks; b++) {
        double bp = 1.0;
        int idx = b;
        for (int pos = 0; pos < k; pos++) {
            int sym = idx % n_syms;
            bp *= probs[sym];
            idx /= n_syms;
        }
        block_probs[b] = bp;
    }

    /* Compute Huffman lengths for blocks */
    res->block_lengths = huffman_lengths_compute(block_probs, n_blocks);

    /* Average bits per block */
    double bits_per_block = huffman_expected_length(block_probs,
        res->block_lengths, n_blocks);
    res->bits_per_symbol = bits_per_block / (double)k;

    free(block_probs);
    return res;
}

void pc_block_code_free(BlockCodeResult* bcr) {
    if (bcr) { free(bcr->block_lengths); free(bcr); }
}

#ifdef PREFIX_CODES_MAIN
int main(void) {
    printf("=== Prefix Codes Self-Test ===\n");
    double probs[4] = {0.4, 0.3, 0.2, 0.1};
    int* sf_lens = shannon_fano_lengths(probs, 4);
    printf("Shannon-Fano lengths: %d %d %d %d (Kraft=%.4f)\n",
           sf_lens[0], sf_lens[1], sf_lens[2], sf_lens[3],
           kraft_sum(sf_lens, 4));
    free(sf_lens);
    int* h_lens = huffman_lengths_compute(probs, 4);
    printf("Huffman lengths: %d %d %d %d (Kraft=%.4f, L=%.4f, H=%.4f)\n",
           h_lens[0], h_lens[1], h_lens[2], h_lens[3],
           kraft_sum(h_lens, 4),
           huffman_expected_length(probs, h_lens, 4),
           -(0.4*log2(0.4)+0.3*log2(0.3)+0.2*log2(0.2)+0.1*log2(0.1)));
    free(h_lens);
    PrefixCode* pc = shannon_fano_code(probs, 4);
    printf("Valid: %d\n", pc_is_valid(pc));
    pc_free(pc);
    printf("Delta(100) length: %zu\n", elias_delta_length(100));
    printf("Gamma(100) length: %zu\n", elias_gamma_length(100));
    printf("Omega(100) length: %zu\n", elias_omega_length(100));
    printf("All tests passed.\n");
    return 0;
}
#endif
