# declarative-arch-manager
A declarative arch system manager (damgr).
It's written in C23 to declaratively set up an Arch system.

## Building damgr
1. Clone the repository: 
- `git clone https://github.com/goajos/declarative-arch-manager.git`
or
- `git clone git@github.com:goajos/declarative-arch-manager.git`
2. cd damgr
3. Run `make` to install the damgr binary

## Available damgr commands:
- `damgr init`
- `damgr merge`
- `damgr update`

## TODO commands
- `damgr validate` # validate the current config files.
- `dangr reset` # reset the hooks.

### damgr init
Use `damgr init` to set up an example folder structure (`~/.config/damgr`).
It also ensures that the `~/.local/state/damgr` folder is available for merging.

### damgr merge
Use `damgr merge` to merge the declared state, currently the command will:
- check for removed modules:
    - remove the (aur)-packages
    - disable the services
    - unlink the dotfiles
- check for new modules:
    - install the packages
    - install the aur-packages
    - link the dotfiles
    - run the post install hooks

### damgr update
Use `damgr update` to update the installed packages. If aur\_helper is available it will use this over native pacman to update.

## damgr folder structure
```
.config/damgr/
    | config.conf
    | dotfiles/
        | <module1>/ // implicit modules dotfile folder
            | ...
    | hooks/
        | user_hook.sh
    | hosts/
        | <host>.conf
    | modules\
        | <module1>.conf
        | <module2>.conf
```
```
.local/.state/damgr/
    | config_state.conf
    | <host>_state.conf
    | <module1>_state.conf
    | <module2>_state.conf
```

## damgr config.conf
Stored in `~/.config/damgr/config.conf`. Declares the aur\_helper to use (currently tested with yay and paru) and the active host. E.g.
```
aur_helper=yay
active_host=<host>
```

## damgr \<host\>.conf
Stored in `~/.config/damgr/hosts/<host>.conf`. Declares the hosts modules and root services. E.g.
```
modules=
    example_module
services=
    example_root_service // implicit :true
```

## damgr \<module\>.conf
Stored in `~/.config/damgr/modules/<module>.conf`. Declares the modules dotfile linking, (aur-)packages, services and (pre/post-)hooks. E.g.
```
dotfiles=link:true // or :false
packages=
    example_package

aur_packages=
    example_aur_package

services=
    example_user_service // implicit :false

pre-hooks=
    example_user_hook.sh:false

post-hooks=
    example_root_hook.sh:true
```
## More Make commands:
- `make clean` to remove the local build folders.
- `make test` will create a Docker container to test the damgr commands in a containerized environment.

## TODO more Make commands
- `make uninstall` to remove the damgr executable and clear the share/damgr folder.

