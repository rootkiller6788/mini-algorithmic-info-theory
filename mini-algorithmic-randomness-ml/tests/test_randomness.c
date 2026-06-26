/*
 * test_randomness.c — Comprehensive tests for algorithmic-randomness-ml
 *
 * Tests: bit string operations, Martin-Löf tests, statistical tests,
 * NIST battery, randomness profiling, neural network operations,
 * ML classifier, autoencoder, Kolmogorov estimator, neural compression,
 * GAN detection, and PRNG detection.
 *
 * All tests use assert() — zero output means all passed.
 */
#include "../include/randomness.h"
#include "../include/randomness_ml.h"
#include "../include/compression_nn.h"
#include "../include/gan_randomness.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <string.h>

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { \
    tests_run++; \
    printf("  TEST %2d: %-50s ", tests_run, name); \
    fflush(stdout); \
} while(0)

#define PASS() do { \
    tests_passed++; \
    printf("PASS\n"); \
} while(0)

#define FAIL(msg) do { \
    printf("FAIL: %s\n", msg); \
} while(0)

#define ASSERT(cond, msg) do { \
    if (!(cond)) { FAIL(msg); return; } \
} while(0)

/* ── Bit String Operations ─────────────────────────────────── */
static void test_bitstring_ops(void) {
    TEST("rbs_create and append");
    RandomBitString* s = rbs_create(64);
    ASSERT(s != NULL, "create failed");
    ASSERT(s->len == 0, "initial len should be 0");
    rbs_append_bit(s, 1); rbs_append_bit(s, 0);
    rbs_append_bit(s, 1); rbs_append_bit(s, 0);
    ASSERT(s->len == 4, "len should be 4");
    ASSERT(rbs_get_bit(s, 0) == 1, "bit 0 should be 1");
    ASSERT(rbs_get_bit(s, 1) == 0, "bit 1 should be 0");
    rbs_free(s);
    PASS();

    TEST("rbs_from_binary");
    RandomBitString* b = rbs_from_binary("01010101");
    ASSERT(b != NULL, "from_binary failed");
    ASSERT(b->len == 8, "len should be 8");
    ASSERT(rbs_count_ones(b) == 4, "should have 4 ones");
    rbs_free(b);
    PASS();

    TEST("rbs_from_hex");
    RandomBitString* h = rbs_from_hex("FF00");
    ASSERT(h != NULL, "from_hex failed");
    ASSERT(h->len == 16, "FF00 should be 16 bits");
    rbs_free(h);
    PASS();

    TEST("rbs_random");
    unsigned int seed = 42;
    RandomBitString* r = rbs_random(1024, &seed);
    ASSERT(r != NULL, "random failed");
    ASSERT(r->len == 1024, "should be 1024 bits");
    double d = rbs_density(r);
    ASSERT(d >= 0.3 && d <= 0.7, "density should be near 0.5");
    rbs_free(r);
    PASS();

    TEST("rbs_clone and equals");
    RandomBitString* orig = rbs_from_binary("11001100");
    RandomBitString* copy = rbs_clone(orig);
    ASSERT(rbs_equals(orig, copy), "clone should equal original");
    rbs_flip_bit(copy, 0);
    ASSERT(!rbs_equals(orig, copy), "flipped should differ");
    rbs_free(orig); rbs_free(copy);
    PASS();
}

/* ── Statistical Tests ─────────────────────────────────────── */
static void test_statistical_tests(void) {
    unsigned int seed = 123;
    RandomBitString* rnd = rbs_random(2048, &seed);

    TEST("frequency_test on random");
    double fp = frequency_test(rnd);
    ASSERT(fp >= 0.0 && fp <= 1.0, "p-value out of range");
    ASSERT(fp >= 0.01, "random should pass frequency test");
    PASS();

    TEST("runs_test on random");
    double rp = runs_test(rnd);
    ASSERT(rp >= 0.0 && rp <= 1.0, "p-value out of range");
    PASS();

    TEST("longest_run_test on random");
    double lp = longest_run_test(rnd);
    ASSERT(lp >= 0.0 && lp <= 1.0, "p-value out of range");
    PASS();

    TEST("binary_matrix_rank_test on random");
    double mp = binary_matrix_rank_test(rnd, 8);
    ASSERT(mp >= 0.0 && mp <= 1.0, "p-value out of range");
    PASS();

    TEST("cumulative_sums_test on random");
    double cp = cumulative_sums_test(rnd);
    ASSERT(cp >= 0.0 && cp <= 1.0, "p-value out of range");
    PASS();

    TEST("approximate_entropy_test on random");
    double ap = approximate_entropy_test(rnd, 5);
    ASSERT(ap >= 0.0 && ap <= 1.0, "p-value out of range");
    PASS();

    TEST("serial_test on random");
    double sp = serial_test(rnd, 5);
    ASSERT(sp >= 0.0 && sp <= 1.0, "p-value out of range");
    PASS();

    TEST("spectral_test on random");
    double dp = spectral_test(rnd);
    ASSERT(dp >= 0.0 && dp <= 1.0, "p-value out of range");
    PASS();

    TEST("autocorrelation_test on random");
    double ac = autocorrelation_test(rnd, 1);
    ASSERT(ac >= 0.0 && ac <= 1.0, "p-value out of range");
    PASS();

    TEST("poker_test on random");
    double pp = poker_test(rnd, 4);
    ASSERT(pp >= 0.0 && pp <= 1.0, "p-value out of range");
    PASS();

    rbs_free(rnd);
}

/* ── Martin-Löf Test ────────────────────────────────────────── */
static void test_martin_lof(void) {
    TEST("ml_test_create and add");
    MLTest* t = ml_test_create("Test ML");
    ASSERT(t != NULL, "create failed");
    ml_test_add_component(t, "Freq", 0.01, 0.005);
    ml_test_add_component(t, "Runs", 0.01, 0.003);
    ASSERT(t->n_tests == 2, "should have 2 tests");
    ASSERT(t->tests[0].passed == 1, "stat 0.005 <= 0.01 should pass");
    PASS();

    TEST("ml_test_evaluate");
    int result = ml_test_evaluate(t, 1);
    ASSERT(result == 1, "all tests should pass");
    PASS();

    TEST("ml_test_is_universal");
    ml_test_add_component(t, "Frequency", 0.01, 0.001);
    ml_test_add_component(t, "RunsSecond", 0.01, 0.001);
    ml_test_add_component(t, "EntropyTest", 0.01, 0.001);
    ml_test_add_component(t, "SerialCheck", 0.01, 0.001);
    ml_test_add_component(t, "SpectralCheck", 0.01, 0.001);
    ASSERT(ml_test_is_universal(t) == 1, "should be universal");
    PASS();

    ml_test_free(t);

    TEST("sequential_test_union");
    MLTest* t1 = ml_test_create("A");
    MLTest* t2 = ml_test_create("B");
    ml_test_add_component(t1, "a1", 0.01, 0.0);
    ml_test_add_component(t2, "b1", 0.01, 0.0);
    MLTest* tests_arr[] = {t1, t2};
    MLTest* union_t = sequential_test_union(tests_arr, 2);
    ASSERT(union_t != NULL, "union failed");
    ASSERT(union_t->n_tests == 2, "union should have 2 tests");
    ml_test_free(t1); ml_test_free(t2); ml_test_free(union_t);
    PASS();
}

/* ── Martingale ─────────────────────────────────────────────── */
static void test_martingale(void) {
    TEST("martingale_create and update");
    RandomMartingale* m = martingale_create(10.0);
    ASSERT(m != NULL, "create failed");
    ASSERT(m->initial_capital == 10.0, "initial capital should be 10");
    RandomBitString* prefix = rbs_from_binary("0101010101");
    martingale_update(m, prefix);
    ASSERT(m->n_prefixes > 0, "should have processed prefix");
    rbs_free(prefix);
    martingale_free(m);
    PASS();
}

/* ── Randomness Profile ─────────────────────────────────────── */
static void test_randomness_profile(void) {
    unsigned int seed = 456;
    RandomBitString* rnd = rbs_random(1024, &seed);

    TEST("randomness_profile_create");
    RandomnessProfile* p = randomness_profile_create(rnd);
    ASSERT(p != NULL, "profile create failed");
    ASSERT(p->n_tests_total > 0, "should have tests");
    ASSERT(p->entropy_per_bit >= 0.0, "entropy should be non-negative");
    PASS();

    TEST("randomness_profile_classify");
    RandomnessType t = randomness_profile_classify(p);
    ASSERT(t >= RAND_ML_RANDOM && t <= RAND_NONRANDOM, "invalid type");
    PASS();

    randomness_profile_free(p);

    TEST("nist_test_battery_run");
    NISTTestBattery* nist = nist_test_battery_run(rnd);
    ASSERT(nist != NULL, "NIST battery failed");
    ASSERT(nist->n_total == 15, "should have 15 tests");
    nist_test_battery_free(nist);
    PASS();

    rbs_free(rnd);
}

/* ── Neural Network ─────────────────────────────────────────── */
static void test_neural_network(void) {
    TEST("nn_create and forward");
    int layers[] = {4, 8, 1};
    NeuralNet* nn = nn_create(layers, 3, 0.01);
    ASSERT(nn != NULL, "create failed");
    double input[] = {0.5, 0.5, 0.5, 0.5};
    double output[1];
    nn_forward(nn, input, output);
    ASSERT(output[0] >= 0.0 && output[0] <= 1.0, "output should be in [0,1]");
    nn_free(nn);
    PASS();

    TEST("nn_train simple XOR");
    int xor_layers[] = {2, 4, 1};
    NeuralNet* xor_net = nn_create(xor_layers, 3, 0.1);
    double* X[4], *Y[4];
    for (int i = 0; i < 4; i++) {
        X[i] = (double*)malloc(2 * sizeof(double));
        Y[i] = (double*)malloc(sizeof(double));
        X[i][0] = (i & 1) ? 1.0 : 0.0;
        X[i][1] = (i & 2) ? 1.0 : 0.0;
        Y[i][0] = (X[i][0] != X[i][1]) ? 1.0 : 0.0;
    }
    nn_train(xor_net, X, Y, 4, 500);
    double pred = nn_predict_single(xor_net, (double[]){0.0, 1.0});
    ASSERT(pred > 0.5, "XOR(0,1)=1: prediction should be > 0.5");
    for (int i = 0; i < 4; i++) { free(X[i]); free(Y[i]); }
    nn_free(xor_net);
    PASS();
}

/* ── ML Classifier ──────────────────────────────────────────── */
static void test_ml_classifier(void) {
    unsigned int seed = 789;
    int n = 6;
    RandomBitString** samples = (RandomBitString**)malloc((size_t)n * sizeof(RandomBitString*));
    int* labels = (int*)malloc((size_t)n * sizeof(int));
    for (int i = 0; i < 3; i++) {
        samples[i] = rbs_random(128, &seed);
        labels[i] = 1; /* random */
    }
    for (int i = 3; i < 6; i++) {
        samples[i] = rbs_from_binary("0101010101010101");
        labels[i] = 0; /* non-random */
    }

    TEST("ml_randomness_classifier_train");
    MLRandomnessClassifier* clf = ml_randomness_classifier_train(
        samples, labels, n, 8, 5);
    ASSERT(clf != NULL, "classifier train failed");
    ASSERT(clf->is_trained == 1, "should be trained");
    ASSERT(clf->accuracy >= 0.0 && clf->accuracy <= 1.0, "accuracy out of range");
    PASS();

    TEST("ml_randomness_classifier_predict (small)");
    /* Use a sample from training set for quick prediction */
    double score = ml_randomness_classifier_predict(clf, samples[0]);
    ASSERT(score >= 0.0 && score <= 1.0, "score out of range");
    PASS();

    for (int i = 0; i < n; i++) rbs_free(samples[i]);
    free(samples); free(labels);
    ml_randomness_classifier_free(clf);
}

/* ── Feature Extraction ─────────────────────────────────────── */
static void test_features(void) {
    unsigned int seed = 111;
    RandomBitString* rnd = rbs_random(512, &seed);

    TEST("random_features_extract");
    RandomFeatures* rf = random_features_extract(rnd);
    ASSERT(rf != NULL, "feature extraction failed");
    ASSERT(rf->n_features == 24, "should have 24 features");
    for (int i = 0; i < rf->n_features; i++)
        ASSERT(isfinite(rf->features[i]), "feature should be finite");
    random_features_free(rf);
    PASS();

    TEST("random_features_to_vector");
    RandomFeatures* rf2 = random_features_extract(rnd);
    int len;
    double* vec = random_features_to_vector(rf2, &len);
    ASSERT(vec != NULL, "to_vector failed");
    ASSERT(len == 24, "vector length should be 24");
    free(vec);
    random_features_free(rf2);
    PASS();

    rbs_free(rnd);
}

/* ── Autoencoder ─────────────────────────────────────────────── */
static void test_autoencoder(void) {
    unsigned int seed = 222;
    int n = 5;
    RandomBitString** normal = (RandomBitString**)malloc((size_t)n * sizeof(RandomBitString*));
    for (int i = 0; i < n; i++)
        normal[i] = rbs_random(1024, &seed);

    TEST("autoencoder_train");
    AutoencoderAnomalyDetector* ae = autoencoder_train(normal, n, 12, 4, 8);
    ASSERT(ae != NULL, "autoencoder train failed");
    ASSERT(ae->latent_dim == 4, "latent dim should be 4");
    PASS();

    TEST("autoencoder_anomaly_score");
    double score = autoencoder_anomaly_score(ae, normal[0]);
    ASSERT(isfinite(score), "score should be finite");
    PASS();

    TEST("autoencoder_is_random");
    int is_rand = autoencoder_is_random(ae, normal[0]);
    ASSERT(is_rand == 0 || is_rand == 1, "should be 0 or 1");
    PASS();

    for (int i = 0; i < n; i++) rbs_free(normal[i]);
    free(normal);
    autoencoder_free(ae);
}

/* ── Neural Compression ─────────────────────────────────────── */
static void test_neural_compression(void) {
    unsigned char data[128];
    for (int i = 0; i < 128; i++) data[i] = (unsigned char)(i * 7 + 13);

    TEST("neural_compress");
    NeuralCompressionResult* r = neural_compress(data, 128, 8, 16);
    ASSERT(r != NULL, "neural compress failed");
    ASSERT(r->original_bits == 1024, "original should be 1024 bits");
    ASSERT(r->ratio > 0.0 && r->ratio <= 1.0, "ratio should be in (0,1]");
    neural_compression_result_free(r);
    PASS();

    TEST("neural_compress_bits");
    unsigned int seed = 333;
    RandomBitString* bits = rbs_random(512, &seed);
    NeuralCompressionResult* rb = neural_compress_bits(bits, 8, 16);
    ASSERT(rb != NULL, "bit compress failed");
    ASSERT(rb->original_bits == 512, "should be 512 bits");
    neural_compression_result_free(rb);
    rbs_free(bits);
    PASS();

    TEST("neural_complexity_upper_bound");
    RandomBitString* b2 = rbs_random(256, &seed);
    size_t bound = neural_complexity_upper_bound(b2, 8, 16);
    ASSERT(bound > 0, "bound should be positive");
    rbs_free(b2);
    PASS();

    TEST("prediction_compression_bound");
    double b = prediction_compression_bound(0.8, 0.05);
    ASSERT(b >= 0.8, "bound should be >= cross_entropy");
    PASS();

    TEST("compression_benchmark_ml");
    CompressionBenchmark* bench = compression_benchmark_ml(data, 128);
    ASSERT(bench != NULL, "benchmark failed");
    ASSERT(bench->shannon_ratio > 0.0, "shannon ratio should be positive");
    compression_benchmark_free(bench);
    PASS();
}

/* ── GAN Detection ──────────────────────────────────────────── */
static void test_gan_detection(void) {
    TEST("gan_create");
    GANTrained* gan = gan_create(32, 8, 16);
    ASSERT(gan != NULL, "gan create failed");
    ASSERT(gan->input_dim == 32, "input dim should be 32");
    gan_free(gan);
    PASS();

    unsigned int seed = 444;
    int n = 5;
    RandomBitString** real = (RandomBitString**)malloc((size_t)n * sizeof(RandomBitString*));
    for (int i = 0; i < n; i++)
        real[i] = rbs_random(32, &seed);

    TEST("gan_train_on_random");
    GANTrained* gan2 = gan_create(32, 8, 16);
    gan_train_on_random(gan2, real, n, 10, 0.01);
    ASSERT(gan2->trained_iters == 10, "should have trained 10 iters");
    gan_free(gan2);
    PASS();

    TEST("gan_generate");
    GANTrained* gan3 = gan_create(32, 8, 16);
    gan_train_on_random(gan3, real, n, 5, 0.01);
    RandomBitString* gen = gan_generate(gan3);
    ASSERT(gen != NULL, "generate failed");
    ASSERT(gen->len > 0, "generated should not be empty");
    rbs_free(gen);
    gan_free(gan3);
    PASS();

    TEST("gan_discriminate");
    GANTrained* gan4 = gan_create(32, 8, 16);
    gan_train_on_random(gan4, real, n, 5, 0.01);
    double d = gan_discriminate(gan4, real[0]);
    ASSERT(d >= 0.0 && d <= 1.0, "discriminate out of range");
    gan_free(gan4);
    PASS();

    for (int i = 0; i < n; i++) rbs_free(real[i]);
    free(real);
}

/* ── PRNG Detection ────────────────────────────────────────── */
static void test_prng_detection(void) {
    unsigned int seed = 555;
    RandomBitString* rnd = rbs_random(1024, &seed);

    TEST("detect_pseudorandom_ml");
    PRNGDetectionResult* pr = detect_pseudorandom_ml(rnd);
    ASSERT(pr != NULL, "PRNG detection failed");
    ASSERT(pr->confidence >= 0.0, "confidence should be >= 0");
    prng_detection_result_free(pr);
    PASS();

    rbs_free(rnd);
}

/* ── ML Randomness Suite ────────────────────────────────────── */
static void test_ml_suite(void) {
    unsigned int seed = 666;
    RandomBitString* rnd = rbs_random(1024, &seed);

    TEST("ml_randomness_suite");
    RandomnessProfile* p = ml_randomness_suite(rnd);
    ASSERT(p != NULL, "suite failed");
    ASSERT(p->kolmogorov_estimate >= 0.0 && p->kolmogorov_estimate <= 1.0,
           "K estimate out of range");
    randomness_profile_free(p);
    PASS();

    rbs_free(rnd);
}

/* ── Theorems ───────────────────────────────────────────────── */
static void test_theorems(void) {
    unsigned int seed = 777;
    RandomBitString* rnd = rbs_random(512, &seed);

    TEST("equivalence_theorem_check");
    double eq = equivalence_theorem_check(rnd);
    ASSERT(eq >= 0.0 && eq <= 1.0, "equivalence out of range");
    PASS();

    TEST("no_halting_detector_demonstration");
    int hd = no_halting_detector_demonstration(rnd, 100);
    ASSERT(hd == 0 || hd == 1, "should be 0 or 1");
    PASS();

    TEST("ml_universal_approximation_demo");
    int ua = ml_universal_approximation_demo(16, 0.5);
    ASSERT(ua == 0 || ua == 1, "should be 0 or 1");
    PASS();

    TEST("pac_randomness_learnability_bound");
    int pac = pac_randomness_learnability_bound(100);
    ASSERT(pac >= 0, "PAC bound should be non-negative");
    PASS();

    rbs_free(rnd);
}

/* ── Context Mixer ──────────────────────────────────────────── */
static void test_context_mixer(void) {
    TEST("cm_create and mix");
    ContextMixer* cm = cm_create(4, 0.01);
    ASSERT(cm != NULL, "create failed");
    double probs[16] = {0.25, 0.25, 0.25, 0.25, 0.3, 0.2, 0.3, 0.2,
                        0.1, 0.4, 0.1, 0.4, 0.2, 0.3, 0.2, 0.3};
    double mixed[4];
    cm_mix(cm, probs, mixed, 4);
    for (int i = 0; i < 4; i++)
        ASSERT(mixed[i] >= 0.0 && mixed[i] <= 1.0, "mixed prob out of range");
    cm_free(cm);
    PASS();
}

/* ── Neural Predictor ───────────────────────────────────────── */
static void test_neural_predictor(void) {
    TEST("nnp_create and train");
    NeuralPredictor* np = nnp_create(4, 8, 2);
    ASSERT(np != NULL, "create failed");
    unsigned char context[] = {0, 1, 0, 1};
    nnp_update(np, context, 1);
    double probs[2];
    nnp_predict(np, context, probs);
    ASSERT(probs[0] + probs[1] >= 0.9 && probs[0] + probs[1] <= 1.1,
           "probs should sum to ~1");
    nnp_free(np);
    PASS();
}

/* ── Kolmogorov Neural Estimator ────────────────────────────── */
static void test_kolmogorov_estimator(void) {
    unsigned int seed = 888;
    int n = 3;
    RandomBitString** samples = (RandomBitString**)malloc((size_t)n * sizeof(RandomBitString*));
    size_t* k_ests = (size_t*)malloc((size_t)n * sizeof(size_t));
    for (int i = 0; i < n; i++) {
        samples[i] = rbs_random(512, &seed);
        k_ests[i] = (size_t)((double)samples[i]->len * 0.9);
    }

    TEST("kolmogorov_neural_train and predict");
    KolmogorovNeuralEstimator* ke = kolmogorov_neural_train(
        samples, k_ests, n, 12, 8, 5);
    ASSERT(ke != NULL, "train failed");
    double pred = kolmogorov_neural_predict(ke, samples[0]);
    ASSERT(pred >= 0.0 && pred <= 1.0, "prediction out of range");
    kolmogorov_neural_free(ke);
    PASS();

    for (int i = 0; i < n; i++) rbs_free(samples[i]);
    free(samples); free(k_ests);
}

int main(void) {
    printf("=== Algorithmic Randomness ML — Comprehensive Tests ===\n\n");

    test_bitstring_ops();
    test_statistical_tests();
    test_martin_lof();
    test_martingale();
    test_randomness_profile();
    test_neural_network();
    test_features();
    test_ml_classifier();
    test_autoencoder();
    test_neural_compression();
    test_context_mixer();
    test_neural_predictor();
    test_kolmogorov_estimator();
    test_gan_detection();
    test_prng_detection();
    test_ml_suite();
    test_theorems();

    printf("\n=== Results: %d/%d tests passed ===\n", tests_passed, tests_run);

    if (tests_passed == tests_run) {
        printf("ALL TESTS PASSED\n");
        return 0;
    } else {
        printf("SOME TESTS FAILED\n");
        return 1;
    }
}
