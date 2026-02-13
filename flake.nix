{
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";

  outputs = { self, nixpkgs }:
    let
      systems = [ "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin" ];
      forEachSystem = f: nixpkgs.lib.genAttrs systems (s: f (import nixpkgs { system = s; }));

      sdl3 = pkgs: pkgs.fetchzip {
        url = "https://github.com/libsdl-org/SDL/releases/download/release-3.2.8/SDL3-devel-3.2.8-mingw.tar.gz";
        hash = "sha256-J6UOkXwaCoN+ZTp7nlmccjMWrgeNHpZP4MsCMTTMe/U=";
      };

      vlc = pkgs: pkgs.stdenv.mkDerivation {
        name = "vlc-3.0.23-win32-sdk";
        src = pkgs.fetchurl {
          url = "https://download.videolan.org/pub/videolan/vlc/3.0.23/win32/vlc-3.0.23-win32.7z";
          hash = "sha256-8Uj/Sc2sbAtrcBitfE5s0kyZvGwt6oJY2CaEJhpjkBc=";
        };
        nativeBuildInputs = [ pkgs.p7zip ];
        unpackPhase = "7z x $src";
        installPhase = ''
          mkdir -p $out
          cp -r vlc-3.0.23/* $out/
        '';
      };

    in {
      packages = forEachSystem (pkgs: {
        shader-tools = pkgs.buildEnv {
          name = "shader-tools";
          paths = [
            pkgs."sdl3-shadercross"
            pkgs.shaderc
            pkgs.xxd
          ];
        };
      });

      devShells = forEachSystem (pkgs: let
        s = sdl3 pkgs;
        v = vlc pkgs;
        mcfg = pkgs.pkgsCross.mingw32.windows.mcfgthreads;
      in {
        default = pkgs.mkShell {
          packages = with pkgs; [
            go-task
            cmake
            gnumake
            ninja
            stb
            pkg-config
            clang-tools
            nasm
            perl
            xxd
            doxygen
            curl
            zip
            pkgsCross.mingw32.stdenv.cc
          ] ++ lib.optionals stdenv.hostPlatform.isLinux [
            sdl3-shadercross
            shaderc
          ];

          shellHook = ''
            toolchain_dir="''${DTTR_TOOLCHAIN_DIR:-.toolchain}"
            mkdir -p "$toolchain_dir"
            ln -sfn "${s}" "$toolchain_dir/sdl3"
            ln -sfn "${v}" "$toolchain_dir/vlc"
            for tool in gcc windres gcc-ar gcc-ranlib; do
              cat > "$toolchain_dir/i686-w64-mingw32-$tool" << EOF
            #!/usr/bin/env bash
            exec $(which i686-w64-mingw32-$tool) "\$@"
            EOF
              chmod +x "$toolchain_dir/i686-w64-mingw32-$tool"
            done

            cat > "$toolchain_dir/i686-w64-mingw32-pkg-config" << EOF
            #!/usr/bin/env bash
            set -e
            base_pkgconfig="${s}/i686-w64-mingw32/lib/pkgconfig"
            export PKG_CONFIG_LIBDIR="\''${base_pkgconfig}"
            exec "$(which pkg-config)" "\$@"
            EOF
            chmod +x "$toolchain_dir/i686-w64-mingw32-pkg-config"

            cat > "$toolchain_dir/toolchain.cmake" << EOF
            get_filename_component(TOOLCHAIN_DIR "\''${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)
            set(CMAKE_SYSTEM_NAME Windows)
            set(CMAKE_SYSTEM_PROCESSOR i686)
            set(CMAKE_C_COMPILER "\''${TOOLCHAIN_DIR}/i686-w64-mingw32-gcc")
            set(CMAKE_AR "\''${TOOLCHAIN_DIR}/i686-w64-mingw32-gcc-ar")
            set(CMAKE_RANLIB "\''${TOOLCHAIN_DIR}/i686-w64-mingw32-gcc-ranlib")
            set(CMAKE_C_COMPILER_AR "\''${TOOLCHAIN_DIR}/i686-w64-mingw32-gcc-ar")
            set(CMAKE_C_COMPILER_RANLIB "\''${TOOLCHAIN_DIR}/i686-w64-mingw32-gcc-ranlib")
            set(CMAKE_RC_COMPILER "\''${TOOLCHAIN_DIR}/i686-w64-mingw32-windres")
            set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
            set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
            set(PKG_CONFIG_EXECUTABLE "\''${TOOLCHAIN_DIR}/i686-w64-mingw32-pkg-config")
            link_directories("${mcfg}/lib")
            add_link_options(-static-libgcc)
            set(CMAKE_CROSSCOMPILING_EMULATOR wine)
            EOF
          '';
        };
      });
    };
}
