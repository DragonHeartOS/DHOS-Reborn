{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs { inherit system; };
        llvm = pkgs.llvmPackages_21;
        limineClang = pkgs.writeShellScriptBin "limine-clang" ''
          exec ${llvm.clang}/bin/clang "$@"
        '';
        limineSrc = pkgs.fetchFromGitHub {
          owner = "limine-bootloader";
          repo = "limine";
          rev = "5be26a73d7b7b4d4477d18be94e1d16e615adf56";
          hash = "sha256-lBPx5B3yiuWC+CiaygsOwCWKTEnLU2Wv/DE+msGXM6w=";
        };
        dhos = pkgs.stdenvNoCC.mkDerivation {
          pname = "dhos";
          version = "0.1";
          src = pkgs.lib.cleanSource ./.;

          postPatch = ''
            patchShebangs Util/*.sh
          '';

          postUnpack = ''
            cp -R ${limineSrc} "$sourceRoot/limine"
            chmod -R u+w "$sourceRoot/limine"
          '';

          nativeBuildInputs = with pkgs; [
            cmake
            ninja
            gnumake
            nasm
            python3
            xorriso
            llvm.clang-unwrapped
            llvm.bintools
            llvm.lld
          ];

          configurePhase = ''
            cmake -G Ninja -B Build \
              -DCMAKE_BUILD_TYPE=Debug \
              -DCMAKE_TOOLCHAIN_FILE=cmake/x86_64-elf-clang.cmake \
              -DCMAKE_C_COMPILER=${llvm.clang-unwrapped}/bin/clang \
              -DCMAKE_CXX_COMPILER=${llvm.clang-unwrapped}/bin/clang++ \
              -DCLANG=${llvm.clang-unwrapped}/bin/clang \
              -DCLANGXX=${llvm.clang-unwrapped}/bin/clang++ \
              -DLLD=${llvm.lld}/bin/ld.lld \
              -DNASM=${pkgs.nasm}/bin/nasm \
              -DCMAKE_AR=llvm-ar \
              -DCMAKE_RANLIB=llvm-ranlib
          '';

          buildPhase = ''
            export MAKEFLAGS="CC=${limineClang}/bin/limine-clang"
            cmake --build Build --target cdrom
          '';

          installPhase = ''
            mkdir -p "$out"
            cp Build/dhos.iso "$out/dhos.iso"
          '';
        };
      in
      {
        packages.default = dhos;

        devShells.default =
          pkgs.mkShellNoCC {
            packages =
              with pkgs;
              [
                llvm.clang-unwrapped
                llvm.lld
                llvm.clang-tools
                llvm.bintools
                lldb
                codespell

                xorriso
                gnumake
                nasm
                qemu
              ]
              ++ [
                cmake
                ninja
              ];
          };
      }
    );
}
