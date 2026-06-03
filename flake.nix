{
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";

  outputs =
    { nixpkgs, ... }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];
      # Map package and shell outputs across supported host platforms.
      forEachSystem = f: nixpkgs.lib.genAttrs systems (system: f (import nixpkgs { inherit system; }));
    in
    {
      formatter = forEachSystem (pkgs: pkgs.nixfmt-tree);

      packages = forEachSystem (
        pkgs:
        pkgs.lib.optionalAttrs pkgs.stdenv.hostPlatform.isLinux {
          shader-tools = pkgs.buildEnv {
            name = "shader-tools";
            paths = with pkgs; [
              sdl3-shadercross
              shaderc
              gnused
              xxd
            ];
          };
        }
      );

      devShells = forEachSystem (
        pkgs:
        let
          lib = pkgs.lib;
          mingw = pkgs.pkgsCross.mingw32;
          mingwCc = mingw.stdenv.cc;
          crossBinPrefix = "${mingwCc}/bin/${mingwCc.targetPrefix}";
          pkgConfigBin = "${pkgs.pkg-config}/bin/pkg-config";
          sdl3 = pkgs.fetchzip {
            url = "https://github.com/libsdl-org/SDL/releases/download/release-3.4.0/SDL3-devel-3.4.0-mingw.tar.gz";
            hash = "sha256-BadBFy3kWT4v6JJthHaovBvK69AJB9N4ogOIEIL29LQ=";
          };
          sdl3Mixer = pkgs.fetchzip {
            url = "https://www.libsdl.org/projects/SDL_mixer/release/SDL3_mixer-devel-3.2.0-mingw.tar.gz";
            hash = "sha256-rgwPYQpO1IwCqT+gWtvlk3LzS9u00NxGqLS+A4kwv+8=";
          };
          cmocka = mingw.cmocka;
          pkgConfigLibDir = lib.concatStringsSep ":" [
            "${sdl3}/i686-w64-mingw32/lib/pkgconfig"
            "${sdl3Mixer}/i686-w64-mingw32/lib/pkgconfig"
            "${cmocka}/lib/pkgconfig"
          ];
          ffmpegFlags = [
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
          ];
          ffmpegFlagsText = lib.concatStringsSep " \\\n                " ffmpegFlags;
          ffmpeg = mingw.stdenv.mkDerivation {
            pname = "ffmpeg-rpl-i686";
            version = pkgs.ffmpeg-headless.version;
            src = pkgs.ffmpeg-headless.src;
            nativeBuildInputs = [
              pkgs.nasm
              pkgs.perl
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
                ${ffmpegFlagsText}
              runHook postConfigure
            '';
          };
          mcfgthreads = mingw.windows.mcfgthreads;
          pythonRequirements = with pkgs.python3Packages; [
            mako
            pefile
          ];
          baseShellPackages =
            with pkgs;
            [
              just
              cmake
              gnumake
              ninja
              stb
              pkg-config
              nasm
              perl
              python3
              xxd
              mingwCc
              cmocka
            ]
            ++ pythonRequirements;
          formatPackages = with pkgs; [
            clang-tools
            python3Packages.black
          ];
          docsPackages =
            with pkgs;
            [
              just
              doxygen
              python3
              zensical
            ]
            ++ pythonRequirements;
          secureFilesPackages = with pkgs; [ glab ];
          archivePackages = with pkgs; [ zip ];
          uploadPackages = with pkgs; [
            curl
          ];
          shaderToolPackages = lib.optionals pkgs.stdenv.hostPlatform.isLinux (
            with pkgs;
            [
              sdl3-shadercross
              shaderc
              gnused
            ]
          );
          shaderCheckPackages =
            (with pkgs; [
              just
              xxd
            ])
            ++ shaderToolPackages;
          mkDttrShell =
            {
              includeFormat ? false,
              includeDocs ? false,
              includeSecureFiles ? false,
              includePackageTools ? false,
              includeShaderTools ? false,
              includeWine ? false,
              extraPackages ? [ ],
            }:
            pkgs.mkShell {
              packages =
                baseShellPackages
                ++ extraPackages
                ++ lib.optionals includeFormat formatPackages
                ++ lib.optionals includeDocs docsPackages
                ++ lib.optionals includeSecureFiles secureFilesPackages
                ++ lib.optionals includePackageTools (archivePackages ++ uploadPackages)
                ++ lib.optionals includeShaderTools shaderToolPackages
                ++ lib.optionals (includeWine && pkgs.stdenv.hostPlatform.system == "x86_64-linux") [
                  pkgs.wineWowPackages.stable
                ];

              shellHook = ''
                toolchain_dir="''${DTTR_TOOLCHAIN_DIR:-.toolchain}"
                mkdir -p "$toolchain_dir"
                ln -sfn "${sdl3}" "$toolchain_dir/sdl3"
                ln -sfn "${sdl3Mixer}" "$toolchain_dir/sdl3_mixer"
                ln -sfn "${cmocka}" "$toolchain_dir/cmocka"
                ln -sfn "${ffmpeg}" "$toolchain_dir/ffmpeg"

                # Keep CMake on the pinned cross tools.
                write_tool_wrapper() {
                  local wrapper tool
                  wrapper="$1"
                  tool="$2"
                  cat > "$toolchain_dir/i686-w64-mingw32-$wrapper" <<WRAPPER
                #!/usr/bin/env bash
                exec "${crossBinPrefix}$tool" "\$@"
                WRAPPER
                  chmod +x "$toolchain_dir/i686-w64-mingw32-$wrapper"
                }

                for wrapper in gcc g++ windres; do
                  write_tool_wrapper "$wrapper" "$wrapper"
                done
                write_tool_wrapper gcc-ar ar
                write_tool_wrapper gcc-ranlib ranlib

                cat > "$toolchain_dir/i686-w64-mingw32-pkg-config" <<WRAPPER
                #!/usr/bin/env bash
                set -e
                export PKG_CONFIG_LIBDIR="${pkgConfigLibDir}"
                exec "${pkgConfigBin}" "\$@"
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
                add_link_options("-L${mcfgthreads}/lib" -static-libgcc -static-libstdc++)
                set(CMAKE_CROSSCOMPILING_EMULATOR wine)
                CMAKE
              '';
            };
        in
        {
          default = mkDttrShell {
            includeFormat = true;
            includeDocs = true;
            includeSecureFiles = true;
            includePackageTools = true;
            includeShaderTools = true;
            includeWine = true;
          };

          ci-build = mkDttrShell { };
          ci-test = mkDttrShell {
            includeSecureFiles = true;
            includeWine = true;
          };
          ci-docs = pkgs.mkShell {
            packages = docsPackages;
          };
          ci-package = mkDttrShell {
            extraPackages = archivePackages;
          };
          ci-upload = pkgs.mkShell {
            packages = uploadPackages;
          };
          ci-shaders = pkgs.mkShell {
            packages = shaderCheckPackages;
          };
        }
      );
    };
}
