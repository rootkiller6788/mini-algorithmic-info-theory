/*
 * incompressibility.h — Incompressibility Method Core Definitions
 *
 * The Incompressibility Method (Li & Vitányi §6):
 *   A proof technique that uses Kolmogorov complexity to establish
 *   combinatorial properties by contradiction.
 *
 * Principle: If a combinatorial object violates a property, it has
 * a short description (via the violation), which contradicts its
 * Kolmogorov incompressibility.
 *
 * Reference: Li & Vitányi "An Introduction to Kolmogorov Complexity
 *   and Its Applications" (4th ed, 2019), Chapter 6.
 * Courses: MIT 6.841, Stanford CS254, Berkeley CS278, CMU 15-855
 *           Oxford Advanced Complexity, Cambridge Part III
 *           ETH 263-4650, Princeton COS 522
 */

#ifndef INCOMPRESSIBILITY_H
#define INCOMPRESSIBILITY_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

/* ══════════════════════════════════════════════════════════════
   L1: Core Type Definitions
   ══════════════════════════════════════════════════════════════ */

/* An incompressible string: one whose Kolmogorov complexity
   is close to its length. Formally, x is c-incompressible if
   K(x) ≥ |x| - c. */
typedef struct {
    unsigned char* data;
    size_t         len;
    size_t         estimated_K;
    int            incomp_c;
    int            is_incompressible;
} IncompressibleString;

/* Proof step types */
typedef enum {
    PROOF_STEP_ASSUME       = 0,
    PROOF_STEP_ENCODE       = 1,
    PROOF_STEP_BOUND        = 2,
    PROOF_STEP_CONTRADICT   = 3,
    PROOF_STEP_CONCLUDE     = 4
} ProofStepType;

/* A single step in an incompressibility proof */
typedef struct {
    ProofStepType type;
    char          description[256];
    size_t        description_length;
    double        bound;
} ProofStep;

/* A complete incompressibility proof */
typedef struct {
    char*          theorem_name;
    char*          property;
    ProofStep*     steps;
    int            n_steps;
    int            capacity;
    size_t         constant_overhead;
    int            is_valid;
} IncompressibilityProof;

/* A counterexample candidate */
typedef struct {
    void*    object;
    size_t   object_size;
    char*    object_type;
    size_t   kolmogorov_bound;
    size_t   encoding_length;
    int      is_genuine_counterexample;
} CounterExample;

/* Kolmogorov complexity bound */
typedef struct {
    size_t   lower_bound;
    size_t   upper_bound;
    size_t   constant_term;
    int      is_tight;
} KolmogorovBound;

/* Proof strategies */
typedef enum {
    PROOF_PIGEONHOLE            = 0,
    PROOF_COUNTING              = 1,
    PROOF_ENCODING              = 2,
    PROOF_DIAGONALIZATION       = 3,
    PROOF_AVERAGE_CASE          = 4,
    PROOF_EXPECTATION           = 5,
    PROOF_COMPRESSION           = 6,
    PROOF_RECURSIVE             = 7
} ProofStrategy;

/* ══════════════════════════════════════════════════════════════
   L2: Core API — Incompressibility Method
   ══════════════════════════════════════════════════════════════ */

IncompressibleString* incstr_create(const unsigned char* data, size_t len, int c);
void incstr_free(IncompressibleString* s);
size_t incstr_estimate_K(const unsigned char* data, size_t len);
int incstr_is_c_incompressible(const unsigned char* data, size_t len, int c);
void incstr_print_report(const IncompressibleString* s);

IncompressibilityProof* incproof_create(const char* theorem_name,
                                         const char* property);
void incproof_add_step(IncompressibilityProof* proof, ProofStepType type,
                        const char* desc, size_t desc_length, double bound);
int incproof_validate(IncompressibilityProof* proof);
void incproof_print(const IncompressibilityProof* proof);
void incproof_free(IncompressibilityProof* proof);

CounterExample* cex_create(void* object, size_t size, const char* type);
size_t cex_encoding_length(CounterExample* cex, ProofStrategy strategy);
int cex_is_genuine(CounterExample* cex, int incompressibility_c);
void cex_free(CounterExample* cex);

KolmogorovBound* kb_create(size_t lower, size_t upper, size_t constant);
int kb_is_contradiction(const KolmogorovBound* b);
int64_t kb_gap(const KolmogorovBound* b);
void kb_free(KolmogorovBound* b);

/* ══════════════════════════════════════════════════════════════
   L3: Mathematical Structures
   ══════════════════════════════════════════════════════════════ */

int inc_pigeonhole_proof(int n_items, int n_boxes);
int inc_counting_bound(size_t universe_size, size_t property_count,
                        int incompressibility_c);

size_t inc_encode_with_property(const unsigned char* x, size_t len,
                                 int property_holds, unsigned char** encoded_out);
int inc_decode_from_property(const unsigned char* encoded, size_t enc_len,
                              unsigned char** decoded_out, size_t* dec_len);

size_t inc_self_delimiting_encode(const unsigned char* x, size_t len,
                                   unsigned char** out);
size_t inc_self_delimiting_decode(const unsigned char* enc, size_t enc_len,
                                   unsigned char** out);
size_t inc_encode_pair(const unsigned char* x, size_t xlen,
                        const unsigned char* y, size_t ylen,
                        unsigned char** out);
int inc_decode_pair(const unsigned char* enc, size_t enc_len,
                     unsigned char** x_out, size_t* x_len,
                     unsigned char** y_out, size_t* y_len);

/* ══════════════════════════════════════════════════════════════
   L5: Proof Generation Engine
   ══════════════════════════════════════════════════════════════ */

IncompressibilityProof* inc_generate_pigeonhole_proof(int n);
IncompressibilityProof* inc_generate_sorting_lowerbound_proof(int n);
IncompressibilityProof* inc_generate_heapsort_optimality_proof(int n);
IncompressibilityProof* inc_generate_graph_noniso_proof(int n_vertices);

#endif /* INCOMPRESSIBILITY_H */
