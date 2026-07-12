{
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-26.05";

  outputs =
    { nixpkgs, ... }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];

      forEachSystem = f: nixpkgs.lib.genAttrs systems (system: f (import nixpkgs { inherit system; }));
    in
    {
      formatter = forEachSystem (pkgs: pkgs.nixfmt-tree);

      devShells = forEachSystem (
        pkgs:
        let
          inherit (pkgs) lib;

          target = "i686-w64-mingw32";
          mingw = pkgs.pkgsCross.mingw32;
          mingwCc = mingw.stdenv.cc;
          crossBinPrefix = "${mingwCc}/bin/${mingwCc.targetPrefix}";
          cmocka = mingw.cmocka;
          mcfgthreads = mingw.windows.mcfgthreads;

          fetchMingwZip = url: hash: pkgs.fetchzip { inherit url hash; };
          sdl3Version = "3.4.12";
          sdl3 = mingw.stdenv.mkDerivation {
            pname = "sdl3-i686";
            version = sdl3Version;
            src = pkgs.fetchurl {
              url = "https://github.com/libsdl-org/SDL/releases/download/release-${sdl3Version}/SDL3-${sdl3Version}.tar.gz";
              sha256 = "sha256-8HuViprFAg+3pEytuVf2WLIUnDyKu09jFF+skwMknbc=";
            };

            nativeBuildInputs = with pkgs; [
              cmake
              ninja
            ];

            cmakeFlags = [
              "-DSDL_SHARED=ON"
              "-DSDL_STATIC=OFF"
              "-DSDL_TEST_LIBRARY=OFF"
              "-DSDL_EXAMPLES=OFF"
            ];

            # Match the official mingw devel tarball layout.
            postInstall = ''
              mkdir -p "$out/${target}"
              for d in bin lib include; do
                ln -sn "../$d" "$out/${target}/$d"
              done

              cp ../LICENSE.txt "$out/LICENSE.txt"
            '';
          };
          sdl3Mixer = fetchMingwZip "https://www.libsdl.org/projects/SDL_mixer/release/SDL3_mixer-devel-3.2.4-mingw.tar.gz" "sha256-jvjf/I01gK3efWya1PksSpDbLj7yrtBQDqCl5eXJsio=";

          pkgConfigLibDir = lib.makeSearchPathOutput "" "lib/pkgconfig" [
            "${sdl3}/${target}"
            "${sdl3Mixer}/${target}"
            cmocka
          ];

          ffmpeg = mingw.stdenv.mkDerivation {
            pname = "ffmpeg-rpl-i686";
            version = pkgs.ffmpeg-headless.version;
            src = pkgs.ffmpeg-headless.src;

            nativeBuildInputs = with pkgs; [
              nasm
              perl
            ];

            configurePhase = ''
              runHook preConfigure
              ./configure \
                --prefix=$out \
                --cross-prefix=${crossBinPrefix} \
                --cc=${crossBinPrefix}gcc \
                --cxx=${crossBinPrefix}g++ \
                --ar=${crossBinPrefix}ar \
                --ranlib=${crossBinPrefix}ranlib \
                --windres=${crossBinPrefix}windres \
                --nm=${crossBinPrefix}nm \
                --strip=${crossBinPrefix}strip \
                --host-cc=${pkgs.stdenv.cc}/bin/cc \
                ${lib.escapeShellArgs [
                  "--target-os=mingw32"
                  "--arch=x86"
                  "--enable-cross-compile"
                  "--disable-everything"
                  "--disable-autodetect"
                  "--disable-programs"
                  "--disable-doc"
                  "--disable-debug"
                  "--disable-network"
                  "--disable-avdevice"
                  "--disable-avfilter"
                  "--enable-small"
                  "--enable-gpl"
                  "--disable-static"
                  "--enable-shared"
                  "--enable-avcodec"
                  "--enable-avformat"
                  "--enable-avutil"
                  "--enable-swresample"
                  "--enable-swscale"
                  "--enable-protocol=file"
                  "--enable-demuxer=rpl"
                  "--enable-demuxer=mp3"
                  "--enable-parser=mpegaudio"
                  "--enable-decoder=mp3float"
                  "--enable-decoder=escape124"
                  "--enable-decoder=escape130"
                  "--enable-decoder=pcm_s16le"
                  "--enable-decoder=pcm_s8"
                  "--enable-decoder=pcm_u8"
                  "--enable-decoder=pcm_vidc"
                  "--enable-decoder=adpcm_ima_ea_sead"
                  "--enable-decoder=adpcm_ima_escape"
                ]}
              runHook postConfigure
            '';
          };

          toolchainLinks = {
            inherit cmocka ffmpeg sdl3;
            sdl3_mixer = sdl3Mixer;
          };
          linkToolchainDeps = lib.concatStringsSep "\n" (
            lib.mapAttrsToList (name: path: ''ln -sfn "${path}" "$toolchain_dir/${name}"'') toolchainLinks
          );
        in
        {
          default = pkgs.mkShell {
            packages =
              (with pkgs; [
                just
                cmake
                gnumake
                ninja
                stb
                pkg-config
                nasm
                perl
                python3
                python3Packages.black
                python3Packages.mako
                python3Packages.pefile
                python3Packages.pydantic
                nodejs
                xxd
                llvmPackages_22.clang-tools
                doxygen
                zensical
                imagemagick
                librsvg
                curl
                glab
                zip
              ])
              ++ [
                mingwCc
                cmocka
              ]
              ++ lib.optionals pkgs.stdenv.hostPlatform.isLinux (
                with pkgs;
                [
                  gnused
                  wineWow64Packages.stable
                ]
              );

            shellHook = ''
              export DTTR_PYTHON3="${pkgs.python3}/bin/python3"
              export PATH="${pkgs.python3}/bin:$PATH"

              toolchain_dir="''${DTTR_TOOLCHAIN_DIR:-.toolchain}"
              mkdir -p "$toolchain_dir"
              ${linkToolchainDeps}

              write_tool_wrapper() {
                local wrapper="$1"
                local tool="$2"
                cat > "$toolchain_dir/${target}-$wrapper" <<WRAPPER
              #!/usr/bin/env bash
              exec "${crossBinPrefix}$tool" "\$@"
              WRAPPER
                chmod +x "$toolchain_dir/${target}-$wrapper"
              }

              for wrapper in gcc:gcc g++:g++ windres:windres gcc-ar:ar gcc-ranlib:ranlib; do
                write_tool_wrapper "''${wrapper%%:*}" "''${wrapper#*:}"
              done

              cat > "$toolchain_dir/${target}-pkg-config" <<WRAPPER
              #!/usr/bin/env bash
              set -e
              export PKG_CONFIG_LIBDIR="${pkgConfigLibDir}"
              exec "${pkgs.pkg-config}/bin/pkg-config" "\$@"
              WRAPPER
              chmod +x "$toolchain_dir/${target}-pkg-config"

              cat > "$toolchain_dir/toolchain.cmake" <<CMAKE
              get_filename_component(TOOLCHAIN_DIR "\''${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)
              set(CMAKE_SYSTEM_NAME Windows)
              set(CMAKE_SYSTEM_PROCESSOR i686)
              set(CMAKE_C_COMPILER "\''${TOOLCHAIN_DIR}/${target}-gcc")
              set(CMAKE_CXX_COMPILER "\''${TOOLCHAIN_DIR}/${target}-g++")
              set(CMAKE_AR "\''${TOOLCHAIN_DIR}/${target}-gcc-ar")
              set(CMAKE_RANLIB "\''${TOOLCHAIN_DIR}/${target}-gcc-ranlib")
              set(CMAKE_C_COMPILER_AR "\''${TOOLCHAIN_DIR}/${target}-gcc-ar")
              set(CMAKE_C_COMPILER_RANLIB "\''${TOOLCHAIN_DIR}/${target}-gcc-ranlib")
              set(CMAKE_RC_COMPILER "\''${TOOLCHAIN_DIR}/${target}-windres")
              set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
              set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
              set(PKG_CONFIG_EXECUTABLE "\''${TOOLCHAIN_DIR}/${target}-pkg-config")
              add_link_options("-L${mcfgthreads}/lib" -static-libgcc -static-libstdc++)
              set(CMAKE_CROSSCOMPILING_EMULATOR wine)
              CMAKE
            '';
          };

          shader = pkgs.mkShell {
            packages =
              (with pkgs; [
                just
                python3
                xxd
              ])
              ++ lib.optionals (pkgs.stdenv.hostPlatform.isLinux && pkgs ? shaderc) [
                pkgs.shaderc
              ]
              ++ lib.optionals (pkgs.stdenv.hostPlatform.isLinux && pkgs ? sdl3-shadercross) [
                pkgs.sdl3-shadercross
              ]
              ++ lib.optionals (pkgs.stdenv.hostPlatform.isLinux && pkgs ? vkd3d) [
                pkgs.vkd3d
              ];

            LD_LIBRARY_PATH =
              lib.optionalString
                (pkgs.stdenv.hostPlatform.isLinux && pkgs ? vkd3d)
                (lib.makeLibraryPath [ pkgs.vkd3d ]);

            shellHook =
              lib.optionalString (pkgs.stdenv.hostPlatform.isLinux && !(pkgs ? sdl3-shadercross)) ''
                echo "sdl3-shadercross is not packaged for this nixpkgs/system; use a nixpkgs revision that provides it." >&2
              ''
              + lib.optionalString (pkgs.stdenv.hostPlatform.isLinux && !(pkgs ? vkd3d)) ''
                echo "vkd3d is not packaged for this nixpkgs/system; DXBC shader generation will fail without libvkd3d-utils." >&2
              '';
          };
        }
      );
    };
}
