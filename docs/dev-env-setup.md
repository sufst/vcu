# Setup / Development Environment

When setting up your development environment,

## Method 1: Using the setup script

Run the below command once to install dependencies, clone the repository and initialise submodules

```sh
curl -LsSf https://raw.githubusercontent.com/sufst/vcu/refs/heads/stag-12/vcu-setup-script.sh | bash
```

## Method 2: Manually

First, clone the repository:

```sh
git clone https://github.com/sufst/vcu
git checkout stag-12 # (or any specific branch you want to work on)
```

### Submodules

This project depends on middlewares in the `src/Middlewares/` folder, some of
which are Git submodules. When first cloning this repo, run the following
commands:

```sh
git submodule init
git submodule update
```

For more information on submodules, see the [Git submodules documentation](https://git-scm.com/book/en/v2/Git-Tools-Submodules).

### Building and Flashing

To build this project and flash code to the microcontroller, you will need the
following on your path:

- [Make](https://www.gnu.org/software/make/)
- [ARM GNU Embedded Toolchain](https://developer.arm.com/downloads/-/gnu-rm)
- [STLink Open Source Toolset](https://github.com/stlink-org/stlink)
- `mkdir`, `rm`, `tput` and `echo`

Build with:

```sh
make -j -s
```

Flash with:

```sh
make flash
```

For detailed toolchain setup instructions, see the SUFST Docs Site.

> Note: Windows users should run these commands from Git Bash.

## VS Code

This project is set up to be edited and debugged in [VS Code](https://code.visualstudio.com).
The `.vscode`folder includes tasks and launch configurations. To improve
Intellisense, it is recommended to use [`ccdgen`](https://github.com/t-bre/ccdgen).

Make sure it is installed with the following command, which may differ depending on your
environment:

```sh
python3 -m pip install ccdgen
```

Then, you can generate the compile commands database using the relevant `Makefile` target:

```sh
make -s ccd
```

Since the toolchain is set up to be fully command line based, it is also
possible to use other code editors.

> Note: Windows users should set Git Bash as the shell in VS Code.

## STM32CubeMX

[STM32CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html) is used
to generate boilerplate initialisation code for the microcontroller. These
configurations are stored in `src/VCU.ioc` which should not be edited manually.
To minimise the chance of merge conflicts, changes to the `.ioc` should be made
as infrequently as possible as the `.ioc` format is not well suited to version
control.

> Note: CubeMX generates a Makefile in the `src` folder. This should _not_ be
> used, there is a custom Makefile in the project root. If CubeMX adds something
> to the Makefile it generates which is not in the custom Makefile, it should be
> copied over. This should only happen infrequently when adding a new peripheral
> or CubeMX managed middlewares.

## Trunk

We use [trunk.io](https://trunk.io/) for testing which gives us [MISRA Compliance](https://misra.org.uk/compliance/). Once installed, you can run `trunk check`. This is also automatically run via a GitHub action, on every push to GitHub. (see `./.github/workflows/trunk-action.yaml`)
