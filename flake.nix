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
        nativeBuildInputs = [ ];
        buildInputs = with pkgs; [
          cmake
          ninja
        ];
      in
      {
        devShells.default =
          pkgs.mkShell {
            packages =
              with pkgs;
              [
                llvmPackages_21.clang-unwrapped
                llvmPackages_21.lld
                llvmPackages_21.clang-tools
                llvmPackages_21.bintools
                lldb
                codespell

                xorriso
                gnumake
                nasm
                qemu
              ]
              ++ buildInputs
              ++ nativeBuildInputs;
          };
      }
    );
}
