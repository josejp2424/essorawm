# Contributing to EssoraWM

Bug reports, tested fixes and focused feature contributions are welcome in the
EssoraWM GitHub repository.

Before opening an issue, include:

- EssoraWM version or commit hash.
- Distribution and architecture.
- Whether the session runs as root, as in Puppy Linux, or as a regular user.
- Relevant `~/.jwmrc`, `/etc/system.jwmrc` or desktop configuration details.
- Exact steps to reproduce the problem.
- Terminal output or logs when available.

Please keep commits logically separated and make sure the source builds at
each commit. Follow the existing JWM C coding style and preserve compatibility
with the original XML configuration format whenever possible.

Essora-specific changes should be documented and may be marked in source with:

```c
/* agregado por josejp2424 */
```

EssoraWM is based on the upstream JWM project. Bugs that also occur unchanged
in upstream JWM may belong at the original project, while Essora desktop,
Pymenu, wallpaper, native desktop and packaging issues belong in this fork.
