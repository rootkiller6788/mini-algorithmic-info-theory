/*
 * test_main.c - Comprehensive test suite for mini-minimum-description-length
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <float.h>

#include "../include/mdl_core.h"
#include "../include/nml.h"
#include "../include/two_part.h"
#include "../include/mdl_advanced.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { tests_run++; printf("  TEST: %s ... ", name); } while(0)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); } while(0)
#define CHECK(cond, msg) do { if (cond) PASS(); else FAIL(msg); } while(0)

int main(void) {
    printf("========================================\n");
    printf("mini-minimum-description-length Test Suite\n");
    printf("========================================\n\n");

    /* L1: Data Management */
    printf("--- L1: MDLData API ---\n");
    TEST("create"); {
        MDLData* d = mdl_data_create(10);
        CHECK(d != NULL && d->n == 0 && d->cap >= 10, "creation failed");
        mdl_data_free(d);
    }
    TEST("append_and_get"); {
        MDLData* d = mdl_data_create(5);
        mdl_data_append(d, 1.5); mdl_data_append(d, 2.5);
        CHECK(mdl_data_get(d,0)==1.5 && mdl_data_get(d,1)==2.5, "append/get mismatch");
        mdl_data_free(d);
    }
    TEST("from_array"); {
        double arr[] = {3,1,4,1,5};
        MDLData* d = mdl_data_from_array(arr, 5);
        CHECK(d->n == 5, "size mismatch");
        mdl_data_free(d);
    }
    TEST("mean_variance"); {
        double arr[] = {1,2,3,4,5};
        MDLData* d = mdl_data_from_array(arr, 5);
        double mu = mdl_data_mean(d);
        double var = mdl_data_variance(d, mu);
        CHECK(fabs(mu-3.0)<0.001 && fabs(var-2.0)<0.001, "mean/var wrong");
        mdl_data_free(d);
    }
    TEST("min_max"); {
        double arr[] = {5,1,9,-2,7};
        MDLData* d = mdl_data_from_array(arr, 5);
        CHECK(mdl_data_min(d)==-2.0 && mdl_data_max(d)==9.0, "min/max wrong");
        mdl_data_free(d);
    }
    TEST("sort"); {
        double arr[] = {5,3,1,4,2};
        MDLData* d = mdl_data_from_array(arr, 5);
        mdl_data_sort(d);
        int ok = 1;
        for (size_t i=1; i<d->n; i++) if (d->values[i]<d->values[i-1]) ok=0;
        CHECK(ok, "sort failed");
        mdl_data_free(d);
    }
    TEST("clone"); {
        double arr[] = {1,2,3};
        MDLData* d = mdl_data_from_array(arr, 3);
        MDLData* c = mdl_data_clone(d);
        CHECK(c->n==3 && c->values[0]==1.0 && c->values[2]==3.0, "clone mismatch");
        mdl_data_free(d); mdl_data_free(c);
    }

    printf("\n--- L1/L3: Universal Integer Codes ---\n");
    TEST("elias_gamma"); {
        CHECK(mdl_elias_gamma_code(1)>=1.0 && mdl_elias_gamma_code(100)>1.0, "gamma wrong");
    }
    TEST("elias_delta"); {
        CHECK(mdl_elias_delta_code(100)>1.0, "delta wrong");
    }
    TEST("rissanen_logstar"); {
        double c = mdl_rissanen_logstar_code(1000);
        CHECK(c>1.0 && c<20.0, "logstar out of range");
    }

    printf("\n--- L2: Two-Part Codes ---\n");
    double ga[] = {0.5,-0.3,1.2,0.1,-0.8,0.7,0.3,-0.1,1.0,-0.5,0.2,0.9,-0.4,0.6,-0.2};
    MDLData* gauss = mdl_data_from_array(ga, 15);
    TEST("gaussian_two_part"); {
        double c = mdl_two_part_gaussian(gauss);
        CHECK(c>0.0 && c<1000.0, "gauss cost nonsense");
    }
    TEST("bernoulli_two_part"); {
        double bin[] = {1,0,1,1,0,1,0,1,1,0};
        MDLData* b = mdl_data_from_array(bin, 10);
        CHECK(mdl_two_part_bernoulli(b)>0.0, "bern cost invalid");
        mdl_data_free(b);
    }
    TEST("poisson_two_part"); {
        double p[] = {2,3,1,4,2,0,3,2,5,1};
        MDLData* pd = mdl_data_from_array(p, 10);
        CHECK(mdl_two_part_poisson(pd)>0.0, "pois cost invalid");
        mdl_data_free(pd);
    }
    TEST("exponential_two_part"); {
        double e[] = {1,2,0.5,1.5,3};
        MDLData* ed = mdl_data_from_array(e, 5);
        CHECK(mdl_two_part_exponential(ed)>0.0, "exp cost invalid");
        mdl_data_free(ed);
    }

    printf("\n--- L3/L5: NML Codes ---\n");
    TEST("nml_param_complexity"); {
        double pc = nml_parametric_complexity_approx(100, 2);
        CHECK(pc>0.0 && pc<10.0, "param complexity out of range");
    }
    TEST("nml_gaussian_full"); {
        CHECK(nml_gaussian_full(gauss)>0.0, "NML gauss invalid");
    }
    TEST("nml_bernoulli"); {
        double b[] = {1,0,1,1,0,1,0,1,1,0,1,1,0,1,0,1,0,0,1,1};
        MDLData* bd = mdl_data_from_array(b, 20);
        CHECK(nml_bernoulli_full(bd)>0.0, "NML bern invalid");
        mdl_data_free(bd);
    }
    TEST("nml_fisher_gaussian"); {
        CHECK(fabs(nml_fisher_information_gaussian(1.0)-2.0)<0.001, "Fisher Gauss wrong");
    }
    TEST("nml_fisher_bernoulli"); {
        CHECK(fabs(nml_fisher_information_bernoulli(0.5)-4.0)<0.001, "Fisher Bern wrong");
    }
    TEST("nml_fisher_poisson"); {
        CHECK(fabs(nml_fisher_information_poisson(2.0)-0.5)<0.001, "Fisher Pois wrong");
    }

    printf("\n--- L4: Redundancy and Regret ---\n");
    TEST("redundancy_bound"); {
        double r = mdl_redundancy_bound(100, 2);
        CHECK(r>0.0 && r<20.0, "redundancy out of range");
    }
    TEST("worst_case_regret"); {
        CHECK(mdl_worst_case_regret(100,2)>0.0, "regret non-positive");
    }
    TEST("nml_redundancy_rate"); {
        double r = nml_redundancy_rate(gauss, 2);
        CHECK(r>=0.0 && r<10.0, "rate invalid");
    }

    printf("\n--- L6: Polynomial MDL ---\n");
    double xa[] = {1,2,3,4,5,6,7,8,9,10};
    double ya[] = {2.1,4.0,6.2,8.1,9.9,12.1,14.0,16.2,18.0,19.9};
    MDLData* x = mdl_data_from_array(xa, 10);
    MDLData* y = mdl_data_from_array(ya, 10);
    TEST("poly_fit_linear"); {
        MDLPolynomial* p = mdl_poly_fit(x, y, 1);
        CHECK(p!=NULL && p->degree==1 && p->mse<1.0, "linear fit failed");
        mdl_poly_free(p);
    }
    TEST("poly_optimal_degree"); {
        int d = mdl_poly_optimal_degree(x, y, 3);
        CHECK(d>=0 && d<=3, "opt degree out of range");
    }
    TEST("poly_evaluate"); {
        MDLPolynomial* p = mdl_poly_fit(x, y, 1);
        if (p) {
            double yp = mdl_poly_evaluate(p, 5.0);
            CHECK(fabs(yp-(p->coeffs[0]+p->coeffs[1]*5.0))<0.001, "eval wrong");
            mdl_poly_free(p);
        }
    }

    printf("\n--- L6: Histogram MDL ---\n");
    TEST("histogram_create"); {
        MDLHistogram* h = mdl_histogram_create(gauss, 5);
        CHECK(h!=NULL && h->n_bins==5, "hist create failed");
        mdl_histogram_free(h);
    }
    TEST("histogram_optimal"); {
        int b = mdl_histogram_optimal_bins(gauss, 8);
        CHECK(b>=1 && b<=8, "opt bins out of range");
    }

    printf("\n--- L6: Model Selection ---\n");
    TEST("model_select"); {
        MDLResult* r = mdl_select_create(3);
        mdl_select_set_cost(r,0,100); mdl_select_set_cost(r,1,50); mdl_select_set_cost(r,2,75);
        CHECK(mdl_select_best(r)==1, "select wrong");
        mdl_select_free(r);
    }

    printf("\n--- L7: Change Point Detection ---\n");
    TEST("changepoint"); {
        double cp[] = {0.1,0.2,0.1,0.3,0.2,5.1,5.2,4.9,5.0,5.3,0.0,-0.1,0.2,-0.1,0.1};
        MDLData* cd = mdl_data_from_array(cp, 15);
        MDLChangePoint* cpt = mdl_changepoint_detect(cd, 3);
        CHECK(cpt!=NULL && cpt->n_changes>=0, "cp detect failed");
        mdl_changepoint_free(cpt); mdl_data_free(cd);
    }

    printf("\n--- L7: AR Model ---\n");
    TEST("ar_fit"); {
        double ts[] = {0.1,0.3,0.2,0.5,0.4,0.7,0.6,0.9,0.8,1.1,1.0,1.3,1.2,1.5,1.4};
        MDLData* td = mdl_data_from_array(ts, 15);
        MDLARModel* ar = mdl_ar_fit(td, 2);
        CHECK(ar!=NULL && ar->ar_order==2, "AR fit failed");
        mdl_ar_free(ar); mdl_data_free(td);
    }
    TEST("ar_optimal"); {
        double ts[] = {0.1,0.3,0.2,0.5,0.4,0.7,0.6,0.9,0.8,1.1};
        MDLData* td = mdl_data_from_array(ts, 10);
        int o = mdl_ar_optimal_order(td, 3);
        CHECK(o>=0 && o<=3, "AR order out of range");
        mdl_data_free(td);
    }

    printf("\n--- L7: Clustering ---\n");
    TEST("cluster_kmeans"); {
        double ca[] = {0.1,0.2,0.3,0.4,5.1,5.2,5.3,5.4,10.1,10.2,10.3,10.4};
        MDLData* cd = mdl_data_from_array(ca, 12);
        MDLCluster* cl = mdl_cluster_kmeans_mdl(cd, 4, 3);
        CHECK(cl!=NULL && cl->k>=1, "cluster failed");
        mdl_cluster_free(cl); mdl_data_free(cd);
    }

    printf("\n--- L7: Information Criteria ---\n");
    TEST("criteria"); {
        ModelCriteria* c = mdl_criteria_compute(gauss, 2, 15.0);
        CHECK(c!=NULL && c->aic>0.0 && c->bic>0.0, "criteria failed");
        mdl_criteria_free(c);
    }

    printf("\n--- L7: Prequential MDL ---\n");
    TEST("prequential_gauss"); {
        PrequentialResult* pr = mdl_prequential_gaussian(gauss, 2);
        CHECK(pr!=NULL && pr->cumulative_log_loss>0.0, "prequential gauss failed");
        mdl_prequential_free(pr);
    }
    TEST("prequential_bern"); {
        double b[] = {1,0,1,1,0,1,0,1,1,0};
        MDLData* bd = mdl_data_from_array(b, 10);
        PrequentialResult* pr = mdl_prequential_bernoulli(bd);
        CHECK(pr!=NULL && pr->cumulative_log_loss>0.0, "prequential bern failed");
        mdl_prequential_free(pr); mdl_data_free(bd);
    }

    printf("\n--- L8: MML Connection ---\n");
    TEST("mml87"); {
        CHECK(mdl_mml87_estimate(gauss,2)>0.0, "MML87 invalid");
    }
    TEST("mdl_mml_div"); {
        CHECK(isfinite(mdl_mdl_mml_divergence(gauss,2)), "MDL-MML div not finite");
    }

    printf("\n--- L8: Mixture MDL ---\n");
    TEST("mixture_mdl"); {
        double nlls[] = {10,8,7.5};
        int ks[] = {1,2,3};
        CHECK(mdl_mixture_mdl(nlls,ks,3,100)>0.0, "mixture MDL invalid");
    }

    printf("\n--- L8: Decision Tree MDL ---\n");
    TEST("tree_cost"); {
        CHECK(mdl_decision_tree_cost(1000,10,50,5)>0.0, "tree cost invalid");
    }

    printf("\n--- L7: Structural Breaks ---\n");
    TEST("detect_breaks"); {
        double sb[] = {0.1,0.2,0.15,5.1,5.2,5.0,10.1,10.2,10.0};
        MDLData* sbd = mdl_data_from_array(sb, 9);
        MDLStructuralBreak* br = mdl_detect_breaks(sbd, 2.0, 3);
        CHECK(br!=NULL, "break detection failed");
        mdl_breaks_free(br); mdl_data_free(sbd);
    }

    printf("\n--- L7: Segmentation MDL ---\n");
    TEST("seg_cost"); {
        int sizes[] = {100,200,150,50};
        CHECK(mdl_segmentation_cost(500,4,sizes,256)>0.0, "seg cost invalid");
    }

    printf("\n--- Two-Part Code Functions ---\n");
    TEST("twopart_encode_real"); {
        CHECK(twopart_encode_real(5,0,10,100)>0.0, "encode real failed");
    }
    TEST("twopart_linear"); {
        double c = twopart_code_linear_regression(x,y);
        CHECK(isfinite(c), "two-part linreg non-finite");
    }
    TEST("twopart_poly"); {
        double c = twopart_code_polynomial(x,y,1);
        CHECK(isfinite(c), "two-part poly non-finite");
    }
    TEST("twopart_select"); {
        double costs[] = {100,50,75};
        CHECK(twopart_select_model(costs,3)==1, "twopart select wrong");
    }

    printf("\n--- NML Model Selection ---\n");
    TEST("nml_select"); {
        double costs[] = {120,80,95};
        CHECK(nml_select(costs,3)==1, "NML select wrong");
    }
    TEST("nml_exponential_nml"); {
        double e[] = {1,2,0.5,1.5,3};
        MDLData* ed = mdl_data_from_array(e,5);
        CHECK(nml_exponential_full(ed)>0.0, "NML exp invalid");
        mdl_data_free(ed);
    }

    printf("\n--- KL Divergence Utilities ---\n");
    TEST("kl_gauss"); {
        CHECK(mdl_kl_divergence_gaussian(0,1,1,1)>=0.0, "KL gauss negative");
    }
    TEST("kl_bern"); {
        CHECK(mdl_kl_divergence_bernoulli(0.5,0.7)>=0.0, "KL bern negative");
    }
    TEST("kl_pois"); {
        CHECK(mdl_kl_divergence_poisson(2,3)>=0.0, "KL pois negative");
    }

    printf("\n--- L8: Power Analysis ---\n");
    TEST("nml_power"); {
        double p = nml_model_selection_power(500,2,1,0.05);
        CHECK(p>=0.0 && p<=1.0, "power out of [0,1]");
    }

    mdl_data_free(gauss); mdl_data_free(x); mdl_data_free(y);
    printf("\n========================================\n");
    printf("Results: %d/%d tests passed\n", tests_passed, tests_run);
    printf("========================================\n");
    return (tests_passed == tests_run) ? 0 : 1;
}
