# VCU Specific Contributing Guidelines

## Conventional Commits

Commits (and issue/pull request titles) should follow the [conventional commits specification](https://www.conventionalcommits.org/en/v1.0.0/) with the format:

```
<type>(<scope>): <subject>
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

`<scope>` should be a single word describing a part of the system such as (but not limited to):
| Scope | Description |
|------------|----------------------------------------------------------------|
| `apps` | Accelerator pedal position sensor |
| `can` | Communication with devices on the CAN bus |
| `control` | Control algorithms (throttle mapping, traction control...) |
| `drs` | Drag reduction system |
| `io` | Reading/writing/configuring general I/O |
| `rtd` | Ready-to-drive procedure |
| `rtos` | Real-time operating system (configuration, thread creation...) |
| `safety` | Safety checks, fault handling |

`<subject>` should written as "lowercase, imperative, present tense" (e.g. _'create control thread'_ or _'add init.c to Makefile'_).

## Branch Names

Branches should be named beginning with the `<type>` for the corresponding issue, followed by a `/` and a dash-separated title briefly describing the overall purpose of the branch:

```
<type>/dash-separated-title
```

## Code Formatting

Code should be formatted before committing using [`trunk`](https://docs.trunk.io) with the command:

```
trunk fmt
```

Install `trunk` for Linux or macOS with:

```
curl https://get.trunk.io -fsSL | bash
```

Alternatively, install the [VS Code extension](https://marketplace.visualstudio.com/items?itemName=Trunk.io).
