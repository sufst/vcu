# VCU Specific Contributing Guidelines

## Conventional Commits

Commits (and issue/pull request titles) should follow the [conventional commits specification](https://www.conventionalcommits.org/en/v1.0.0/) with the format:

```text
<type>: <subject>
```

`<type>` **must** be one of:
| Type | Description |
|------------|---------------------------------------------------------------------|
| `feat` | New features |
| `fix` | Bug fixes |
| `refactor` | Refactors (rewriting/restructuring code without changing behaviour) |
| `perf` | Refactors specifically to improve performance |
| `style` | Code formatting, whitespace etc |
| `test` | New tests or changes to existing tests |
| `docs` | Changes to documentation |
| `build` | Changes to build system, compilation, toolchain |  
| `chore` | Chores (internal/organisational changes, updating .gitignore, etc) |
| `misc` | Anything which absolutely does not match any of the other types |

`<subject>` should written as "lowercase, imperative, present tense" (e.g. _'create control thread'_ or _'add init.c to Makefile'_).

## Branch Names

Branches should be named beginning with the `<type>` for the corresponding issue, followed by a `/` and a dash-separated title briefly describing the overall purpose of the branch:

```text
<type>/dash-separated-title
```

## Code Formatting

Code should be formatted before committing using [`trunk`](https://docs.trunk.io) with the command:

```sh
trunk fmt
```

Install `trunk` for Linux or macOS with:

```sh
curl https://get.trunk.io -fsSL | bash
```

Alternatively, install the [VS Code extension](https://marketplace.visualstudio.com/items?itemName=Trunk.io).
