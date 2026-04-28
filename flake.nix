{
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";

  outputs = { self, nixpkgs }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];
      forEachSystem = f:
        nixpkgs.lib.genAttrs systems (system: f (import nixpkgs { inherit system; }));
    in {
      packages = forEachSystem (pkgs: {
        shader-tools = pkgs.buildEnv {
          name = "shader-tools";
          paths = [
            pkgs.sdl3-shadercross
            pkgs.shaderc
            pkgs.xxd
          ];
        };
      });

      devShells = forEachSystem (pkgs:
        let
          sdl3 = pkgs.fetchzip {
            url = "https://github.com/libsdl-org/SDL/releases/download/release-3.2.8/SDL3-devel-3.2.8-mingw.tar.gz";
            hash = "sha256-J6UOkXwaCoN+ZTp7nlmccjMWrgeNHpZP4MsCMTTMe/U=";
          };
          mpv = pkgs.stdenv.mkDerivation {
            name = "libmpv-dev-i686";
            src = pkgs.fetchurl {
              url = "https://sourceforge.net/projects/mpv-player-windows/files/libmpv/mpv-dev-i686-20260201-git-40d2947.7z/download";
              hash = "sha256-UOHLNYHEX2gwB2+MC/wh4yCaweJU0k3zgq6HAUGBdsA=";
            };
            nativeBuildInputs = [ pkgs.p7zip ];
            unpackPhase = "7z x $src";
            installPhase = ''
              mkdir -p $out
              cp -r * $out/
            '';
          };
          mcfg = pkgs.pkgsCross.mingw32.windows.mcfgthreads;
        in {
          default = pkgs.mkShell {
            packages = with pkgs;
              [
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
              ]
              ++ lib.optionals stdenv.hostPlatform.isLinux [
                sdl3-shadercross
                shaderc
              ];

            shellHook = ''
              toolchain_dir="''${DTTR_TOOLCHAIN_DIR:-.toolchain}"
              mkdir -p "$toolchain_dir"
              ln -sfn "${sdl3}" "$toolchain_dir/sdl3"
              ln -sfn "${mpv}" "$toolchain_dir/mpv"

              for tool in gcc g++ windres gcc-ar gcc-ranlib; do
                cat > "$toolchain_dir/i686-w64-mingw32-$tool" <<WRAPPER
              #!/usr/bin/env bash
              exec $(which i686-w64-mingw32-$tool) "\$@"
              WRAPPER
                chmod +x "$toolchain_dir/i686-w64-mingw32-$tool"
              done

              cat > "$toolchain_dir/i686-w64-mingw32-pkg-config" <<WRAPPER
              #!/usr/bin/env bash
              set -e
              base_pkgconfig="${sdl3}/i686-w64-mingw32/lib/pkgconfig"
              export PKG_CONFIG_LIBDIR="\''${base_pkgconfig}"
              exec "$(which pkg-config)" "\$@"
              WRAPPER
              chmod +x "$toolchain_dir/i686-w64-mingw32-pkg-config"

              cat > "$toolchain_dir/toolchain.cmake" <<CMAKE
              get_filename_component(TOOLCHAIN_DIR "\''${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)
              set(CMAKE_SYSTEM_NAME Windows)
              set(CMAKE_SYSTEM_PROCESSOR i686)
              set(CMAKE_C_COMPILER "\''${TOOLCHAIN_DIR}/i686-w64-mingw32-gcc")
              set(CMAKE_CXX_COMPILER "\''${TOOLCHAIN_DIR}/i686-w64-mingw32-g++")
              set(CMAKE_AR "\''${TOOLCHAIN_DIR}/i686-w64-mingw32-gcc-ar")
              set(CMAKE_RANLIB "\''${TOOLCHAIN_DIR}/i686-w64-mingw32-gcc-ranlib")
              set(CMAKE_C_COMPILER_AR "\''${TOOLCHAIN_DIR}/i686-w64-mingw32-gcc-ar")
              set(CMAKE_C_COMPILER_RANLIB "\''${TOOLCHAIN_DIR}/i686-w64-mingw32-gcc-ranlib")
              set(CMAKE_RC_COMPILER "\''${TOOLCHAIN_DIR}/i686-w64-mingw32-windres")
              set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
              set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
              set(PKG_CONFIG_EXECUTABLE "\''${TOOLCHAIN_DIR}/i686-w64-mingw32-pkg-config")
              link_directories("${mcfg}/lib")
              add_link_options(-static-libgcc -static-libstdc++)
              set(CMAKE_CROSSCOMPILING_EMULATOR wine)
              CMAKE
            '';
          };
        });
    };
}
