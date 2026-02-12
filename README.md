# declarative-arch-manager
A declarative arch system manager (damngr).
It's written in C23 to declaratively set up your arch installation.

## Building damngr
1. Clone the repository: 
- `git clone --recurse-submodules https://github.com/goajos/declarative-arch-manager.git`
or
- `git clone --recurse-submodules git@github.com:goajos/declarative-arch-manager.git`
2. cd damngr
3. Run `sudo make install`

## Available damngr commands:
- `damngr init`
- `damngr merge`
- `damngr update`

### damngr init
Use `damngr init` to set up an example folder structure (`~/.config/damngr`).
It also ensures that the `~/.local/state/damngr` folder is available for merging.

### damngr merge
Use `damngr merge` to merge the declared state, currently the command will:
- check for removed modules:
    - remove the (aur)-packages
    - disable the services
    - unlink the dotfiles
- check for new modules:
    - install the packages
    - install the aur-packages
    - link the dotfiles
    - run the post install hooks

### damngr update
Use `damngr update` to update the installed packages. If aur\_helper is available it will use this over native pacman to update.

## damngr folder structure
```
.config/damngr/
    | config.kdl
    | dotfiles/
        | <module1>/ // implicit modules dotfile folder
            | ...
    | hooks/
        | user_hook.sh
    | hosts/
        | <host>.kdl
    | modules\
        | <module1>.kdl
        | <module2>.kdl
```
```
.state/damngr/
    | config_state.kdl
    | <host>_state.kdl
    | <module1>_state.kdl
    | <module2>_state.kdl
```

## damngr config.kdl
Stored in `~/.config/damngr/config.kdl`. Declares the aur\_helper to use (currently tested with yay and paru) and the active host. E.g.
```
config {
    aur_helper paru
    active_host example_host
}
```

## damngr \<host\>.kdl
Stored in `~/.config/damngr/hosts/<host>.kdl`. Declares the hosts modules and root services. E.g.
```
host {
    example_host {
        modules {
            example_module
        }
        services {
            example_root_service // implicit root=#true
        }
    }
}
```

## damngr \<module\>.kdl
Stored in `~/.config/damngr/modules/<module>.kdl`. Declares the modules dotfile linking, (aur-)packages, services and hooks. E.g.
```
module {
    example_module {
        dotfiles link=#true // or #false
        packages {
            example_package
        }
        aur_packages {
            example_aur_package
        }
        services {
            example_user_service // implicit root=#false
        }
        hooks {
            example_user_hook.sh root=#false
            example_root_hook.sh root=#true
        }
    }    
}
```
## More make commands:
- `sudo make uninstall` to remove the damngr executable and clear the share/damngr folder
- `sudo make clean` to remove the local build folders 

