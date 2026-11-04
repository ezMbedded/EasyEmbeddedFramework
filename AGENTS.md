# Read docs_llm/LLM_GUIDE.md before writing any code that uses this framework.

Hard rules (full details in the guide):

1. No dynamic allocation (`malloc`/`free`/`new`).
2. Call from an ISR only functions documented `ISR-safe: yes`; use the `FromISR` variant.
3. Never block in an ISR or while holding a lock.
4. Check every return value; never cast it to `(void)`.
5. Use only the public API in `inc/<layer>/<module>/`, never container internals.
6. Do not invent APIs. If it is not in the headers or `docs_llm/api/`, it does not exist.
