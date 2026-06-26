# Mini Algorithmic Information Theory

A collection of **from-scratch, zero-dependency C implementations** of university-level algorithmic information theory (AIT). Each module maps to MIT (and other top-tier university) courses, bridging Kolmogorov complexity, Solomonoff induction, Chaitin's Omega, and the incompressibility method into runnable C code.

## Sub-Modules

| Sub-Module | Topics | Key Courses |
|-----------|--------|-------------|
| [mini-algorithmic-randomness-ml](mini-algorithmic-randomness-ml/) | ML-based algorithmic randomness: Martin-Löf tests, GAN randomness detection, neural compression based K(x) estimation | MIT 6.867, Stanford CS236, Berkeley CS294-158 |
| [mini-chaitin-omega-constant](mini-chaitin-omega-constant/) | Chaitin's Ω constant: halting probability, prefix-free UTM, Solovay reducibility, left-c.e. reals, Calude's theorem | MIT 6.841, Stanford CS254, Berkeley CS278 |
| [mini-incompressibility-method](mini-incompressibility-method/) | Incompressibility method: Li & Vitányi §6 proof technique, sorting lower bounds, combinatorial proofs via K(x) | MIT 6.841, Oxford Advanced Complexity, Cambridge Part III |
| [mini-kolmogorov-complexity-k](mini-kolmogorov-complexity-k/) | Kolmogorov complexity K(x): plain/prefix definitions, LZ/Huffman/BWT compressors, Shannon entropy, NCD applications | MIT 6.841, Stanford CS254, CMU 15-855 |
| [mini-minimum-description-length](mini-minimum-description-length/) | Minimum Description Length: two-part codes, NML, refined MDL, model selection vs AIC/BIC | MIT 6.441, Stanford EE376A, Helsinki CS |
| [mini-prefix-complexity-machine](mini-prefix-complexity-machine/) | Prefix-free complexity: prefix TMs, universal distribution m(x), Coding Theorem, monotone complexity Km(x) | MIT 6.841, Stanford CS254, MIT 6.441 |
| [mini-resource-bounded-kolmogorov](mini-resource-bounded-kolmogorov/) | Resource-bounded complexity K^t(x)/K^s(x): time/space bounds, Levin universal search, NCD approximations | MIT 6.841, Stanford CS254, CMU 15-855 |
| [mini-solomonoff-universal-prediction](mini-solomonoff-universal-prediction/) | Solomonoff universal induction: algorithmic probability M(x), dovetailing enumeration, prediction convergence | MIT 6.841/18.400, CMU 15-751, Cambridge Part III |

## Design Philosophy

- **Zero external dependencies** — pure C (C99/C11), only `libc` and `libm`
- **Self-contained modules** — each directory has its own `Makefile`, `include/`, `src/`, `examples/`, `demos/`, `tests/`
- **Theory-to-code mapping** — every module includes `docs/` with course-alignment notes and textbook references (Li & Vitányi, Cover & Thomas, Nies, Downey & Hirschfeldt)
- **Practical demos** — program enumeration, compression benchmarks, Martin-Löf test simulations, Chaitin's Ω approximation, universal search, and more

## Building

Each module is standalone. Navigate to a module directory and run:

```bash
cd mini-algorithmic-randomness-ml
make all    # build everything
make test   # run tests
```

Requires **GCC** and **GNU Make**.

## Project Structure

```
mini-algorithmic-info-theory/
├── mini-algorithmic-randomness-ml/     # ML-based Martin-Löf tests, GAN randomness detection, K(x) estimation via neural nets
├── mini-chaitin-omega-constant/        # Chaitin's Ω: halting probability, prefix-free UTM, Solovay reducibility, left-c.e. reals
├── mini-incompressibility-method/      # Incompressibility proofs: combinatorial lower bounds, Li & Vitányi §6 method
├── mini-kolmogorov-complexity-k/       # Core K(x): plain & prefix complexity, LZ/Huffman/BWT compressors, entropy measures
├── mini-minimum-description-length/    # MDL principle: two-part codes, NML universal codes, refined MDL model selection
├── mini-prefix-complexity-machine/     # Prefix TMs: prefix codes & Kraft inequality, universal distribution m(x), Km(x)
├── mini-resource-bounded-kolmogorov/   # K^t(x) & K^s(x): time/space bounded complexity, Levin universal search, NCD
└── mini-solomonoff-universal-prediction/ # Solomonoff induction: algorithmic probability M(x), dovetailing, prediction
```

## License

MIT
