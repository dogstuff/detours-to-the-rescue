{
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";

  outputs = { nixpkgs, ... }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];
      forEachSystem = f:
        nixpkgs.lib.genAttrs systems (system: f (import nixpkgs { inherit system; }));
    in
    {
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
          pkgConfigLibDir = lib.concatStringsSep ":" [
            "${sdl3}/i686-w64-mingw32/lib/pkgconfig"
            "${sdl3Mixer}/i686-w64-mingw32/lib/pkgconfig"
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
            nativeBuildInputs = [ pkgs.nasm pkgs.perl ];
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
          mcfg = mingw.windows.mcfgthreads;
        in
        {
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
                mingwCc
              ]
              ++ lib.optionals stdenv.hostPlatform.isLinux [
                sdl3-shadercross
                shaderc
              ];

            shellHook = ''
              toolchain_dir="''${DTTR_TOOLCHAIN_DIR:-.toolchain}"
              mkdir -p "$toolchain_dir"
              ln -sfn "${sdl3}" "$toolchain_dir/sdl3"
              ln -sfn "${sdl3Mixer}" "$toolchain_dir/sdl3_mixer"
              ln -sfn "${ffmpeg}" "$toolchain_dir/ffmpeg"

              write_tool_wrapper() {
                wrapper="$1"
                tool="$2"
                cat > "$toolchain_dir/i686-w64-mingw32-$wrapper" <<WRAPPER
              #!/usr/bin/env bash
              exec "${crossBinPrefix}$tool" "\$@"
              WRAPPER
                chmod +x "$toolchain_dir/i686-w64-mingw32-$wrapper"
              }

              write_tool_wrapper gcc gcc
              write_tool_wrapper g++ g++
              write_tool_wrapper windres windres
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
              add_link_options("-L${mcfg}/lib" -static-libgcc -static-libstdc++)
              set(CMAKE_CROSSCOMPILING_EMULATOR wine)
              CMAKE
            '';
          };
        });
    };
}
